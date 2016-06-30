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

#include "ch.h"
#include "hal.h"
#include "tachomotor.h"

// class TachoMotor

void
TachoMotor::updateMotor(systime_t new_time) {
  current = (current + 1) % TACHO_BUFFER_LEN;
  timestamps[current] = new_time;
  counter++;
}

void
TachoMotor::resetMotorPeriod(systime_t init_time) {
  for (uint8_t i = 0; i < TACHO_BUFFER_LEN; i++)
    timestamps[i] = init_time;
}

void
TachoMotor::resetMotor(systime_t init_time) {
  current = 0;
  resetMotorPeriod(init_time);
  resetCounter();
}

systime_t
TachoMotor::getLastEvent(void) {
  return timestamps[current];
}

systime_t
TachoMotor::getAvgPeriod(uint8_t periods) {
  systime_t base = 0;
  systime_t rest = 0;
  for (unsigned short i = 0; i < periods; i++) {
    systime_t delta =
      timestamps[(current + TACHO_BUFFER_LEN - i) % periods] -
      timestamps[(current + TACHO_BUFFER_LEN - i - 1) % periods];
    base += delta / periods;
    rest += delta % periods;
  }
  return base + rest / periods;
}

// class TachoMotors

class TachoMotors *
TachoMotors::tachomotors;

TachoMotors::TachoMotors() {
    tachomotors = this;
    resetMotors();
};

void TachoMotors::resetMotors() {
  systime_t init_time = chVTGetSystemTimeX();
  for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
    motors[i].resetMotor(init_time);
  }
}

#define isMotorActive(changes, motor) ((changes & _BV(motor)) ? true : false)

#define blink() \
  (PORTB = ((PORTB & ~_BV(5)) | (_BV(5) & ~PORTB)))

void tacho_cb(EXTDriver *extp, expchannel_t channel) {
  chSysLockFromISR();
  uint8_t changes = extp->pc_current_values[channel] ^
                    extp->pc_old_values[channel];
  systime_t current_time = chVTGetSystemTimeX();
  for (uint8_t i = 0; i < MOTORS_NUMBER; i++)
    if (isMotorActive(changes, i))
      TachoMotors::tachomotors->motors[i].updateMotor(current_time);
  chSysUnlockFromISR();
}
