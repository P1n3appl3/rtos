#include "mouse.h"
#include "OS.h"
#include "printf.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/usblib.h"

const uint8_t LangDescriptor[] = {4, USB_DTYPE_STRING,
                                  USBShort(USB_LANG_EN_US)};
const uint8_t ManufacturerString[] = {
    (4 + 1) * 2, USB_DTYPE_STRING, 'R', 0, 'T', 0, 'O', 0, 'S', 0};
const uint8_t ProductString[] = {
    (4 + 1) * 2, USB_DTYPE_STRING, 'L', 0, 'a', 0, 'b', 0, '7', 0};
const uint8_t SerialNumberString[] = {(8 + 1) * 2, USB_DTYPE_STRING,
                                      '1',         0,
                                      '2',         0,
                                      '3',         0,
                                      '4',         0,
                                      '5',         0,
                                      '6',         0,
                                      '7',         0,
                                      '8',         0};
const uint8_t HIDInterfaceString[] =
    "(\x03H\0I\0D\0 \0M\0o\0u\0s\0e\0 \0I\0n\0t\0e\0r\0f\0a\0c\0e";

const uint8_t ConfigString[] =
    "0\x03H\0I\0D\0 \0M\0o\0u\0s\0e\0 \0C\0o\0n\0f\0i\0g\0u\0r\0a\0t\0i\0o\0n";

#define NUM_STRING_DESCRIPTORS (sizeof(StringDescriptors) / sizeof(uint8_t*))

const uint8_t* const StringDescriptors[] = {
    LangDescriptor,     ManufacturerString, ProductString,
    SerialNumberString, HIDInterfaceString, ConfigString};

static Sema4 mouse_ready;
uint32_t mouse_handler(void* pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
                       void* pvMsgData) {
    switch (ui32Event) {
    case USB_EVENT_CONNECTED: {
        printf("\nHost Connected...\r\n");
        OS_Signal(&mouse_ready);
        break;
    }
    case USB_EVENT_DISCONNECTED: {
        printf("\nHost Disconnected...\r\n");
        break;
    }
    case USB_EVENT_TX_COMPLETE: {
        // printf("TX complete\n\r");
        OS_Signal(&mouse_ready);
        break;
    }
    case USB_EVENT_SUSPEND: {
        OS_Signal(&mouse_ready);
        printf("\nBus Suspended\n\r");
        break;
    }
    case USB_EVENT_RESUME: {
        OS_Signal(&mouse_ready);
        printf("\nBus Resume\n\r");
        break;
    }
        // We ignore all other events.
    default: {
        break;
    }
    }
    return 0;
}

tUSBDHIDMouseDevice mouse_dev = {
    // The Vendor ID you have been assigned by USB-IF.
    .ui16VID = 0xFFFF,
    // The product ID you have assigned for this device.
    .ui16PID = 0xFFFF,
    // The power consumption of your device in milliamps.
    .ui16MaxPowermA = 0,
    .ui8PwrAttributes = USB_CONF_ATTR_SELF_PWR,
    // A pointer to your mouse callback event handler.
    .pfnCallback = mouse_handler,
    // A value that you want passed to the callback alongside every event.
    .pvCBData = (void*)&mouse_dev,
    // A pointer to your string table.
    .ppui8StringDescriptors = StringDescriptors,
    // The number of entries in your string table. This must equal
    // (1 + (5 * (num languages))).
    .ui32NumStringDescriptors = NUM_STRING_DESCRIPTORS};

typedef enum { LEFT = 1, RIGHT, MIDDLE } MouseButton;

static bool held_down[3] = {false, false, false};
void mouse_toggle(MouseButton button) {
    --button;
    OS_Wait(&mouse_ready);
    USBDHIDMouseStateChange(&mouse_dev, 0, 0,
                            held_down[button] ? 0 : 1 << button);
    held_down[button] = !held_down[button];
}

void mouse_click(MouseButton button) {
    mouse_toggle(button), mouse_toggle(button);
}

void mouse_move(int8_t x, int8_t y) {
    OS_Wait(&mouse_ready);
    USBDHIDMouseStateChange(&mouse_dev, x, y, 0);
}

static char continuous_dir = ' ';
void mouse_continuous(void) {
    const uint8_t s = 3;
    const int frequency = hz(60);
    while (true) {
        OS_Sleep(frequency);
        switch (continuous_dir) {
        case 'Q': mouse_move(-s, -s); break;
        case 'W': mouse_move(0, -s); break;
        case 'E': mouse_move(s, -s); break;
        case 'A': mouse_move(-s, 0); break;
        case 'D': mouse_move(s, 0); break;
        case 'Z': mouse_move(-s, s); break;
        case 'X': mouse_move(0, s); break;
        case 'C': mouse_move(s, s); break;
        }
    }
}

void mouse_center(void) {
    continuous_dir = ' ';
    const uint16_t width = 1920;
    const uint16_t height = 1080;
    for (int i = 0; i < width / 100; ++i) { mouse_move(-100, 0); }
    for (int i = 0; i < width / 100 / 2; ++i) { mouse_move(100, 0); }
    for (int i = 0; i < height / 100; ++i) { mouse_move(0, -100); }
    for (int i = 0; i < height / 100 / 2; ++i) { mouse_move(0, 100); }
    for (int i = 0; i < 3; ++i) {
        if (held_down[i]) {
            mouse_toggle(i + 1);
        }
    }
}

bool mouse_cmd(char c) {
    const int8_t s = 10;
    switch (c) {
    case 127: return false;
    case ',': mouse_click(LEFT); break;
    case '.': mouse_click(MIDDLE); break;
    case '/': mouse_click(RIGHT); break;
    case '<': mouse_toggle(LEFT); break;
    case '>': mouse_toggle(MIDDLE); break;
    case '?': mouse_toggle(RIGHT); break;
    case ' ': mouse_center(); break;
    case 'q': mouse_move(-s, -s); break;
    case 'w': mouse_move(0, -s); break;
    case 'e': mouse_move(s, -s); break;
    case 'a': mouse_move(-s, 0); break;
    case 'd': mouse_move(s, 0); break;
    case 'z': mouse_move(-s, s); break;
    case 'x': mouse_move(0, s); break;
    case 'c': mouse_move(s, s); break;
    case 's':
    case 'Q':
    case 'W':
    case 'E':
    case 'A':
    case 'S':
    case 'D':
    case 'Z':
    case 'X':
    case 'C': continuous_dir = c; break;
    }
    return true;
}

void mouse_init(void) {
    OS_InitSemaphore(&mouse_ready, -1);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    if (!USBDHIDMouseInit(0, &mouse_dev)) {
        printf("error: hidmouseinit\n\r");
    }
    OS_Wait(&mouse_ready);
    OS_AddThread(mouse_continuous, "Continuous mouse movement", 512, 1);
    OS_Sleep(ms(1500));
}
