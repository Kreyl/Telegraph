/*
 * adc_f2.cpp
 *
 *  Created on: 25 ���. 2013 �.
 *      Author: kreyl
 */

#include "adc_f2.h"

static Thread *IPThread;
static eventmask_t IEvt;

// Wrapper for IRQ
extern "C" {
void AdcTxIrq(void *p, uint32_t flags) {
    dmaStreamDisable(ADC_DMA);
    ADC1->CR2 = 0;  // Disable ADC
    // Signal event to thread
    if(IPThread != nullptr) {
        chSysLockFromIsr();
        chEvtSignalI(IPThread, IEvt);
        chSysUnlockFromIsr();
    }
}
} // extern C

void Adc_t::Init() {
    rccEnableADC1(FALSE);   // Enable digital clock
    SetupClk(adcDiv4);      // Setup ADCCLK
    SetChannelCount(ADC_CHANNEL_CNT);
    for(uint8_t i = 0; i < ADC_CHANNEL_CNT; i++) ChannelConfig(AdcChannels[i]);
    // ==== DMA ====
    // Here only unchanged parameters of the DMA are configured.
    dmaStreamAllocate     (ADC_DMA, IRQ_PRIO_LOW, AdcTxIrq, NULL);
    dmaStreamSetPeripheral(ADC_DMA, &ADC1->DR);
    dmaStreamSetMode      (ADC_DMA, ADC_DMA_MODE);
}

void Adc_t::Measure(Thread *PThread, eventmask_t Evt) {
    IPThread = PThread;
    IEvt = Evt;
    // DMA
    dmaStreamSetMemory0(ADC_DMA, Result);
    dmaStreamSetTransactionSize(ADC_DMA, ADC_BUF_SZ);
    dmaStreamSetMode(ADC_DMA, ADC_DMA_MODE);
    dmaStreamEnable(ADC_DMA);
    // ADC
    ADC1->CR1 = ADC_CR1_SCAN;
    ADC1->CR2 = ADC_CR2_DMA | ADC_CR2_ADON;
    StartConversion();
}

uint16_t Adc_t::Average() {
    uint32_t Rslt = Result[0];
    for(uint32_t i=1; i<ADC_BUF_SZ; i++) Rslt += Result[i];
    return (Rslt / ADC_BUF_SZ);
}


void Adc_t::SetChannelCount(uint32_t Count) {
    chDbgAssert((Count > 0), "Adc Ch cnt=0", NULL);
    ADC1->SQR1 &= ~ADC_SQR1_L;  // Clear count
    ADC1->SQR1 |= (Count - 1) << 20;
}

void Adc_t::ChannelConfig(AdcChnl_t ACfg) {
    chDbgAssert((ACfg.N > 0), "Adc Ch N=0", NULL);
    uint32_t Offset;
    // Setup sampling time of the channel
    if(ACfg.N <= 9) {
        Offset = ACfg.N * 3;
        ADC1->SMPR2 &= ~((uint32_t)0b111 << Offset);    // Clear bits
        ADC1->SMPR2 |= ACfg.SampleTime << Offset;       // Set 0; // new bits
    }
    else {
        Offset = (ACfg.N - 10) * 3;
        ADC1->SMPR1 &= ~((uint32_t)0b111 << Offset);
        ADC1->SMPR1 |= ACfg.SampleTime << Offset;
    }
    // Put channel number in appropriate place in sequence
    if(ACfg.SeqIndx <= 6) {
        Offset = (ACfg.SeqIndx - 1) * 5;
        ADC1->SQR3 &= ~(uint32_t)(0b11111 << Offset);
        ADC1->SQR3 |= (uint32_t)(ACfg.N << Offset);
    }
    else if(ACfg.SeqIndx <= 12) {
        Offset = (ACfg.SeqIndx - 7) * 5;
        ADC1->SQR2 &= ~(uint32_t)(0b11111 << Offset);
        ADC1->SQR2 |= (uint32_t)(ACfg.N << Offset);
    }
    else if(ACfg.SeqIndx <= 16) {
        Offset = (ACfg.SeqIndx - 13) * 5;
        ADC1->SQR1 &= ~(uint32_t)(0b11111 << Offset);
        ADC1->SQR1 |= (uint32_t)(ACfg.N << Offset);
    }
}

void Adc_t::Disable() {
    ADC1->CR2 = 0;
    dmaStreamRelease(ADC_DMA);
    rccDisableADC1(FALSE);
}
