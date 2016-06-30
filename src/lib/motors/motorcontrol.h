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

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H
#include "motors.h"
#include "ch.h"
#include "hal.h"
#ifdef __cplusplus
extern "C" {
#endif
  #include "twislave.h"
#ifdef __cplusplus
}
#endif


// The MSb is the direction, the remaining 15 bits are the real period.
typedef uint16_t packed_period_t;

typedef struct {
  volatile uint8_t *pwm_ddr_address;
  volatile uint8_t pwm_pin_bit_mask;
  volatile uint8_t *pwm_counter_address;
  volatile uint8_t *pwm_control_register_address;
  volatile uint8_t pwm_compare_output_mode_mask;
  volatile uint8_t pwm_wgm_cs_mask;
  volatile uint8_t *dir_ddr_address;
  volatile uint8_t *dir_pin_address;
  volatile uint8_t dir_forward_bit_mask;
  volatile uint8_t dir_backward_bit_mask;
} MotorData;

TWI_CREATE_COMMANDS(motor, 1,
  PWM,                 // 01
  STATE,               // 02
  SET_ACTION,          // 03
  TACHO_CALIB_PERIOD,  // 04
  TACHO_AVG_PERIOD,    // 05
  TACHO_COUNT,         // 06
  PWM_M,               // 07
  PWM_Q,               // 08
);

typedef union {
  uint8_t data[0];
  uint8_t pwm;
  uint8_t state;
  uint16_t tacho_count;
  systime_t tacho_avg_period;
  uint8_t command;
  systime_t tacho_calib_period;
  float m;
  float q;
} __attribute__((__packed__)) MotorRegValue;

typedef struct {
  uint8_t reg;
  MotorRegValue value;
} __attribute__((__packed__)) __attribute__((__may_alias__)) TWImotorBuffer;

// Do not expose the class declaration when included
// from a c file, which is anyway not interested in it.
#ifdef __cplusplus
class MotorsDriver {
  public:
    static class MotorsDriver *motorsdriver;
    MotorsDriver(void);
    void configure(uint8_t motor);
    void setPWM(uint8_t motor, uint8_t val=0);
    uint8_t getPWM(uint8_t motor);
    void increasePWM(uint8_t motor, uint8_t delta);
    void decreasePWM(uint8_t motor, uint8_t delta);
    void setState(uint8_t motor, MotorStates state);
    uint8_t getState(uint8_t motor);
    void idle(uint8_t motor) { setState(motor, IDLE);};
    void forward(uint8_t motor) { setState(motor, FORWARD);};
    void backward(uint8_t motor) { setState(motor, BACKWARD);};
    void lock(uint8_t motor) { setState(motor, LOCK);};
    friend uint8_t twi_motor_handler(uint8_t mode);
};

typedef enum {
  DISABLED = 0,
  CALIBRATE,
  CRUISE,
  DRIVE_MODES
} DriveMode;

typedef enum {
  DRIVE_NOP = 0,
  DRIVE_INIT,
  DRIVE_CONTINUE,
  DRIVE_STEPS,
} DriveStep;

typedef enum {
  CALIBRATE_STATIC_MIN_PWM_INIT = DRIVE_INIT,
  CALIBRATE_STATIC_MIN_PWM_RAMP_UP,
  CALIBRATE_STATIC_MIN_PWM_MEASURE,
  CALIBRATE_MIN_PERIOD_RAMP_UP,
  CALIBRATE_MIN_PERIOD_MEASURE,
  CALIBRATE_DYNAMIC_MIN_PWM_RAMP_DOWN,
  CALIBRATE_DYNAMIC_MIN_PWM_MEASURE,
  CALIBRATE_STEPS,
} CalibrateStep;

typedef uint32_t (*DriveModeHandler)(void);

class MotorsController {
  public:
    static DriveModeHandler drive_mode_handler;
    static uint8_t drive_step;
    static class MotorsController *motorscontroller;
    static volatile packed_period_t target_periods[MOTORS_NUMBER];
    static volatile packed_period_t real_target_periods[MOTORS_NUMBER];
    static volatile packed_period_t min_periods[MOTORS_NUMBER];
    static volatile packed_period_t static_max_periods[MOTORS_NUMBER];
    static volatile packed_period_t dynamic_max_periods[MOTORS_NUMBER];
    static volatile uint8_t static_min_pwms[MOTORS_NUMBER];
    static volatile uint8_t dynamic_min_pwms[MOTORS_NUMBER];
    static volatile float m[MOTORS_NUMBER];
    static volatile float q[MOTORS_NUMBER];
    static volatile Motors linkage[MOTORS_NUMBER];
    MotorsController(void);
    void set_all_raw(uint8_t pwm, uint8_t state);
    void set_motor_raw(uint8_t motor, uint8_t pwm, uint8_t state);
    void set_motor_period(uint8_t motor, packed_period_t target_period) {
      target_periods[motor] = target_period;
      real_target_periods[motor] = target_period;
    };
    uint32_t control(void);
    friend uint8_t convert(void);
    friend uint8_t twi_motor_handler(uint8_t mode);
    friend uint32_t calibrate_motors(void);
    friend void calculate_parameters(void);
    friend uint32_t cruise(void);
};

void createMotorThread(void);

#endif

#endif
