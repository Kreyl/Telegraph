/*
 * motor.h
 *
 *  Created on: 25 июля 2015 г.
 *      Author: Kreyl
 */

#ifndef MOTOR_MOTOR_H_
#define MOTOR_MOTOR_H_

#include "L6470.h"

class Motor_t : public L6470_t {
public:
    void Run();
    void Stop();
};



#endif /* MOTOR_MOTOR_H_ */
