#!/usr/bin/python

import sys
import serial

if len(sys.argv) == 0:
    print("Pass a filename to transfer over serial")
    sys.exit()

with open(sys.argv[1], "rb") as f:
    contents = f.read()

port = "/dev/ttyACM0"
if len(sys.argv) > 2:
    port = sys.argv[2]

with serial.Serial(port, 57600) as ser:
    ser.write(str(len(contents)).encode("UTF-8"))
    ser.write("\r".encode("UTF-8"))
    ser.write(contents)
    print("File uploaded")
