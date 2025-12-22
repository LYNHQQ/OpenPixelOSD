import serial
import asyncio
import json
from enum import Enum
from typing import List


class MspState(Enum):
    MSP_SYNC_DOLLAR = 0
    MSP_SYNC_X = 1
    MSP_TYPE = 2
    MSP_FLAG = 3
    MSP_FUNCTION = 4
    MSP_PAYLOAD_SIZE = 5
    MSP_PAYLOAD = 6
    MSP_CRC = 7
    MSP_TYPE_M = 8
    MSP_FUNCTION_M = 9
    MSP_PAYLOAD_SIZE_M = 10
    MSP_PAYLOAD_M = 11
    MSP_CRC_M = 12

MSP_HEADER_DOLLAR = ord('$')
MSP_HEADER_X = ord('X')
MSP_HEADER_M = ord('M')
MSP_HEADER_REQUEST = ord('<')
MSP_HEADER_RESPONSE = ord('>')
MSP_HEADER_ERROR = ord('!')


class MspFunction():
  MSP_VTX_CONFIG  = 88
  MSP_VTXTABLE_POWERLEVEL = 138
  MSP_SET_VTXTABLE_POWERLEVEL = 228
  MSP_VTXTABLE_BAND = 137
  MSP_SET_VTXTABLE_BAND = 227
  MSP_DEBUG = 254
  MSP_STATUS = 101
  MSP_RC = 105
  MSP_EEPROM_WRITE = 250
  MSP_PACALTABLE = 0x4800
  MSP_SET_PACALTABLE = 0x4801
  MSP_PACALIBRATION = 0x4802
  MSP_SET_PACALIBRATION = 0x4803


class PaCalibration:
  def __init__(self):
    self.idx: int = 0
    self.mW: int = 0
    self.value: list[int] = []
    self.detector: list[int] = []

  def to_dict(self) -> dict:
    return {
      'idx': self.idx,
      'mW': self.mW,
      'value': self.value,
      'detector': self.detector
    }

  @staticmethod
  def from_dict(data: dict) -> 'PaCalibration':
    obj = PaCalibration()
    obj.idx = data.get('idx', 0)
    obj.mW = data.get('mW', 0)
    obj.value = data.get('value', [])
    obj.detector = data.get('detector', [])
    return obj


class PaCalTable:
  def __init__(self):
    self.pa_table: list[PaCalibration] = []

  def append(self, item: PaCalibration):
    self.pa_table.append(item)

  def clear(self):
    self.pa_table.clear()

  def __iter__(self):
    return iter(self.pa_table)

  def __len__(self):
    return len(self.pa_table)

  def __getitem__(self, index):
    return self.pa_table[index]

  def to_dict(self) -> list[dict]:
    return [item.to_dict() for item in self.pa_table]

  def save_json(self, filename: str):
    try:
      with open(filename, 'w') as f:
        json.dump(self.to_dict(), f, indent=2)
    except:
      raise

  @classmethod
  def load_json(cls, filename: str) -> 'PaCalTable':
    try:
      with open(filename, 'r') as f:
        data = json.load(f)
      table = cls()
      for item_dict in data:
        table.append(PaCalibration.from_dict(item_dict))
      return table
    except:
      raise


