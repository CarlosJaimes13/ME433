# pico_controls.py
# Flash to Pico
#
#   GP14  →  LEFT  button
#   GP15  →  RIGHT button
#   GP16  →  JUMP  button


import time
from machine import Pin

btn_left  = Pin(14, Pin.IN, Pin.PULL_UP)
btn_right = Pin(15, Pin.IN, Pin.PULL_UP)
btn_jump  = Pin(16, Pin.IN, Pin.PULL_UP)

while True:
    left  = 0 if btn_left.value()  else 1
    right = 0 if btn_right.value() else 1
    jump  = 0 if btn_jump.value()  else 1

    # Send format that puffle_rescue.py expects: "(left,right,jump)"
    print("({},{},{})".format(left, right, jump))

    time.sleep(1 / 30)   # 30 updates per second