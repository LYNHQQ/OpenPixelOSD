import sys
import asyncio
from cImmersionRC import PowerMeter
from cMsp import MSP


if sys.platform.startswith("win"):
  import msvcrt
  async def getch():
    while True:
      if msvcrt.kbhit():
        return msvcrt.getch().decode("utf-8")
      await asyncio.sleep(0.01)
else:
  import tty
  import termios
  import select


  async def getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
      tty.setcbreak(fd)
      while True:
        if select.select([sys.stdin], [], [], 0)[0]:
          return sys.stdin.read(1)
        await asyncio.sleep(0.01)
    finally:
      termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

def map(x, in_min, in_max, out_min, out_max):
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min


async def testPa(power):
  global meter
  msp.power = power
  msp.pitmode = 0
  msp.send_MSP_SET_PACALIBRATION(1)
  print(f"Test PA {power} at {msp.pa_table[power].mW} mW")

  for i in range(7):
    meter.frequency = msp.pa_table[0].value[i]
    msp.frequency = msp.pa_table[0].value[i]
    msp.send_MSP_VTX_CONFIG()
    msp.send_MSP_SET_PACALIBRATION(msp.pa_table[power].value[i])

    for w in range(40):
      await asyncio.sleep(0.1)
      sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {msp.pa_table[power].value[i]} mV  Pwr: {meter.power:03.3f} mW  Det: {msp.detector}    ")
      msp.send_MSP_SET_PACALIBRATION(0)
    
    sys.stdout.write("\n")
    
    msp.send_MSP_SET_PACALIBRATION(1)


async def testPower(power):
  global meter
  msp.power = power
  msp.pitmode = 0
  print(f"Test power {power} at {msp.pa_table[power].mW} mW")

  for i in range(7):
    meter.frequency = msp.pa_table[0].value[i]
    msp.frequency = msp.pa_table[0].value[i]
    msp.send_MSP_VTX_CONFIG()

    stable = False
    stablecounter = 0
    mv_last = msp.mvPa
    while not stable:
      await asyncio.sleep(0.2)
      sys.stdout.write(f"\rfrq: {msp.frequency} MHz  PA: {msp.mvPa} mV  Pwr: {meter.avg_power:03.3f} mW  Det: {msp.detector}    ")
      msp.send_MSP_SET_PACALIBRATION(0)
      if mv_last < msp.mvPa:
        stablecounter = 0
      elif stablecounter > 8:
        stable = True
      else:
        stablecounter += 1
      mv_last = msp.mvPa

    mw = meter.avg_power
    dt = float(msp.detector)
    for w in range(50):
      await asyncio.sleep(0.2)
      mw = 0.95 * mw + 0.05 * meter.avg_power
      dt = 0.95 * dt + 0.05 * msp.detector
      sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {msp.mvPa} mV  Pwr: {mw:03.3f} mW  Det: {int(dt)}    ")
      msp.send_MSP_SET_PACALIBRATION(0)
    
    sys.stdout.write("\n")
    
    msp.send_MSP_SET_PACALIBRATION(1)


async def scanPa(power):
  global meter
  msp.power = power
  msp.pitmode = 0
  msp.send_MSP_SET_PACALIBRATION(1)
  print(f"Scanning PA {power} at {msp.pa_table[power].mW} mW")
  for i in range(7):
    meter.frequency = msp.pa_table[0].value[i]
    msp.frequency = msp.pa_table[0].value[i]
    msp.send_MSP_VTX_CONFIG()
    paval = msp.pa_table[power].value[i]
    paval = paval - 5 if paval > 10 else paval
    msp.send_MSP_SET_PACALIBRATION(paval)
    await asyncio.sleep(0.5)

    while True:
      for r in range(4):
        msp.send_MSP_SET_PACALIBRATION(paval)
        await asyncio.sleep(0.1)
        sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {paval} mV  Pwr: {meter.avg_power:03.3f} mW  Det: {msp.detector}    ")
      
      if (msp.pa_table[power].mW * 0.80 < meter.avg_power):
        break
      
      paval +=50
    
    while True:
      msp.send_MSP_SET_PACALIBRATION(paval)
      await asyncio.sleep(0.21)
      sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {paval} mV  Pwr: {meter.avg_power:03.3f} mW  Det: {msp.detector}    ")
      if (msp.pa_table[power].mW > meter.avg_power):
        break
      paval -=5

    while True:
      for r in range(4):
        msp.send_MSP_SET_PACALIBRATION(paval)
        await asyncio.sleep(0.1)
        sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {paval} mV  Pwr: {meter.avg_power:03.3f} mW  Det: {msp.detector}    ")
      
      if meter.avg_power > msp.pa_table[power].mW:
        break
      paval +=1

    for r in range(25):
        msp.send_MSP_SET_PACALIBRATION(0)
        await asyncio.sleep(0.1)
        sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {paval} mV  Pwr: {meter.avg_power:03.3f} mW  Det: {msp.detector}    ")

    msp.pa_table[power].value[i] = paval
    sys.stdout.write("\n")
    
    msp.send_MSP_SET_PACALIBRATION(1)


