# Read roll, pitch, yaw data and send to FDAI driver board via the serial port
# This code uses data from a MAV log file
# The tricky part is ensuring that elapsed wall time matches the timestamps in the input
#
# Sample execution:
# python mavlogdump.py 00000018.BIN | python sendRPY.py

import datetime
import glob
import os
import re
import serial
import sys
import time

usb = glob.glob("/dev/tty*usb*")
port = usb[0]
print("Opening", port)

ser = serial.Serial(port, 115200, timeout=1) 

t0 = None # Initial timestamp of input data
ts = None # Initial wall time
for line in sys.stdin:
  # Parse the log line, extracting the timestamp, roll, pitch, and yaw
  m = re.search(r'(\S+ \S+):.*Roll : (\S+). Pitch : (\S+), Yaw : ([^,])', line)
  if m:
    t, r, p, y = m.groups()
    r = float(r)
    p = float(p)
    y = float(y)
    ts = datetime.datetime.strptime(t, "%Y-%m-%d %H:%M:%S.%f")

    # Initialize on first line
    if t0 is None:
      t0 = ts
      wallt0 = time.time()

    # Drop data or delay as necessary so data is sent at the proper elapsed time
    dtime = (ts-t0).total_seconds() # Delta log time: time since start of log
    dwalltime = time.time() - wallt0 # Delta elapsed time: time since start of execution
    if dtime + .5 < dwalltime: # Allow half a second of slack before dropping data
      print("Dropping", t, r, p, y)
      continue
    if dtime > dwalltime: # Delay to send data at the right time
      time.sleep(dtime - dwalltime)

    print(t, r, p, y)
    # Send data to the FDAI board and wait for a response
    ser.write((">%.2f %.2f %.2f\n" % (r, p, y)).encode()) # Send roll, pitch, and yaw, with > starting the line
    while 1:
      if ser.inWaiting() > 0:
        data = ser.read(ser.inWaiting()).decode()
        print(data.strip())
        break