class MSP:

  def __init__(self, port: str, baudrate: int = 115200):
    self.port_name = port
    self.baudrate = baudrate
    self.serial = None
    self.in_buffer = ""
    self.interval_ms = 1000
    self.running = False

    self.state = MspState.MSP_SYNC_DOLLAR
    self.in_crc = 0
    self.in_function = 0
    self.in_payload_size = 0
    self.in_type = 0
    self.in_idx = 0
    self.receive_array: List[int] = []
    self.payload: List[int] = []
    self.payload_size = 0
    self.receive_frame = bytearray()
    self.msp_version = 0
    self.frequency = 5800
    self.band = 0
    self.channel = 0
    self.power = 1
    self.pitmode = 1
    self.pa_table: PaCalTable = PaCalTable()
    self.detector = 0
    self.mvPa = 0

  async def open(self):
    try:
      self.serial = serial.Serial(self.port_name, self.baudrate, timeout=0)
    except:
      print(f"{self.port_name} not ready")
      raise

    self.running = True
    asyncio.create_task(self._poll_serial())


  async def close(self):
    self.running = False
    await asyncio.sleep(0.1) 
    if self.serial and self.serial.is_open:
        self.serial.close()


  def msp_checksum(self, data: bytes) -> int:
    csum = 0
    for b in data:
        csum ^= b
    return csum


  def msp_calc_crc(self, crc: int, b: int) -> int:
    crc ^= b
    for _ in range(8):
      if crc & 0x80:
        crc = ((crc << 1) ^ 0xD5) & 0xFF
      else:
        crc = (crc << 1) & 0xFF
    return crc


  def msp_checksumV2(self,data: bytes) -> int:
    crc = 0
    for b in data:
      crc = self.msp_calc_crc(crc, b)
    return crc


  def build_msp_commandV1(self, cmd: int, payload: bytes) -> bytes:
    length = len(payload)
    header = b"$M<"
    frame = bytes([length, cmd]) + payload
    csum = self.msp_checksum(frame)
    return header + frame + bytes([csum])


  def build_msp_commandV2(self, cmd: int, payload: bytes) -> bytes:
    header = b"$X<"
    flag = 0
    if (payload != None):
      length = len(payload)
      frame = bytes([flag, cmd & 0xff, (cmd >> 8) &0xff, length & 0xff, (length >> 8) & 0xff]) + payload
    else:
      frame = bytes([flag, cmd & 0xff, (cmd >> 8) &0xff, 0,0])

    csum = self.msp_checksumV2(frame)
    return header + frame + bytes([csum])
  

  def process_packet(self, msp_type: int, function: int, payload_size: int):
      
      match function:
        case MspFunction.MSP_STATUS:
          payload  = bytes([0,0,0,0,0,0,0,0])
          cmd = self.build_msp_commandV1(MspFunction.MSP_STATUS, payload)
          self.serial.write(cmd)

        case MspFunction.MSP_VTX_CONFIG:
          self.send_MSP_VTX_CONFIG()

        case MspFunction.MSP_RC:
          pass

        case MspFunction.MSP_SET_PACALTABLE:
          item = PaCalibration()
          try:
            if self.payload[0] == 0:
              self.pa_table.clear()

            item.idx = self.payload[0]
            item.mW = (self.payload[2] << 8) | self.payload[1]
            for i in range(7):
              val = (self.payload[4 + i * 2] << 8) | self.payload[3 + i * 2]
              item.value.append(val)
            for i in range(7):
              det = (self.payload[18 + i * 2] << 8) | self.payload[17 + i * 2]
              item.detector.append(det)
            self.pa_table.append(item)
          
          except Exception as ex:
            pass  

        case MspFunction.MSP_PACALIBRATION:
          self.detector = (self.payload[4] << 8) | self.payload[3]
          self.mvPa = (self.payload[2] << 8) | self.payload[1]
        case _:
          #print(f"Unknown MSPv{self.msp_version} function={function}, size={payload_size}")
          pass


  def send_MSP_VTX_CONFIG(self):
    payload =  bytes([5,self.band,self.channel,self.power,self.pitmode]) #vtxtype,band,channel,power,pitmode 
    payload += bytes([self.frequency & 0xff, (self.frequency >> 8) & 0xff])
    payload += bytes([1,0])
    payload += bytes([5600 & 0xff, (5600 >> 8) & 0xff])
    payload += bytes([1,5,8,4]) #vtxtable,bandcount,channels,powerlevels
    cmd = self.build_msp_commandV1(MspFunction.MSP_VTX_CONFIG, payload)
    self.serial.write(cmd)


  def send_MSP_SET_PACALTABLE(self):
    for i in range(len(self.pa_table)-1):
      payload = bytes([self.pa_table[i+1].idx])
      payload += bytes([self.pa_table[i+1].mW & 0xff, (self.pa_table[i+1].mW >> 8) & 0xff])
            
      for j in range(7):
        payload += bytes([self.pa_table[i+1].value[j] & 0xff, (self.pa_table[i+1].value[j] >> 8) & 0xff])
      for j in range(7):
        payload += bytes([self.pa_table[i+1].detector[j] & 0xff, (self.pa_table[i+1].detector[j] >> 8) & 0xff])
      cmd = self.build_msp_commandV2(MspFunction.MSP_SET_PACALTABLE, payload)
      self.serial.write(cmd)


  def send_MSP_PACALTABLE(self):
    cmd = self.build_msp_commandV2(MspFunction.MSP_PACALTABLE, None)
    self.serial.write(cmd)


  def send_MSP_SET_PACALIBRATION(self, paValue):
    payload =  bytes([self.power])
    payload += bytes([paValue & 0xff, (paValue >> 8) & 0xff])
    cmd = self.build_msp_commandV2(MspFunction.MSP_SET_PACALIBRATION, payload)
    self.serial.write(cmd)


  def send_MSP_EEPROM_WRITE(self):
    cmd = self.build_msp_commandV2(MspFunction.MSP_EEPROM_WRITE, None)
    self.serial.write(cmd)


  async def _poll_serial(self):
    while self.running:
      await asyncio.sleep(0.01)
      if self.serial and self.serial.in_waiting > 0:
        while self.serial and self.serial.in_waiting > 0:
          data = self.serial.read(1)
          if not data:
            break
          byte = data[0]

          if self.in_idx == 0:
            self.receive_array.clear()

          self.receive_array.append(byte)
          self.in_idx += 1

          match self.state:
            case MspState.MSP_SYNC_DOLLAR:
              self.state = MspState.MSP_SYNC_X if byte == MSP_HEADER_DOLLAR else MspState.MSP_SYNC_DOLLAR
              self.in_idx = 0 if byte != MSP_HEADER_DOLLAR else self.in_idx

            case MspState.MSP_SYNC_X:
              if byte == MSP_HEADER_X:
                self.state = MspState.MSP_TYPE
              elif byte == MSP_HEADER_M:
                self.state = MspState.MSP_TYPE_M
              else:
                self.state = MspState.MSP_SYNC_DOLLAR
                self.in_idx = 0

            case MspState.MSP_TYPE:
              if byte in (MSP_HEADER_REQUEST, MSP_HEADER_RESPONSE, MSP_HEADER_ERROR):
                self.in_type = byte
                self.state = MspState.MSP_FLAG
                self.in_crc = 0
              else:
                self.state = MspState.MSP_SYNC_DOLLAR
                self.in_idx = 0

            case MspState.MSP_FLAG:
              self.in_crc = self.msp_calc_crc(self.in_crc, byte)
              self.state = MspState.MSP_FUNCTION

            case MspState.MSP_FUNCTION:
              self.in_crc = self.msp_calc_crc(self.in_crc, byte)
              if self.in_idx == 6:
                self.in_function = (self.receive_array[5] << 8) | self.receive_array[4]
                self.state = MspState.MSP_PAYLOAD_SIZE

            case MspState.MSP_PAYLOAD_SIZE:
              self.in_crc = self.msp_calc_crc(self.in_crc, byte)
              if self.in_idx == 8:
                self.in_payload_size = (self.receive_array[7] << 8) | self.receive_array[6]
                #print(f"MSPv2 Payload Size: {self.in_payload_size}")
                self.state = MspState.MSP_PAYLOAD if self.in_payload_size > 0 else MspState.MSP_CRC

            case MspState.MSP_PAYLOAD:
              self.in_crc = self.msp_calc_crc(self.in_crc, byte)
              if self.in_idx >= (8 + self.in_payload_size):
                self.state = MspState.MSP_CRC

            case MspState.MSP_CRC:
              if self.in_crc == byte:
                if self.in_type != MSP_HEADER_ERROR:
                  self.payload = self.receive_array[8:]
                  self.payload_size = self.in_payload_size
                  self.receive_frame = bytearray(self.receive_array)
                  self.msp_version = 2
                  self.process_packet(self.in_type, self.in_function, self.in_payload_size)
              else:
                pass
              self.state = MspState.MSP_SYNC_DOLLAR
              self.in_idx = 0

            case MspState.MSP_TYPE_M:
              if byte in (MSP_HEADER_REQUEST, MSP_HEADER_RESPONSE, MSP_HEADER_ERROR):
                self.in_type = byte
                self.state = MspState.MSP_PAYLOAD_SIZE_M
              else:
                self.state = MspState.MSP_SYNC_DOLLAR
                self.in_idx = 0

            case MspState.MSP_PAYLOAD_SIZE_M:
              self.in_crc = byte
              self.in_payload_size = byte
              self.state = MspState.MSP_FUNCTION_M

            case MspState.MSP_FUNCTION_M:
              self.in_crc ^= byte
              self.in_function = byte
              self.state = MspState.MSP_PAYLOAD_M if self.in_payload_size > 0 else MspState.MSP_CRC_M

            case MspState.MSP_PAYLOAD_M:
              self.in_crc ^= byte
              if self.in_idx == (5 + self.in_payload_size):
                self.state = MspState.MSP_CRC_M

            case MspState.MSP_CRC_M:
              if self.in_crc == byte:
                if self.in_type != MSP_HEADER_ERROR:
                  self.payload = self.receive_array[5:]
                  self.payload_size = self.in_payload_size
                  self.receive_frame = bytearray(self.receive_array)
                  self.msp_version = 1
                  self.process_packet(self.in_type, self.in_function, self.in_payload_size)
              self.state = MspState.MSP_SYNC_DOLLAR
              self.in_idx = 0

            case _:
              self.state = MspState.MSP_SYNC_DOLLAR
              self.in_idx = 0