async def scanDetector(power):
  global meter
  msp.power = power
  msp.pitmode = 0
  msp.send_MSP_SET_PACALIBRATION(1)
  print(f"Scanning detector {power} at {msp.pa_table[power].mW} mW")
  for i in range(7):
    meter.frequency = msp.pa_table[0].value[i]
    msp.frequency = msp.pa_table[0].value[i]
    msp.send_MSP_VTX_CONFIG()
    paval = msp.pa_table[power].value[i]
    paval = paval - 5 if paval > 10 else paval
    msp.send_MSP_SET_PACALIBRATION(paval)
    await asyncio.sleep(0.5)

    while True:
      msp.send_MSP_SET_PACALIBRATION(paval)
      await asyncio.sleep(0.1)
      sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {paval} mV  Pwr: {meter.avg_power:03.3f} mW  Det: {msp.detector}    ")
      if (msp.pa_table[power].mW > meter.avg_power):
        break
      paval -=1

    avg_pwr = [0] * 3
    avg_det = [0] * 3
    while True:
      avg_pwr[0] = 0
      avg_det[0] = 0
      for r in range(20):
        msp.send_MSP_SET_PACALIBRATION(paval)
        await asyncio.sleep(0.1)
        sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {paval} mV  Pwr: {meter.power:03.3f} mW  Det: {msp.detector}    ")
        if r > 9:
          avg_pwr[0] += meter.avg_power * 0.1
          avg_det[0] += msp.detector * 0.1

      dev = max(0.1, msp.pa_table[power].mW * 0.1)
      if (avg_pwr[0] < (msp.pa_table[power].mW - dev)):
        paval +=2
      elif (avg_pwr[0] > (msp.pa_table[power].mW + dev)):
        paval -=2
      elif (avg_pwr[0] < (msp.pa_table[power].mW)):
        avg_pwr[1] = avg_pwr[0]
        avg_det[1] = avg_det[0]
        paval +=1
      else:
        avg_pwr[2] = avg_pwr[0]
        avg_det[2] = avg_det[0]
        paval -=1
      
      if(avg_pwr[1] > 0 and avg_pwr[2] > 0):
        break
    
    detector = int(map(msp.pa_table[power].mW, avg_pwr[1], avg_pwr[2], avg_det[1], avg_det[2]))
    sys.stdout.write("\n")
    #sys.stdout.write(f"\rFrq: {msp.frequency} MHz  PA: {paval} mV  Pwr: {meter.avg_power:03.3f} mW  Det: {int(detector)}    \n")
    msp.pa_table[power].detector[i] = int(detector)
    sys.stdout.write(f"Pwr: {avg_pwr[1]:03.3f}/{avg_pwr[2]:03.3f} Detector: {int(avg_det[1])}/{int(avg_det[2])}  Det: {int(detector)}    \n")
    
    msp.send_MSP_SET_PACALIBRATION(1)


