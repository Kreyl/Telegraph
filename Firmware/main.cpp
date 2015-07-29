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
#include "adc_f2.h"

App_t App;
// Outputs
const PinOutput_t PinLine1Tx = {GPIOB, 4, omPushPull};
const PinOutput_t Magnet = {GPIOA, 10, omPushPull};
// Inputs
const PinInput_t PinEcho = {GPIOC, 12, pudPullUp};
#define ECHO_IS_ON()    (!PinEcho.IsHi())
const PinInput_t PinSrcPwrSns = {GPIOC, 14, pudPullDown};
// Leds
const LedOnOff_t LedLine = {GPIOA, 2};
const LedOnOff_t LedSrcPwr = {GPIOC, 15};
const LedOnOff_t LedBtPwr = {GPIOB, 1};
const LedOnOff_t LedBtPwrLo = {GPIOB, 2};
// Other
Beeper_t Beeper;
Motor_t Motor;
PeriodicTmr_t TmrCheck = {MS2ST(702), EVTMSK_CHECK};
Adc_t Adc;

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

    // ==== Pins ====
    // Inputs
    PinEcho.Init();
    PinSrcPwrSns.Init();
    PinSetupAnalog(GPIOB, 0);   // ADC input, battery measurement
    // Outputs
    PinLine1Tx.Init();
    PinLine1Tx.SetLo();
    Magnet.Init();
    Magnet.SetLo();
    // Leds
    LedLine.Init();
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
    PinSensors.Init();
    Adc.Init();

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
            //Uart.Printf("\rCheck");
            if(PinSrcPwrSns.IsHi()) { // External Power
                LedSrcPwr.On();
                LedBtPwr.Off();
                LedBtPwrLo.Off();
                BatteryWasDischarged = false;
            }
            else { // Battery Power
                LedSrcPwr.Off();
                Adc.Measure(chThdSelf(), EVTMSK_ADC_DONE);  // Measure battery
            }
        } // EVTMSK_CHECK

        if(EvtMsk & EVTMSK_ADC_DONE) {
            uint32_t U = Adc.Average();
            U = ADC2MV(U);  // Convert to mV
//            Uart.Printf("\rU=%u", U);
            // Indicate
            if(U <= LOW_BATTERY_MV and !BatteryWasDischarged) {
                BatteryWasDischarged = true;
                LedBtPwr.Off();
                LedBtPwrLo.On();
                Uart.Printf("\rBattery Discharged");
            }
            else if(U >= NORMAL_BATTERY_MV) {
                BatteryWasDischarged = false;
                LedBtPwr.On();
                LedBtPwrLo.Off();
//                Uart.Printf("\rBattery Ok");
            }
            else if(U > LOW_BATTERY_MV and !BatteryWasDischarged) {
                LedBtPwr.On();
                LedBtPwrLo.Off();
//                Uart.Printf("\rBattery Ok");
            }
        } // EVTMSK_ADC_DONE
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
void App_t::OnKeyPress() {
//    Uart.Printf("\rKey Press");
    Beeper.Beep(1975, BEEP_VOLUME);
    // If echo is on, print self and do not transmit to line
    if(ECHO_IS_ON()) {
        WasSelfPrinting = true;
        PrintDot();
    }
    else {
        IsTransmitting = true;
        PinLine1Tx.SetHi();
    }
}
void App_t::OnKeyDepress() {
//    Uart.Printf("\rKey Rel");
    Beeper.Off();
    PinLine1Tx.SetLo();
    if(WasSelfPrinting) {
        WasSelfPrinting = false;
        PrintSpace();
    }
    IsTransmitting = false;
}

void App_t::OnLineShort() {
//    Uart.Printf("\rLine Short");
    LedLine.Off();
    if(!IsTransmitting) {
        Beeper.Beep(999, BEEP_VOLUME);
        PrintDot();
    }
}
void App_t::OnLineRelease() {
//    Uart.Printf("\rLine R");
    LedLine.On();
    Beeper.Off();
    if(!IsTransmitting) {
        PrintSpace();
    }
}

void ProcessKey(PinSnsState_t *PState, uint32_t Len) {
    if(*PState == pssFalling) App.OnKeyPress(); // Key pressed
    else if(*PState == pssRising) App.OnKeyDepress(); // Key released
}

void ProcessLine(PinSnsState_t *PState, uint32_t Len) {
    if(PState[0] == pssFalling) App.OnLineShort();
    else if(PState[0] == pssRising) App.OnLineRelease();
}
#endif

void App_t::PrintDot() {
    if(Motor.IsRunning) { chVTReset(&App.TmrRxTimeout); }
    else Motor.Run();
    Magnet.SetHi();
}
void App_t::PrintSpace() {
    if(Motor.IsRunning) chVTRestart(&App.TmrRxTimeout, MS2ST(RX_TIMEOUT_MS), EVTMSK_RX_TIMEOUT);
    Magnet.SetLo();
}
