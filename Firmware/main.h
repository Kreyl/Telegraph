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

// ==== Pins ====
#define ECHO_GPIO   GPIOC
#define ECHO_PIN    12
#define ECHO_ON()   PinIsSet(ECHO_GPIO, ECHO_PIN)

void TmrGeneralCallback(void *p);

// ==== Timings ====
#define RX_TIMEOUT_MS           999

class App_t {
private:

public:
    VirtualTimer TmrRxTimeout;
    // Printing
    void OnPress();
    void OnDepress();
    bool WasSelfPrinting = false;
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
