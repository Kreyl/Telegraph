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

App_t App;
Beeper_t Beeper;
PinOutputPushPull_t Line1Tx(GPIOB, 4);

// Universal VirtualTimer callback
void TmrGeneralCallback(void *p) {
    chSysLockFromIsr();
    App.SignalEvtI((eventmask_t)p);
    chSysUnlockFromIsr();
}

int main() {
    // ==== Setup clock ====
    Clk.UpdateFreqValues();
    uint8_t ClkResult = FAILURE;
    Clk.SetupFlashLatency(12);  // Setup Flash Latency for clock in MHz
    // 12 MHz/6 = 2; 2*192 = 384; 384/8 = 48 (preAHB divider); 384/8 = 48 (USB clock)
    Clk.SetupPLLDividers(6, 192, pllSysDiv8, 8);
    // 48/4 = 12 MHz core clock. APB1 & APB2 clock derive on AHB clock
    Clk.SetupBusDividers(ahbDiv4, apbDiv1, apbDiv1);
    if((ClkResult = Clk.SwitchToPLL()) == 0) Clk.HSIDisable();
    Clk.UpdateFreqValues();

    // ==== Init OS ====
    halInit();
    chSysInit();

    // ==== Init Hard & Soft ====
    App.InitThread();
    Uart.Init(115200);
    Uart.Printf("\r%S_%S   AHBfreq=%uMHz", APP_NAME, APP_VERSION, Clk.AHBFreqHz/1000000);
    // Report problem with clock if any
    if(ClkResult) Uart.Printf("Clock failure\r");

    PinSensors.Init();
    Line1Tx.Init();
    Line1Tx.SetLo();

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
            while(true) {
                uint32_t Duration=0;
                uint8_t r = LineRx1.DotBuf.Get(&Duration);
                if(r != OK) break;
                //UsbUart.Printf("RX=%u\r\n", Duration);
                Uart.Printf("\rRX=%u", Duration);
            }
        }

#if 1   // ==== Uart cmd ====
        if(EvtMsk & EVTMSK_UART_NEW_CMD) {
            OnUartCmd(&Uart);
            Uart.SignalCmdProcessed();
        }
#endif

#if USB_ENABLED // ==== USB connection ====
        if(EvtMsk & EVTMSK_USB_CONNECTED) {
            chSysLock();
            Clk.SetFreq48Mhz();
            Clk.InitSysTick();
            chSysUnlock();
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
            chSysLock();
            Clk.SetFreq12Mhz();
            Clk.InitSysTick();
            chSysUnlock();
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
        Uart.Printf("\rKey Press");
    }
    else if(*PState == pssRising) { // Key released
        Uart.Printf("\rKey Release");
    }
}

void ProcessLine(PinSnsState_t *PState, uint32_t Len) {
    if(PState[0] == pssFalling) { // Key pressed
//        Uart.Printf("\rLineShort");
        App.LineRx1.OnLineShort();
    }
    else if(PState[0] == pssRising) { // Key released
//        Uart.Printf("\rLine Release");
        App.LineRx1.OnLineRelease();
        chVTStartIfNotStarted(&App.ITmrRxReport, MS2ST(RX_REPORT_EVERY_MS), EVTMSK_RX_REPORT);
    }
}

void ProcessUsbSns(PinSnsState_t *PState, uint32_t Len) {
    if     (*PState == pssRising)  App.SignalEvt(EVTMSK_USB_CONNECTED);
    else if(*PState == pssFalling) App.SignalEvt(EVTMSK_USB_DISCONNECTED);
}
#endif
