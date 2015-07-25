/*
 * main.h
 *
 *  Created on: 30.03.2013
 *      Author: kreyl
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "kl_lib.h"
#include "uart.h"
#include "kl_buf.h"

#include "evt_mask.h"

#define APP_NAME        "Telegraph"
#define APP_VERSION     "2015-07-22_20:28"

#define USB_ENABLED TRUE

// Pins
#define ECHO_GPIO   GPIOC
#define ECHO_PIN    12
#define ECHO_ON()   PinIsSet(ECHO_GPIO, ECHO_PIN)

void TmrGeneralCallback(void *p);

// ==== RX ====
#define RX_MIN_DURTN_MS         27
#define RX_REPORT_PERIOD_MS     360
#define RX_TIMEOUT_MS           999

#if 1 // === Rx Buf ====
struct DotSpace_t {
    uint32_t Dot, Space;
};

template <uint32_t Sz>
class RxBuf_t {
private:
    uint32_t ITime;
    DotSpace_t DS;
    CircBuf_t<DotSpace_t, Sz> DotBuf;
public:
    void OnShort() {
        uint32_t Duration = chTimeNow() - ITime;
        if(Duration > RX_MIN_DURTN_MS) {
            ITime = chTimeNow();
            DS.Space = Duration;
        }
    }
    void OnRelease() {
        uint32_t Duration = chTimeNow() - ITime;
//        Uart.Printf("\rt=%u; it=%u; d=%u", chTimeNow(), ITime, Duration);
        if(Duration > RX_MIN_DURTN_MS) {
            ITime = chTimeNow();
            DS.Dot = Duration;
            DotBuf.Put(&DS);
        }
    }
    uint8_t Get(DotSpace_t *p) { return DotBuf.Get(p); }
    void    Put(DotSpace_t *p) {        DotBuf.Put(p); }
};
#endif

struct Settings_t {
    int32_t BeepInt=0;
    int32_t ConnectBeep=0;
    int32_t RxBeep=0;
    int32_t TxBeep=0;
    int32_t TxLine=0;

};
extern Settings_t Settings;

class App_t {
private:

public:
    void OnUsbCmd();
    VirtualTimer TmrRxReport, TmrRxTimeout;
    RxBuf_t<2007> LineRx1;
    RxBuf_t<207> KeyRx;
    CircBuf_t<DotSpace_t, 2007> UsbRx;
    // Eternal
    Thread *PThread, *PTxThread;
    void InitThread() { PThread = chThdSelf(); }
    void SignalEvt(eventmask_t Evt) {
        chSysLock();
        chEvtSignalI(PThread, Evt);
        chSysUnlock();
    }
    void SignalEvtI(eventmask_t Evt) { chEvtSignalI(PThread, Evt); }
    void OnUartCmd(Uart_t *PUart);
    // Inner use
    void ITask();
};
extern App_t App;

//class Tx_t {
//private:
//
//public:
//
//};

#endif /* MAIN_H_ */
