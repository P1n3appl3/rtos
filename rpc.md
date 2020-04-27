# RPC Protocol

All commands and return values are string encoded

command: adc
parameter: none
return: adc reading

command: time
parameters: get | reset
return: current time

command: message
parameters: message string
return: none

command: load
parameter: filename
return: none

command: led
parameters: COLOR [on, off, or toggle]
return: none

## filesystem commands

command: mount
command: unmount

command: open
parameter: FILENAME

command: append
parameters: FILENAME word
