/*
 * usb_serial.cpp
 *
 *  Created on: 05 ����. 2013 �.
 *      Author: G.Kruglov
 */

#include "usb_uart.h"
#include <stdarg.h>
#include "uart.h"
#include "kl_sprintf.h"

#include "main.h"

// MCU-dependent
#if defined STM32F4XX
#include "kl_lib_f40x.h"

#endif

#if 1 // ==== CDC related defines ====
#define CDC_SET_LINE_CODING             0x20
#define CDC_GET_LINE_CODING             0x21
#define CDC_SET_CONTROL_LINE_STATE      0x22

// ==== Line Coding structure ====
struct Linecoding_t {
  uint8_t dwDTERate[4];
  uint8_t bCharFormat;
  uint8_t bParityType;
  uint8_t bDataBits;
} __attribute__((__packed__));

// Line Control bit definitions
#define LC_STOP_1                       0
#define LC_STOP_1P5                     1
#define LC_STOP_2                       2
#define LC_PARITY_NONE                  0
#define LC_PARITY_ODD                   1
#define LC_PARITY_EVEN                  2
#define LC_PARITY_MARK                  3
#define LC_PARITY_SPACE                 4

static Linecoding_t LineCoding = {
  {0x00, 0x96, 0x00, 0x00},             // 38400
  LC_STOP_1, LC_PARITY_NONE, 8
};
#endif

UsbUart_t UsbUart;

#if 1 // ======================== USB events ===================================
static void OnUsbReady() {
//    Uart.PrintfI("\rUsbReady");
    chSysLockFromIsr();
    App.SignalEvtI(EVTMSK_USB_READY);
    chSysUnlockFromIsr();
    Usb.PEpBulkOut->StartOutTransaction();
}

//static void SetLineCoding() {
////    Uart.PrintfI("\r%S", __FUNCTION__);
//}
//static void SetCtrlLineState() {
//    Uart.PrintfI("\r%S", __FUNCTION__);
//    uint16_t w = Usb.SetupReq.wValue;
//    if(w & 0x0001) Uart.Printf("DTR 1\r");
//    else Uart.Printf("DTR 0\r");
//    if(w & 0x0002) Uart.Printf("RTS 1\r");
//    else Uart.Printf("RTS 0\r");
//}

// Handler of non-standard control pkt
EpState_t NonStandardControlRequestHandler(uint8_t **PPtr, uint32_t *PLen) {
//    Uart.PrintfI("\r%S  %A", __FUNCTION__, &Usb.SetupReq, 8, ' ');
//    Uart.PrintfI("\r%A", Usb.Ep0OutBuf, 8, ' ');
    // Check if class type request
    if((Usb.SetupReq.bmRequestType & USB_REQTYPE_TYPEMASK) == USB_REQTYPE_CLASS) {
        switch(Usb.SetupReq.bRequest) {
            case CDC_GET_LINE_CODING:
//                Uart.PrintfI("\rGET_LINE_CODING");
                *PPtr = (uint8_t*)&LineCoding;
                *PLen = sizeof(LineCoding);
                return esInData;
                break;

            case CDC_SET_LINE_CODING:
//                Uart.PrintfI("\rSET_LINE_CODING");
                *PPtr = (uint8_t*)&LineCoding;  // Do not use length in setup pkt
                *PLen = sizeof(LineCoding);
//                Usb.Events.OnDataOUT[0] = SetLineCoding;
                return esOutData;
                break;

            case CDC_SET_CONTROL_LINE_STATE:    // Nothing to do, there are no control lines
//                Uart.PrintfI("\rSET_CTRL_LINE_STATE");
//                SetCtrlLineState();
                return esOutStatus;
                break;
            default: break;
        } // switch
    } // if class
    return esError;
}

void OnDataOUT() {
    //Uart.PrintfI("\rDataOut");
    chSysLockFromIsr();
    App.SignalEvtI(EVTMSK_USB_DATA_OUT);
    chSysUnlockFromIsr();
}
#endif

void UsbUart_t::Init() {
    // Usb events
    Usb.Events.OnCtrlPkt = NonStandardControlRequestHandler;
    Usb.Events.OnReady = OnUsbReady;
    Usb.Events.OnDataOUT[EP_BULK_OUT_INDX] = OnDataOUT;
    // Queues
    chIQInit(&UsbOutQueue, OutQBuf, CDC_OUTQ_SZ, nullptr, NULL);
    Usb.PEpBulkOut->POutQueue = &UsbOutQueue;
    chOQInit(&UsbInQueue, InQBuf, CDC_INQ_SZ, nullptr, NULL);
    // Start reception
    Usb.PEpBulkOut->StartOutTransaction();
}

#if 1 // ==== Cmd ====
ProcessDataResult_t UsbUart_t::ProcessOutData() {
    while(true) {
        msg_t r = chIQGetTimeout(&UsbOutQueue, TIME_IMMEDIATE);
        if(r >= 0) {
            if(Cmd.PutChar(r) == pdrNewCmd) return pdrNewCmd;
        } // if r > 0
        else break; // Queue become empty or inoperable
    }
    return pdrProceed;
}
#endif

#if 1 // ==== Printf ====
static inline void FPutChar(char c) { chOQPut(&UsbUart.UsbInQueue, c); }

void UsbUart_t::Printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    kl_vsprintf(FPutChar, CDC_INQ_SZ, format, args);
    va_end(args);
    // Start transmission
    Usb.PEpBulkIn->StartTransmitQueue(&UsbInQueue);
}
#endif
