# Generate test data for sendRPY.py
# This does a 360 degree cycle of yaw, roll, and pitch
# taking 10 seconds for each.
# Sample execution:
# python simulateLog.py | python sendRPY.py

import datetime

start = datetime.datetime.now().replace(microsecond=0)
def write(t, r, p, y):
  ts = start + datetime.timedelta(seconds=t)
  print("%s: Roll : %.2f, Pitch : %.2f, Yaw : %.2f" % (ts.strftime("%Y-%m-%d %H:%M:%S.%f"), r, p, y))

r = 0
p = 0
y = 0
t = 0

DURATION = 10 # Cycle duration in seconds
STEP = 10 # Number of steps per second
for i in range(0, DURATION * STEP):
  # Yaw over 10 seconds
  y = i / DURATION / STEP * 360
  write(t, r, p, y)
  t += 1 / STEP
y = 0
for i in range(0, DURATION * STEP):
  # Roll over 10 seconds
  r = i / DURATION / STEP * 360
  write(t, r, p, y)
  t += 1 / STEP
r = 0
for i in range(0, DURATION * STEP):
  # Pitch over 10 seconds
  p = i / DURATION / STEP* 360
  write(t, r, p, y)
  t += 1 / STEP

write(t, 0, 0, 0)
