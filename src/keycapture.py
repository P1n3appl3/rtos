#!/usr/bin/python

import sys
import serial
import select
import curses

port = "/dev/ttyACM0"
if len(sys.argv) > 1:
    port = sys.argv[2]

screen = curses.initscr()
screen.refresh()

with serial.Serial(port, 9600) as ser:
    while True:
        a = screen.getkey()
        ser.write(a.encode('utf-8'))
        if a == 'q':
            break
curses.endwin()
