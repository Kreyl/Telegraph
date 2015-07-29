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
#define APP_VERSION     "2015-07-26_1024"

void TmrGeneralCallback(void *p);

#define RX_TIMEOUT_MS       999
#define NORMAL_BATTERY_MV   10500
#define LOW_BATTERY_MV      9000
#define ADC2MV(Uadc)        ((Uadc * 3544) / 1000)

class App_t {
private:
    bool BatteryWasDischarged = false;
    bool WasSelfPrinting = false;
    bool IsTransmitting = false;
public:
    VirtualTimer TmrRxTimeout;
    // Printing
    void PrintDot();
    void PrintSpace();
    // Inputs
    void OnKeyPress();
    void OnKeyDepress();
    void OnLineShort();
    void OnLineRelease();
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
