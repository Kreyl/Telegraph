/*
 * File:   main.cpp
 * Author: Kreyl
 * Project: klNfcF0
 *
 * Created on May 27, 2011, 6:37 PM
 */

#include "ch.h"
#include "hal.h"
#include "main.h"
#include "buttons.h"
#include "SimpleSensors.h"
#include "beeper.h"
#include "Sequences.h"
#include "motor.h"
#include "led.h"

App_t App;
Beeper_t Beeper;
const PinOutput_t Line1TxPin = {GPIOB, 4, omPushPull};
const PinOutput_t Magnet = {GPIOA, 10, omPushPull};
const PinInput_t EchoPin = {GPIOC, 12, pudPullUp};
const LedOnOff_t Led1 = {GPIOA, 2};
const LedOnOff_t LedSrcPwr = {GPIOC, 15};
const LedOnOff_t LedBtPwr = {GPIOB, 1};
const LedOnOff_t LedBtPwrLo = {GPIOB, 2};
Motor_t Motor;
PeriodicTmr_t TmrCheck = {MS2ST(702), EVTMSK_CHECK};

#if 1 // ============= Timers ===================
// Universal VirtualTimer one-shot callback
void TmrOneShotCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI((eventmask_t)p);
    chSysUnlockFromIsr();
}
void TmrPeriodicCallback(void *p) { ((PeriodicTmr_t*)p)->CallbackHandler(); }
#endif

int main() {
    // ==== Setup clock ==== Quartz 12MHz
    uint8_t ClkResult = FAILURE;
    Clk.SetupFlashLatency(12);  // Setup Flash Latency for clock in MHz
    Clk.SetupBusDividers(ahbDiv1, apbDiv1, apbDiv1);
    if((ClkResult = Clk.SwitchToHSE()) == OK) Clk.HSIDisable();
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    App.InitThread();
    Uart.Init(115200);
    Uart.Printf("\r%S_%S", APP_NAME, APP_VERSION);
    Clk.PrintFreqs();
    // Report problem with clock if any
    if(ClkResult) Uart.Printf("Clock failure\r");

    PinSensors.Init();
    PinSetupIn(ECHO_GPIO, ECHO_PIN, pudPullUp); // Echo on/off

    // Pins
    Line1TxPin.Init();
    Line1TxPin.SetLo();
    Magnet.Init();
    Magnet.SetLo();
    EchoPin.Init();
    Led1.Init();
    LedSrcPwr.Init();
    LedBtPwr.Init();
    LedBtPwrLo.Init();

    // Timers
    TmrCheck.PThread = chThdSelf();
    TmrCheck.Start();

    chThdSleepMilliseconds(540);    // Let power to stabilize
    Motor.Init();
    Beeper.Init();
    Beeper.StartSequence(bsqBeepBeep);

    // ==== Main cycle ====
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);

        if(EvtMsk & EVTMSK_RX_TIMEOUT) {
//            Uart.Printf("\rStop");
            Motor.Stop();
        } // EVTMSK_RX_TIMEOUT

        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnUartCmd(&Uart);
            Uart.SignalCmdProcessed();
        }

        if(EvtMsk & EVTMSK_CHECK) {
            Uart.Printf("\rCheck");
        }
    } // while true
}

void App_t::OnUartCmd(Uart_t *PUart) {
    UartCmd_t *PCmd = &PUart->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("\r%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PUart->Ack(OK);

    else PUart->Ack(CMD_UNKNOWN);
}

#if 1 // ==== Pin Sns handlers ====
void ProcessKey(PinSnsState_t *PState, uint32_t Len) {
    if(*PState == pssFalling) { // Key pressed
//        Uart.Printf("\rKey Press");
        Beeper.Beep(1975, BEEP_VOLUME);
        // Print self if needed
        if(EchoPin.IsHi()) {
            App.WasSelfPrinting = true;
            App.OnPress();
        }
    }
    else if(*PState == pssRising) { // Key released
//        Uart.Printf("\rKey Rel");
        Beeper.Off();
        if(App.WasSelfPrinting) {
            App.WasSelfPrinting = false;
            App.OnDepress();
        }
    }
}

void ProcessLine(PinSnsState_t *PState, uint32_t Len) {
    if(PState[0] == pssFalling) { // Key pressed
//        Uart.Printf("\rLine Short");
        Beeper.Beep(999, BEEP_VOLUME);
        App.OnPress();
    }
    else if(PState[0] == pssRising) { // Key released
//        Uart.Printf("\rLine R");
        Beeper.Off();
        App.OnDepress();
    }
}
#endif

void App_t::OnPress() {
    if(Motor.IsRunning) { chVTReset(&App.TmrRxTimeout); }
    else Motor.Run();
    Magnet.SetHi();
}
void App_t::OnDepress() {
    if(Motor.IsRunning) chVTRestart(&App.TmrRxTimeout, MS2ST(RX_TIMEOUT_MS), EVTMSK_RX_TIMEOUT);
    Magnet.SetLo();
}
