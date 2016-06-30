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
#include "motorcontrol.h"

extern const EXTConfig ext_cfg;

int main(void) {
  halInit();
  chSysInit();
  extStart(&EXTD1, &ext_cfg);

  palClearPad(IOPORT2, PB5);
  palSetPadMode(IOPORT2, PB5, PAL_MODE_OUTPUT_PUSHPULL);

  // Standby: for now just enable it.
  palClearPad(IOPORT4, 2);
  palSetPadMode(IOPORT4, 2, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPad(IOPORT4, 2);
  palClearPad(IOPORT2, PB5);
  palSetPadMode(IOPORT2, PB5, PAL_MODE_OUTPUT_PUSHPULL);

  TachoMotors tachomotors = TachoMotors();
  MotorsDriver motorsdriver = MotorsDriver();
  MotorsController motorscontroller = MotorsController();
  createMotorThread();


  void *a = &tachomotors;
  a = &motorsdriver;
  a = &motorscontroller;
  a = ((int*)a) + 1;
  sei();
  while(TRUE) {
    chThdSleepMilliseconds(100);
  }
}

//
//  //-----------------------------------------------------------
//  //
//  // Front Right
//  //
//  // PD6 PIN6 PWMA-R
//  palClearPad(IOPORT4, 6);
//  palSetPadMode(IOPORT4, 6, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPad(IOPORT4, 6);
//
//  // PB0 PIN8 AIN1-R
//  palClearPad(IOPORT2, 0);
//  palSetPadMode(IOPORT2, 0, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPad(IOPORT2, 0);
//
//  // PB1 PIN9 AIN2-R
//  palClearPad(IOPORT2, 1);
//  palSetPadMode(IOPORT2, 1, PAL_MODE_OUTPUT_PUSHPULL);
//  // palSetPad(IOPORT2, 1);
//
//  //-----------------------------------------------------------
//  //
//  // Rear Right
//  //
//  // PD5 PIN5 PWMB-R
//  palClearPad(IOPORT4, 5);
//  palSetPadMode(IOPORT4, 5, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPad(IOPORT4, 5);
//
//  // PD7 PIN7 BIN1-R
//  palClearPad(IOPORT4, 7);
//  palSetPadMode(IOPORT4, 7, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPad(IOPORT4, 7);
//
//  // PD4 PIN4 BIN2-R
//  palClearPad(IOPORT4, 4);
//  palSetPadMode(IOPORT4, 4, PAL_MODE_OUTPUT_PUSHPULL);
//  // palSetPad(IOPORT4, 4);
//
//  //-----------------------------------------------------------
//  //
//  // Front Left
//  //
//  // PB3 PIN11 PWMA-L
//  palClearPad(IOPORT2, 3);
//  palSetPadMode(IOPORT2, 3, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPad(IOPORT2, 3);
//
//  // PB4 PIN12 AIN1-L
//  palClearPad(IOPORT2, 4);
//  palSetPadMode(IOPORT2, 4, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPad(IOPORT2, 4);
//
//  // PB2 PIN10 AIN2-L
//  palClearPad(IOPORT2, 2);
//  palSetPadMode(IOPORT2, 2, PAL_MODE_OUTPUT_PUSHPULL);
//  // palSetPad(IOPORT2, 2);
//
//  //-----------------------------------------------------------
//  //
//  // Rear Left
//  //
//  // PD3 PIN3 PWMB-L
//  palClearPad(IOPORT4, 3);
//  palSetPadMode(IOPORT4, 3, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPad(IOPORT4, 3);
//
//  // PD0 PIN0 BIN1-L
//  palClearPad(IOPORT4, 0);
//  palSetPadMode(IOPORT4, 0, PAL_MODE_OUTPUT_PUSHPULL);
//  palSetPad(IOPORT4, 0);
//
//  // PD1 PIN1 BIN2-L
//  palClearPad(IOPORT4, 1);
//  palSetPadMode(IOPORT4, 1, PAL_MODE_OUTPUT_PUSHPULL);
//  // palSetPad(IOPORT4, 1);
//
//  //-----------------------------------------------------------
//
//
