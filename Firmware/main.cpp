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
#include "usb_f2_4.h"
#include "usb_uart.h"
#include "beeper.h"
#include "Sequences.h"
#include "motor.h"

App_t App;
Beeper_t Beeper;
PinOutputPushPull_t Line1TxPin(GPIOB, 4);
Settings_t Settings;
Motor_t Motor;

#if 1 // ============= Timers ===================
#define StartReportTmr()    chVTStartIfNotStarted(&App.ITmrRxReport, MS2ST(RX_REPORT_PERIOD_MS), EVTMSK_RX_REPORT)
// Universal VirtualTimer callback
void TmrGeneralCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI((eventmask_t)p);
    chSysUnlockFromIsr();
}
//void TmrReportCallback(void *p) {
//    chSysLockFromIsr();
//    App.SignalEvtI(EVTMSK_RX_REPORT);
//    chVTSetI(&App.ITmrRxReport, MS2ST(RX_REPORT_PERIOD_MS), TmrReportCallback, nullptr);
//    chSysUnlockFromIsr();
//}
#endif

static WORKING_AREA(waTxThread, 128);
__attribute__((__noreturn__))
static void TxThread(void *arg);

int main() {
    // ==== Setup clock ====
    Clk.UpdateFreqValues();
    uint8_t ClkResult = FAILURE;
    Clk.SetupFlashLatency(48);  // Setup Flash Latency for clock in MHz
    // 12 MHz/6 = 2; 2*192 = 384; 384/8 = 48 (preAHB divider); 384/8 = 48 (USB clock)
    Clk.SetupPLLDividers(6, 192, pllSysDiv8, 8);
    // 48/4 = 12 MHz core clock. APB1 & APB2 clock derive on AHB clock
    Clk.SetupBusDividers(ahbDiv1, apbDiv4, apbDiv4);
    if((ClkResult = Clk.SwitchToPLL()) == 0) Clk.HSIDisable();
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

    // TX thread
    App.PTxThread = chThdCreateStatic(waTxThread, sizeof(waTxThread), LOWPRIO, (tfunc_t)TxThread, NULL);

    // Timers
//    chVTSet(&App.ITmrRxReport, MS2ST(RX_REPORT_PERIOD_MS), TmrReportCallback, nullptr);

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
        __attribute__((unused))
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);

        if(EvtMsk & EVTMSK_RX_REPORT) {
            DotSpace_t DS;
            // Line RX
            while(LineRx1.Get(&DS) == OK) {
                UsbUart.Printf("\r#RX %u %u", DS.Space, DS.Dot);
                Uart.Printf("\r#RX %u %u", DS.Space, DS.Dot);
            }
            // Key RX
            while(KeyRx.Get(&DS) == OK) {
                UsbUart.Printf("\r#RX %u %u", DS.Space, DS.Dot);
//                Uart.Printf("\r#RX %u %u", DS.Space, DS.Dot);
            }
            chThdSleepMicroseconds(108);
        }

#if 1   // ==== Uart cmd ====
        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnUartCmd(&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

#if USB_ENABLED // ==== USB connection ====
        if(EvtMsk & EVTMSK_USB_CONNECTED) {
//            chSysLock();
//            Clk.SetFreq48Mhz();
//            Clk.InitSysTick();
//            chSysUnlock();
            Usb.Init();
            UsbUart.Init();
            chThdSleepMilliseconds(540);
            Usb.Connect();
            Uart.Printf("\rUsb connected");
            Clk.PrintFreqs();
            Beeper.StartSequence(bsqBeepBeep);
        }
        if(EvtMsk & EVTMSK_USB_DISCONNECTED) {
            Usb.Shutdown();
//            chSysLock();
//            Clk.SetFreq12Mhz();
//            Clk.InitSysTick();
//            chSysUnlock();
            Uart.Printf("\rUsb disconnected, AHB freq=%uMHz", Clk.AHBFreqHz/1000000);
        }
        if(EvtMsk & EVTMSK_USB_DATA_OUT) {
            while(UsbUart.ProcessOutData() == pdrNewCmd) OnUsbCmd();
        }
#endif

    } // while true
}

