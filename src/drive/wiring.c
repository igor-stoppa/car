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
#include "motorcontrol.h"
#include "motors.h"
#include "twislave.h"

/*

  This is a .c file, rather than .cpp because C allows for
  explicit initialization of both fields in a structure and
  elements in an array.

  This mapping is specific to the ATMega328P:

  TACHO signals are connected to PORT C:
  PC0 (Arduino Pin 14) = LEFT  FRONT MOTOR
  PC1 (Arduino Pin 15) = LEFT  REAR  MOTOR
  PC2 (Arduino Pin 16) = RIGHT REAR  MOTOR
  PC3 (Arduino Pin 17) = RIGHT FRONT MOTOR

  Motors are connected to TC0 and to TC2.
  TC1 is reserved by the OS for tickless functionality.
  2A (Arduino Pin  6) = PWM LEFT  FRONT MOTOR
  2B (Arduino Pin  5) = PWM RIGHT FRONT MOTOR
  0A (Arduino Pin 11) = PWM LEFT  REAR  MOTOR
  0B (Arduino Pin  3) = PWM RIGHT REAR  MOTOR

  Of the remaining pins, pairs from the same port are used
  to control the spinning directon of each motor:
  Forward Backward  Effect
        0        0  Motor disengaged, no forceful control.
        0        1  Motor engaged, spinning backward.
        1        0  Motor engaged, spinning forward.
        1        1  Motor locked.

 */

typedef enum {
  MASK_TACHO_LEFT_FRONT  = _BV(LEFT_FRONT),
  MASK_TACHO_RIGHT_FRONT = _BV(RIGHT_FRONT),
  MASK_TACHO_LEFT_REAR   = _BV(LEFT_REAR),
  MASK_TACHO_RIGHT_REAR  = _BV(RIGHT_REAR),
  MASK_TACHOS = (MASK_TACHO_LEFT_FRONT  |
                 MASK_TACHO_RIGHT_FRONT |
                 MASK_TACHO_LEFT_REAR   |
                 MASK_TACHO_RIGHT_REAR),
} TachoMasks;

void tacho_cb(EXTDriver *extp, expchannel_t channel);

const EXTConfig ext_cfg = {
  .channels = {
    [PORTC_INDEX] = {
      .mode = MASK_TACHOS,
      .cb = tacho_cb,
    },
  },
};

Motors motorslinkage[MOTORS_NUMBER] = {
  [LEFT_FRONT] = LEFT_REAR,
  [LEFT_REAR] = LEFT_FRONT,
  [RIGHT_FRONT] = RIGHT_REAR,
  [RIGHT_REAR] = RIGHT_FRONT,
};

MotorData motorsdata[MOTORS_NUMBER] = {
  [RIGHT_FRONT] = {
    .pwm_ddr_address = &DDRD,
    .pwm_pin_bit_mask = _BV(6),
    .pwm_counter_address = &OCR0A,
    .pwm_control_register_address = &TCCR0A,
    .pwm_compare_output_mode_mask = _BV(COM0A1),
    .pwm_wgm_cs_mask = (3 << 4) | 3,
    .dir_ddr_address = &DDRB,
    .dir_pin_address = &PORTB,
    .dir_forward_bit_mask = _BV(0),
    .dir_backward_bit_mask = _BV(1),
  },
  [RIGHT_REAR] = {
    .pwm_ddr_address = &DDRD,
    .pwm_pin_bit_mask = _BV(3),
    .pwm_counter_address = &OCR0B,
    .pwm_control_register_address = &TCCR2A,
    .pwm_compare_output_mode_mask = _BV(COM2A1),
    .pwm_wgm_cs_mask = (3 << 4) | 4,
    .dir_ddr_address = &DDRD,
    .dir_pin_address = &PORTD,
    .dir_forward_bit_mask = _BV(7),
    .dir_backward_bit_mask = _BV(4),
  },
  [LEFT_FRONT] = {
    .pwm_ddr_address = &DDRB,
    .pwm_pin_bit_mask = _BV(3),
    .pwm_counter_address = &OCR2A,
    .pwm_control_register_address = &TCCR0A,
    .pwm_compare_output_mode_mask = _BV(COM0B1),
    .pwm_wgm_cs_mask = (3 << 4) | 3,
    .dir_ddr_address = &DDRB,
    .dir_pin_address = &PORTB,
    .dir_forward_bit_mask = _BV(4),
    .dir_backward_bit_mask = _BV(2),
  },
  [LEFT_REAR] = {
    .pwm_ddr_address = &DDRD,
    .pwm_pin_bit_mask = _BV(5),
    .pwm_counter_address = &OCR2B,
    .pwm_control_register_address = &TCCR2A,
    .pwm_compare_output_mode_mask = _BV(COM2B1),
    .pwm_wgm_cs_mask = (3 << 4) | 4,
    .dir_ddr_address = &DDRD,
    .dir_pin_address = &PORTD,
    .dir_forward_bit_mask = _BV(0),
    .dir_backward_bit_mask = _BV(1),
  },
};


DECLARE_TWI_HANDLERS(motor);
