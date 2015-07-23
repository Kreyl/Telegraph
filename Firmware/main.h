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

#include "evt_mask.h"

#define APP_NAME        "Telegraph"
#define APP_VERSION     "2015-07-22_20:28"

#define USB_ENABLED TRUE

// Timings

void TmrGeneralCallback(void *p);

class App_t {
private:
    VirtualTimer ITmr;
public:
    void OnUsbCmd();
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
