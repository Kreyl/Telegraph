/*
 * SnsPins.h
 *
 *  Created on: 17 џэт. 2015 у.
 *      Author: Kreyl
 */

/* ================ Documentation =================
 * There are several (may be 1) groups of sensors (say, buttons and USB connection).
 * There is GPIO and Pin data for every sensor.
 *
 */


#ifndef PINSNSSETTINGS_H_
#define PINSNSSETTINGS_H_

#include "ch.h"
#include "hal.h"
#include "kl_lib.h"

#include "main.h" // App.thd here
#include "evt_mask.h"

#define SIMPLESENSORS_ENABLED   TRUE

#define SNS_POLL_PERIOD_MS      11

// Possible states of pin
enum PinSnsState_t {pssLo, pssHi, pssRising, pssFalling};
// Postprocessor function callback
typedef void (*ftVoidPSnsStLen)(PinSnsState_t *PState, uint32_t Len);

// Single pin setup data
struct PinSns_t {
    GPIO_TypeDef *PGpio;
    uint16_t Pin;
    PinPullUpDown_t Pud;
    ftVoidPSnsStLen Postprocessor;
    void Init() const { PinSetupIn(PGpio, Pin, Pud); }
    void Off()  const { PinSetupAnalog(PGpio, Pin);  }
    bool IsHi() const { return PinIsSet(PGpio, Pin); }
};

// ================================= Settings ==================================
#define BUTTONS_ENABLED FALSE
#if BUTTONS_ENABLED
// Buttons handler
extern void ProcessButtons(PinSnsState_t *PState, uint32_t Len);
#define BUTTONS_CNT     1   // Setup appropriately. Required for buttons handler
#endif

// PinSns Handlers. Add handler for every group.
extern void ProcessKey(PinSnsState_t *PState, uint32_t Len);
extern void ProcessLine(PinSnsState_t *PState, uint32_t Len);
extern void ProcessUsbSns(PinSnsState_t *PState, uint32_t Len);

// List of pins utilized as sensors. Add pins here.
const PinSns_t PinSns[] = {
#if BUTTONS_ENABLED // Buttons
        {GPIOA,  0, pudNone, ProcessButtons},
#endif
        {GPIOA, 4, pudPullUp, ProcessKey},      // Key
        {GPIOA, 0, pudNone,   ProcessLine},     // }
        {GPIOA, 1, pudNone,   ProcessLine},     // } Lines
        {GPIOA, 9, pudPullDown, ProcessUsbSns}, // USB sns
};

// Number of pins used, do not change
#define PIN_SNS_CNT     countof(PinSns)

#endif /* PINSNSSETTINGS_H_ */
