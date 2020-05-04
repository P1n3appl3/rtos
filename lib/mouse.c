#include "mouse.h"
#include "OS.h"
#include "printf.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include "usblib/device/usbdhidmouse.h"
#include "usblib/usblib.h"

volatile bool ready = false;

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
const uint8_t HIDInterfaceString[] = {(19 + 1) * 2, USB_DTYPE_STRING,
                                      'H',          0,
                                      'I',          0,
                                      'D',          0,
                                      ' ',          0,
                                      'M',          0,
                                      'o',          0,
                                      'u',          0,
                                      's',          0,
                                      'e',          0,
                                      ' ',          0,
                                      'I',          0,
                                      'n',          0,
                                      't',          0,
                                      'e',          0,
                                      'r',          0,
                                      'f',          0,
                                      'a',          0,
                                      'c',          0,
                                      'e',          0};
const uint8_t ConfigString[] = {(23 + 1) * 2, USB_DTYPE_STRING,
                                'H',          0,
                                'I',          0,
                                'D',          0,
                                ' ',          0,
                                'M',          0,
                                'o',          0,
                                'u',          0,
                                's',          0,
                                'e',          0,
                                ' ',          0,
                                'C',          0,
                                'o',          0,
                                'n',          0,
                                'f',          0,
                                'i',          0,
                                'g',          0,
                                'u',          0,
                                'r',          0,
                                'a',          0,
                                't',          0,
                                'i',          0,
                                'o',          0,
                                'n',          0};
#define NUM_STRING_DESCRIPTORS (sizeof(StringDescriptors) / sizeof(uint8_t*))

const uint8_t* const StringDescriptors[] = {
    LangDescriptor,     ManufacturerString, ProductString,
    SerialNumberString, HIDInterfaceString, ConfigString};

uint32_t mouse_handler(void* pvCBData, uint32_t ui32Event, uint32_t ui32MsgData,
                       void* pvMsgData) {
    switch (ui32Event) {
    case USB_EVENT_CONNECTED: {
        printf("\nHost Connected...\r\n");
        ready = true;
        break;
    }
    case USB_EVENT_DISCONNECTED: {
        printf("\nHost Disconnected...\r\n");
        ready = false;
        break;
    }
    case USB_EVENT_TX_COMPLETE: {
        printf("TX complete\n\r");
        ready = true;
        break;
    }
    case USB_EVENT_SUSPEND: {
        printf("\nBus Suspended\n\r");
        ready = false;
        break;
    }
    case USB_EVENT_RESUME: {
        printf("\nBus Resume\n\r");
        ready = true;
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

void mouse_action(void) {
    while (!ready) {}
    ready = false;
    USBDHIDMouseStateChange(&mouse_dev, 40, 40, MOUSE_REPORT_BUTTON_1);
    while (!ready) {}
    USBDHIDMouseStateChange(&mouse_dev, 40, 40, 0);
}

void mouse_init(void) {
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    if (!USBDHIDMouseInit(0, &mouse_dev)) {
        printf("error: hidmouseinit\n\r");
    }
    while (!ready) {}
    OS_Sleep(ms(1500));

    mouse_action();
    // USBDHIDMouseTerm(&mouse_dev);
    OS_Kill();
}
