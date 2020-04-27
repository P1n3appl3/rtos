// can0.c
// Runs on LM4F120/TM4C123
// Use CAN0 to communicate on CAN bus PE4 and PE5
//

// Jonathan Valvano
// May 2, 2015

/* This example accompanies the books
   Embedded Systems: Real-Time Operating Systems for ARM Cortex-M
 Microcontrollers, Volume 3, ISBN: 978-1466468863, Jonathan Valvano, copyright
 (c) 2015

   Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers,
 Volume 2 ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// MCP2551 Pin1 TXD  ---- CAN0Tx PE5 (8) O TTL CAN module 0 transmit
// MCP2551 Pin2 Vss  ---- ground
// MCP2551 Pin3 VDD  ---- +5V with 0.1uF cap to ground
// MCP2551 Pin4 RXD  ---- CAN0Rx PE4 (8) I TTL CAN module 0 receive
// MCP2551 Pin5 VREF ---- open (it will be 2.5V)
// MCP2551 Pin6 CANL ---- to other CANL on network
// MCP2551 Pin7 CANH ---- to other CANH on network
// MCP2551 Pin8 RS   ---- ground, Slope-Control Input (maximum slew rate)
// 120 ohm across CANH, CANL on both ends of network
#include "can0.h"
#include "interrupts.h"
#include "tivaware/can.h"
#include "tivaware/gpio.h"
#include "tivaware/hw_can.h"
#include "tivaware/hw_gpio.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_memmap.h"
#include "tivaware/hw_types.h"
#include "tivaware/pin_map.h"
#include "tivaware/rom.h"
#include "tivaware/sysctl.h"
#include <stdint.h>

// reverse these IDs on the other microcontroller

uint32_t RCV_ID = 2; // set dynamically at time of CAN0_Open
uint32_t XMT_ID = 4;

#define NULL 0
// reverse these IDs on the other microcontroller

// Mailbox linkage from background to foreground
uint8_t static RCVData[4];
int static MailFlag;

//*****************************************************************************
//
// The CAN controller interrupt handler.
//
//*****************************************************************************
void can0_handler(void) {
    uint8_t data[4];
    uint32_t ulIntStatus, ulIDStatus;
    int i;
    tCANMsgObject xTempMsgObject;
    xTempMsgObject.pui8MsgData = data;
    ulIntStatus = ROM_CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE); // cause?
    if (ulIntStatus & CAN_INT_INTID_STATUS) {                     // receive?
        ulIDStatus = ROM_CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT);
        for (i = 0; i < 32; i++) {         // test every bit of the mask
            if ((0x1 << i) & ulIDStatus) { // if active, get data
                ROM_CANMessageGet(CAN0_BASE, (i + 1), &xTempMsgObject, true);
                if (xTempMsgObject.ui32MsgID == RCV_ID) {
                    RCVData[0] = data[0];
                    RCVData[1] = data[1];
                    RCVData[2] = data[2];
                    RCVData[3] = data[3];
                    MailFlag = true; // new mail
                }
            }
        }
    }
    ROM_CANIntClear(CAN0_BASE, ulIntStatus); // acknowledge
}

// Set up a message object.  Can be a TX object or an RX object.
void static CAN0_Setup_Message_Object(uint32_t MessageID, uint32_t MessageFlags,
                                      uint32_t MessageLength,
                                      uint8_t* MessageData, uint32_t ObjectID,
                                      tMsgObjType eMsgType) {
    tCANMsgObject xTempObject;
    xTempObject.ui32MsgID = MessageID; // 11 or 29 bit ID
    xTempObject.ui32MsgLen = MessageLength;
    xTempObject.pui8MsgData = MessageData;
    xTempObject.ui32Flags = MessageFlags;
    ROM_CANMessageSet(CAN0_BASE, ObjectID, &xTempObject, eMsgType);
}
// Initialize CAN port
void CAN0_Open(uint32_t rcvId, uint32_t xmtID) {
    RCV_ID = rcvId;
    XMT_ID = xmtID;
    MailFlag = false;

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) {}
    ROM_GPIOPinConfigure(GPIO_PE4_CAN0RX);
    ROM_GPIOPinConfigure(GPIO_PE5_CAN0TX);
    ROM_GPIOPinTypeCAN(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    ROM_CANInit(CAN0_BASE);
    ROM_CANBitRateSet(CAN0_BASE, 80000000, CAN_BITRATE);
    ROM_CANEnable(CAN0_BASE);
    // make sure to enable STATUS interrupts
    ROM_CANIntEnable(CAN0_BASE,
                     CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
    // Set up filter to receive these IDs
    // in this case there is just one type, but you could accept multiple ID
    // types
    CAN0_Setup_Message_Object(RCV_ID, MSG_OBJ_RX_INT_ENABLE, 4, NULL, RCV_ID,
                              MSG_OBJ_TYPE_RX);
    ROM_IntEnable(INT_CAN0);
    return;
}

// send 4 bytes of data to other microcontroller
void CAN0_SendData(uint8_t data[4]) {
    // in this case there is just one type, but you could accept multiple ID
    // types
    CAN0_Setup_Message_Object(XMT_ID, NULL, 4, data, XMT_ID, MSG_OBJ_TYPE_TX);
}

// Returns true if receive data is available
//         false if no receive data ready
int CAN0_CheckMail(void) {
    return MailFlag;
}
// if receive data is ready, gets the data and returns true
// if no receive data is ready, returns false
int CAN0_GetMailNonBlock(uint8_t data[4]) {
    if (MailFlag) {
        data[0] = RCVData[0];
        data[1] = RCVData[1];
        data[2] = RCVData[2];
        data[3] = RCVData[3];
        MailFlag = false;
        return true;
    }
    return false;
}
// if receive data is ready, gets the data
// if no receive data is ready, it waits until it is ready
void CAN0_GetMail(uint8_t data[4]) {
    while (MailFlag == false) {};
    data[0] = RCVData[0];
    data[1] = RCVData[1];
    data[2] = RCVData[2];
    data[3] = RCVData[3];
    MailFlag = false;
}