async def powerMenue(action):
  try:
    sys.stdout.write(f"\nEnter power index (1 to {len(msp.pa_table)}, 0=all): ")
    key = await asyncio.wait_for(getch(), timeout=20.0)
    if key.isdigit():
      power = int(key)
      sys.stdout.write(f" {power}\n\n")
      if (power == 0):
        for i in range(len(msp.pa_table)-1):
          if (action == 0):
            await scanPa(i+1)
          elif (action == 1):
            await scanDetector(i+1)
          elif (action == 2):
            await testPa(i+1)
          elif (action == 3):
            await testPower(i+1)
      elif (power <= len(msp.pa_table)):
        if (action == 0):
          await scanPa(power)
        elif (action == 1):
          await scanDetector(power)
        elif (action == 2):
          await testPa(power)
        elif (action == 3):
          await testPower(power)

  except asyncio.TimeoutError:
    pass


def printPaTable():
  for item in msp.pa_table:
    sys.stdout.write(f'{item.idx},\t {item.mW},\t')
    sys.stdout.write(f' {{ ')
    for r in range(7):
      sys.stdout.write(f'{item.value[r]:04}')
      if r < 6:
        sys.stdout.write(f', ')
      else:
        sys.stdout.write(f' }}\n')

    sys.stdout.write(f'\t\t {{ ')
    for r in range(7):
      sys.stdout.write(f'{item.detector[r]:04}')
      if r < 6:
        sys.stdout.write(f', ')
      else:
        sys.stdout.write(f' }}\n')


async def menue():
  try:
    while True:
      print("")
      print("[ 1 ] Scan PA")
      print("[ 2 ] Scan detector")
      print("[ 3 ] Test PA")
      print("[ 4 ] Test power")
      print("[ 5 ] Print PA table")
      print("[ 6 ] Load PA table from VTX")
      print("[ 7 ] Store PA table to VTX")
      print("[ 8 ] Load PA table from file")
      print("[ 9 ] Store PA table to file")
      print("[ s ] Save EEPROM")
      print("[ q ] Exit")
      key = await asyncio.wait_for(getch(), timeout=60.0)
      if key in ["1","2","3","4"]:
        await powerMenue(int(key) - 1)

      elif key == "5":
        print("")
        printPaTable()

      elif key == "6":
        print("\nLoad PA table")
        msp.send_MSP_VTX_CONFIG()
        await asyncio.sleep(0.5)

      elif key == "7":
        msp.send_MSP_SET_PACALTABLE()
        print("\nSending PA table")

      elif key == "8":
        try:
          filename = input("\nEnter filename: ")
          filename = "pa_table.json" if filename == "" else filename
          msp.pa_table = msp.pa_table.load_json(filename)
          print(f"\nFile {filename} loaded")
        except:
          print("\nFile error")

      elif key == "9":
        try:
          filename = input("\nEnter filename: ")
          filename = "pa_table.json" if filename == "" else filename
          msp.pa_table.save_json(filename)
          print(f"\nFile saved as {filename}")
        except:
          print("\nFile error")

      elif key == "s":
        msp.send_MSP_EEPROM_WRITE()
        print("\nSave EEPROM")

      elif key == "q":
        break

      else:
        pass
  except asyncio.TimeoutError:
    pass


def on_power_update():
  pass
  #sys.stdout.write(f"\rLeistung: {meter.avg_power:.6f} mW ({meter.power:.1f} mW)")
  #sys.stdout.flush()


async def main(serialPortVtx, serialPortMeter):
  global meter, msp

  meter = PowerMeter(port=serialPortMeter, baudrate=115200) 
  msp = MSP(port=serialPortVtx, baudrate=115200)
  
  meter.on_power_update = on_power_update
  try:
    await msp.open()
    await meter.open()
  except:
    return

  meter.frequency = 5800
  meter.interval_ms = 50
  msp.power = 1
  msp.pitmode = 1
  msp.send_MSP_VTX_CONFIG()
  msp.send_MSP_PACALTABLE()
  await asyncio.sleep(1)

  await menue()

  meter.frequency = 5800
  msp.frequency = 5800
  msp.power = 1
  msp.pitmode = 1
  msp.send_MSP_VTX_CONFIG()

  await meter.close()
  await msp.close()
  await asyncio.sleep(1)


if __name__ == "__main__":
  if len(sys.argv) != 3:
      print(f"\nUsage: python {sys.argv[0]} <COMM_PORT_VTX> <COM_PORT_POWER_METER>")
      sys.exit(1)
  asyncio.run(main(sys.argv[1], sys.argv[2]))

