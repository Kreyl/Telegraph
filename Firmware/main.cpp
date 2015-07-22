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
//#include "SimpleSensors.h"

App_t App;

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

    // ==== Main cycle ====
    App.ITask();
}

__attribute__ ((__noreturn__))
void App_t::ITask() {
    while(true) {
        __attribute__((unused))
        uint32_t EvtMsk = chEvtWaitAny(ALL_EVENTS);
        // ==== Card ====
//        if(EvtMsk & EVTMSK_CARD_APPEARS) ProcessCardAppearance();

#if USB_ENABLED // ==== USB connection ====
        if(EvtMsk & EVTMSK_USB_CONNECTED) {
            chSysLock();
            Clk.SetFreq48Mhz();
            Clk.InitSysTick();
            chSysUnlock();
            Usb.Init();
            chThdSleepMilliseconds(540);
            Usb.Connect();
            Uart.Printf("\rUsb connected, AHB freq=%uMHz", Clk.AHBFreqHz/1000000);
        }
        if(EvtMsk & EVTMSK_USB_DISCONNECTED) {
            Usb.Shutdown();
            MassStorage.Reset();
            chSysLock();
            Clk.SetFreq12Mhz();
            Clk.InitSysTick();
            chSysUnlock();
            Uart.Printf("\rUsb disconnected, AHB freq=%uMHz", Clk.AHBFreqHz/1000000);
        }
#endif

    } // while true
}
