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

App_t App;
Beeper_t Beeper;
PinOutputPushPull_t Line1TxPin(GPIOB, 4);
PinOutputPushPull_t Magnet(GPIOA, 10);
PinInput_t<pudPullUp> EchoPin(GPIOC, 12);
Motor_t Motor;

#if 1 // ============= Timers ===================
// Universal VirtualTimer callback
void TmrGeneralCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI((eventmask_t)p);
    chSysUnlockFromIsr();
}
#endif

int main() {
    // ==== Setup clock ====
    uint8_t ClkResult = FAILURE;
    Clk.SetupFlashLatency(12);  // Setup Flash Latency for clock in MHz
    // 12 MHz/6 = 2; 2*192 = 384; 384/8 = 48 (preAHB divider); 384/8 = 48 (USB clock)
//    Clk.SetupPLLDividers(6, 192, pllSysDiv8, 8);
    // 48/4 = 12 MHz core clock. APB1 & APB2 clock derive on AHB clock
    Clk.SetupBusDividers(ahbDiv1, apbDiv1, apbDiv1);
//    if((ClkResult = Clk.SwitchToPLL()) == 0) Clk.HSIDisable();
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
    Line1TxPin.Init();
    Line1TxPin.SetLo();

    Magnet.Init();
    Magnet.SetLo();

    EchoPin.Init();

    Motor.Init();

    Beeper.Init();
    Beeper.StartSequence(bsqBeepBeep);
    chThdSleepMilliseconds(720);

    // ==== Main cycle ====
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);

        if(EvtMsk & EVTMSK_RX_TIMEOUT) {
            Uart.Printf("\rStop");
            Motor.Stop();
        } // EVTMSK_RX_TIMEOUT

        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnUartCmd(&Uart);
            Uart.SignalCmdProcessed();
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
