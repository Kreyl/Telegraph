/*
 * motor.h
 *
 *  Created on: 25 июля 2015 г.
 *      Author: Kreyl
 */

#ifndef MOTOR_MOTOR_H_
#define MOTOR_MOTOR_H_

#include "L6470.h"

#define MOTOR_SPEED     10000
#define MOTOR_ACC       60

class Motor_t : private L6470_t {
public:
    bool IsRunning = false;
    void Init() {
        L6470_t::Init();
        SetAcceleration(MOTOR_ACC);
        SetDeceleration(MOTOR_ACC);
    }
    void Run() {
        L6470_t::Run(dirForward, MOTOR_SPEED);
        IsRunning = true;
    }
    void Stop() {
        L6470_t::StopSoftAndHiZ();
        IsRunning = false;
    }
};



#endif /* MOTOR_MOTOR_H_ */
