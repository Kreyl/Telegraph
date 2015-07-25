/*
 * motorL6470.h
 *
 *  Created on: 25 июля 2015 г.
 *      Author: Kreyl
 */

#ifndef L6470_H_
#define L6470_H_

#include "kl_lib.h"

#if 1 // ==== Pins ====
#define M_SPI           SPI1
#define M_SPI_GPIO      GPIOA
#define M_SCK           5
#define M_MISO          6
#define M_MOSI          7
#define M_CS            8
#if defined STM32F2XX
#define M_SPI_AF        AF5
#endif

#define M_AUX_GPIO      GPIOC
#define M_BUSY_SYNC1    0
#define M_SW1           1
#define M_FLAG1         2
#define M_STBY_RST      6

#endif

#if 1 // ========= Registers etc. ===========
// Registers
#define L6470_REG_ACCELERATION  0x05
#define L6470_REG_DECELERATION  0x06
#define L6470_REG_CONFIG        0x18
#define L6470_REG_STATUS        0x19

#endif

enum Dir_t {dirForward = 1, dirReverse = 0};

class L6470_t {
private:
    Spi_t ISpi;
    void CsHi() { PinSet(M_SPI_GPIO, M_CS); }
    void CsLo() { PinClear(M_SPI_GPIO, M_CS); }
    void CSHiLo() {
        PinSet(M_SPI_GPIO, M_CS);
        for(volatile uint32_t i=0; i<10; i++) __NOP();
        PinClear(M_SPI_GPIO, M_CS);
    }
    // Commands
    uint16_t GetStatus();
    void GetParam(uint8_t Addr, uint8_t *PParam1);
    void GetParam(uint8_t Addr, uint8_t *PParam1, uint8_t *PParam2);
    void SetParam(uint8_t Addr, uint16_t Value);
    void Cmd(uint8_t ACmd); // Single-byte cmd
protected:
    void ResetOn()  { PinClear(M_AUX_GPIO, M_STBY_RST); }
    void ResetOff() { PinSet  (M_AUX_GPIO, M_STBY_RST); }
    bool IsBusy()   { return !PinIsSet(M_AUX_GPIO, M_BUSY_SYNC1); }
    bool IsFlagOn() { return !PinIsSet(M_AUX_GPIO, M_FLAG1); }
public:
    void Init();
    // Motion
    void Run(Dir_t Dir, uint32_t Speed);
    // Stop
    void StopSoftAndHold() { Cmd(0b10110000); } // SoftStop
    void StopSoftAndHiZ()  { Cmd(0b10100000); } // SoftHiZ
    // Acceleration
    void SetAcceleration(uint16_t Value) { SetParam(L6470_REG_ACCELERATION, Value); }
    void SetDeceleration(uint16_t Value) { SetParam(L6470_REG_DECELERATION, Value); }
};


#endif /* L6470_H_ */
