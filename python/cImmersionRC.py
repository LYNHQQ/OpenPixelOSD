import serial
import asyncio

class CMD:
  idle = 0
  frequency = 1
  power = 2

class PowerMeter:
  def __init__(self, port: str, baudrate: int = 115200):
    self.port_name = port
    self.baudrate = baudrate
    self.serial = None
    self.in_buffer = ""
    self.last_cmd = CMD.idle
    self._frequency = 0
    self._avg_power =  [float(0)] * 4
    self._power_dbm = 0.0
    self.interval_ms = 100
    self.running = False

    # Event handlers
    self.on_frequency_change = None
    self.on_power_update = None


  async def open(self):
    try:
      self.serial = serial.Serial(self.port_name, self.baudrate, timeout=0)
    except:
      print(f"{self.port_name} not ready")
      raise

    self.running = True
    asyncio.create_task(self._poll_serial())
    asyncio.create_task(self._timer_loop())


  async def close(self):
    self.running = False
    await asyncio.sleep(0.1) 
    if self.serial and self.serial.is_open:
      self.serial.close()


  @property
  def avg_power(self):
    return float(sum(self._avg_power) / len(self._avg_power))


  @property
  def power(self):
    try:
      return 10 ** (self._power_dbm / 10.0)
    except:
        return 0.0

  @property
  def power_dbm(self):
    return self._power_dbm
  

  @property
  def frequency(self):
    return self._frequency


  @frequency.setter
  def frequency(self, value):
    value = max(5650, min(5950, value))
    i = int((value - 5625) / 50)
    if self.serial and self.serial.is_open:
      self.serial.write(f"F{i + 8}\r\n".encode())
      self.last_cmd = CMD.frequency


  async def _timer_loop(self):
    while self.running:
      await asyncio.sleep(self.interval_ms / 1000.0)
      if self.last_cmd == CMD.idle and self.serial and self.serial.is_open:
        self.serial.write(b"D\r\n")
        self.last_cmd = CMD.power


  async def _poll_serial(self):
    while self.running:
      await asyncio.sleep(0.01)
      if self.serial and self.serial.in_waiting > 0:
        data = self.serial.read(self.serial.in_waiting).decode(errors='ignore')
        for byte in data:
          if byte in ['\r', '\n']:
            self._process_line(self.in_buffer.strip())
            self.in_buffer = ""
            self.last_cmd = CMD.idle
          else:
            self.in_buffer += byte


  def _process_line(self, line):
    if self.last_cmd == CMD.frequency:
      try:
        self._frequency = int(line)
      except ValueError:
        self._frequency = 0
      if self.on_frequency_change:
        self.on_frequency_change()
    elif self.last_cmd == CMD.power:
      try:
        mw = 10 ** (float(line) / 10.0)
        self._power_dbm = float(line)
        self._avg_power.pop(len(self._avg_power)-1)
        self._avg_power.insert(0, mw)

      except:
        pass
      if self.on_power_update:
        self.on_power_update()

