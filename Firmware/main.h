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


void TmrGeneralCallback(void *p);

// ==== Line RX ====
#define RX_MIN_DURTN_MS     27
#define RX_REPORT_EVERY_MS  360
#define RX_BUF_SZ           45
class LineRx_t {
private:
    uint32_t ITime;
public:
    CircBufNumber_t<uint32_t, RX_BUF_SZ> DotBuf;
    void OnLineShort() { ITime = chTimeNow(); }
    void OnLineRelease() {
        uint32_t Duration = chTimeNow() - ITime;
//        Uart.Printf("\rt=%u; it=%u; d=%u", chTimeNow(), ITime, Duration);
        ITime = chTimeNow();
        if(Duration > RX_MIN_DURTN_MS) DotBuf.Put(Duration);
    }
};

class App_t {
private:

public:
    void OnUsbCmd();
    VirtualTimer ITmrRxReport;
    LineRx_t LineRx1;
    // Eternal
    Thread *PThread;
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

#endif /* MAIN_H_ */
