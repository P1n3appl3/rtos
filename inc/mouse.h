#pragma once
#include "usblib/usbhid.h"

void mouse_init(void);
void mouse_action(void);
bool mouse_cmd(char c);