void App_t::OnUsbCmd() {
    UsbCmd_t *PCmd = &UsbUart.Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("\r%S", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) UsbUart.Ack(OK);

    else if(PCmd->NameIs("#morse")) {
        if(Settings.BeepInt != 0) Beeper.StartSequence(bsqBeep);
        UsbUart.Ack(OK);
    }
    else if(PCmd->NameIs("#uartBeep")) {
        if(PCmd->GetNextToken() != OK) { UsbUart.Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { UsbUart.Ack(CMD_ERROR); return; }
        Settings.BeepInt = dw32;
        UsbUart.Ack(OK);
    }

    else if(PCmd->NameIs("#connectBeep")) {
        if(PCmd->GetNextToken() != OK) { UsbUart.Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { UsbUart.Ack(CMD_ERROR); return; }
        Settings.ConnectBeep = dw32;
        UsbUart.Ack(OK);
    }
    else if(PCmd->NameIs("#rxBeep")) {
        if(PCmd->GetNextToken() != OK) { UsbUart.Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { UsbUart.Ack(CMD_ERROR); return; }
        Settings.RxBeep = dw32;
        UsbUart.Ack(OK);
    }
    else if(PCmd->NameIs("#txBeep")) {
        if(PCmd->GetNextToken() != OK) { UsbUart.Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { UsbUart.Ack(CMD_ERROR); return; }
        Settings.TxBeep = dw32;
        UsbUart.Ack(OK);
    }
    else if(PCmd->NameIs("#Line")) {
        if(PCmd->GetNextToken() != OK) { UsbUart.Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { UsbUart.Ack(CMD_ERROR); return; }
        if(dw32 < 1 or dw32 > 2) { UsbUart.Ack(CMD_ERROR); return; }
        Settings.TxLine = dw32;
        UsbUart.Ack(OK);
    }

    else if(PCmd->NameIs("#tx")) {
        DotSpace_t DS;
        while(PCmd->GetNextToken() == OK) {
            if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { UsbUart.Ack(CMD_ERROR); return; }
            DS.Space = dw32;
            if(PCmd->GetNextToken() != OK) { UsbUart.Ack(CMD_ERROR); return; }
            if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { UsbUart.Ack(CMD_ERROR); return; }
            DS.Dot = dw32;
            UsbRx.Put(&DS);
//            Uart.Printf("\rutx %u %u", DS.Space, DS.Dot);
        }
        chEvtSignal(PTxThread, EVTMSK_TX_USB);
        UsbUart.Ack(OK);
    }

    else UsbUart.Ack(CMD_UNKNOWN);
}

void App_t::OnUartCmd(Uart_t *PUart) {
    UartCmd_t *PCmd = &PUart->Cmd;
    __attribute__((unused)) int32_t dw32 = 0;  // May be unused in some configurations
    Uart.Printf("\r%S\r", PCmd->Name);
    // Handle command
    if(PCmd->NameIs("Ping")) PUart->Ack(OK);

    else if(PCmd->NameIs("GetID")) PUart->Reply("ID", 2);

    else if(PCmd->NameIs("SetID")) {
        if(PCmd->GetNextToken() != OK) { PUart->Ack(CMD_ERROR); return; }
        if(PCmd->TryConvertTokenToNumber(&dw32) != OK) { PUart->Ack(CMD_ERROR); return; }
        uint8_t r = dw32;
        PUart->Ack(r);
    }

    else PUart->Ack(CMD_UNKNOWN);
}

#if 1 // ==== Pin Sns handlers ====
void ProcessKey(PinSnsState_t *PState, uint32_t Len) {
    if(*PState == pssFalling) { // Key pressed
//        Uart.Printf("\rKey Press");
        App.KeyRx.OnShort();
        Beeper.Beep(1975, BEEP_VOLUME);
    }
    else if(*PState == pssRising) { // Key released
        App.KeyRx.OnRelease();
        Beeper.Off();
        StartReportTmr();
    }
}

void ProcessLine(PinSnsState_t *PState, uint32_t Len) {
    if(PState[0] == pssFalling) { // Key pressed
//        Uart.Printf("\rLineShort");
        App.LineRx1.OnShort();
        Beeper.Beep(999, BEEP_VOLUME);
    }
    else if(PState[0] == pssRising) { // Key released
//        Uart.Printf("\rLR");
        App.LineRx1.OnRelease();
        Beeper.Off();
        StartReportTmr();
    }
}

void ProcessUsbSns(PinSnsState_t *PState, uint32_t Len) {
    if     (*PState == pssRising)  App.SignalEvt(EVTMSK_USB_CONNECTED);
    else if(*PState == pssFalling) App.SignalEvt(EVTMSK_USB_DISCONNECTED);
}
#endif

#if 1 // ==== TX thread ====
static void TxThread(void *arg) {
    chRegSetThreadName("Tx");
    while(true) {
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        if(EvtMsk & EVTMSK_TX_USB) {
            DotSpace_t DS;
            while(App.UsbRx.Get(&DS) == OK) {
                if(DS.Space != 0) chThdSleepMilliseconds(DS.Space);
                Beeper.Beep(1500, 1);
                if(DS.Dot != 0) chThdSleepMilliseconds(DS.Dot);
                Beeper.Off();
                // Send back to USB as LineSignal
                App.LineRx1.Put(&DS);
                StartReportTmr();   // Signal to USB
            }
        } // if tx usb
    } // while true
}
#endif

