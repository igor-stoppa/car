/*
    Car controls.
    Copyright (C) 2015-16 Igor Stoppa <igor.stoppa@gmail.com>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef TACHO_H
#define TACHO_H

#include "ch.h"
#include "hal.h"
#include "motors.h"

#define TACHO_BUFFER_LEN 32

class TachoMotor {
  protected:
    systime_t timestamps[TACHO_BUFFER_LEN];
    uint16_t counter;
    uint8_t current;
  public:
    TachoMotor(){resetMotor(chVTGetSystemTimeX());};
    void resetMotorPeriod(systime_t init_time);
    void resetMotor(systime_t init_time);
    void updateMotor(systime_t new_time);
    systime_t getLastEvent(void);
    systime_t getAvgPeriod(uint8_t periods=(TACHO_BUFFER_LEN - 1));
    uint16_t getCounter(void) {return counter;};
    void resetCounter(void) {counter = 0x0000;};
};

extern "C" void tacho_cb(EXTDriver *extp, expchannel_t channel);

class TachoMotors {
  protected:
  public:
    TachoMotor motors[MOTORS_NUMBER];
    void resetMotors();
    static class TachoMotors *tachomotors;
    TachoMotors();
    friend void tacho_cb(EXTDriver *extp, expchannel_t channel);
};
#endif
