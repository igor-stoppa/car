/*
    Motors controls.
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
#include "tachomotor.h"

template<int s> struct Sizer;
struct foo {
      int a,b;
};

#define sizer(foo) \
    Sizer<sizeof(foo)> size

typedef enum {
  MAX_PWM = 255,
} Limits;

extern MotorData motorsdata[MOTORS_NUMBER];
extern Motors motorslinkage[MOTORS_NUMBER];

uint8_t
twi_motor_handler(uint8_t mode);

class MotorsDriver *MotorsDriver::motorsdriver;

MotorsDriver::MotorsDriver(void) {
  motorsdriver = this;
  for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
    configure(i);
    setPWM(i, 0x00);
    idle(i);
  }
}

void
MotorsDriver::configure(uint8_t motor) {
  register MotorData &m = motorsdata[motor];
  // Set PWM pin as output.
  (*m.pwm_ddr_address) |= m.pwm_pin_bit_mask;
  // Set PWM mode and divider.
  (*m.pwm_control_register_address) |=
    m.pwm_compare_output_mode_mask | ((m.pwm_wgm_cs_mask >> 4) & 3);
  (*(m.pwm_control_register_address + 1)) |=
    ((m.pwm_wgm_cs_mask >> 3) & 8) | (m.pwm_wgm_cs_mask & 7);
  // Set Forward/Backward pins as output.
  (*m.dir_ddr_address) |= (m.dir_forward_bit_mask | m.dir_backward_bit_mask);
}

void
MotorsDriver::setPWM(uint8_t motor, uint8_t val) {
  uint8_t start_motor;
  uint8_t end_motor;
  if (motor < ALL_MOTORS) {
    start_motor = motor;
    end_motor = motor + 1;
  } else if (motor == ALL_MOTORS){
    start_motor = 0;
    end_motor = MOTORS_NUMBER;
  } else {
    start_motor = end_motor = 0;
  }
  for (uint8_t counter = start_motor; counter < end_motor; counter++) {
    // PWM duty cycle, expressed in 255ths of the PWM period.
    (*motorsdata[counter].pwm_counter_address) = val;
  }
}

uint8_t
MotorsDriver::getPWM(uint8_t motor) {
  // PWM duty cycle, expressed in 255ths of the PWM period.
  return (*motorsdata[motor].pwm_counter_address);
}

void
MotorsDriver::increasePWM(uint8_t motor, uint8_t delta) {
  uint8_t pwm = (*motorsdata[motor].pwm_counter_address) + delta;
  (*motorsdata[motor].pwm_counter_address) = (pwm < delta) ? MAX_PWM : pwm;
}

void
MotorsDriver::decreasePWM(uint8_t motor, uint8_t delta) {
  if ((*motorsdata[motor].pwm_counter_address) > delta)
    (*motorsdata[motor].pwm_counter_address) -= delta;
  else
    (*motorsdata[motor].pwm_counter_address) = 0;
}

void
MotorsDriver::setState(uint8_t motor, MotorStates state) {
  uint8_t start_motor;
  uint8_t end_motor;
  if (motor < ALL_MOTORS) {
    start_motor = motor;
    end_motor = motor + 1;
  } else if (motor == ALL_MOTORS){
    start_motor = 0;
    end_motor = MOTORS_NUMBER;
  } else {
    start_motor = end_motor = 0;
  }
  for (uint8_t counter = start_motor; counter < end_motor; counter++) {
    register MotorData &m = motorsdata[counter];
    switch (state) {
      case IDLE:
        (*m.dir_pin_address) &=
          ~(m.dir_forward_bit_mask | m.dir_backward_bit_mask);
        break;
      case BACKWARD:
        (*m.dir_pin_address) &= ~m.dir_forward_bit_mask;
        (*m.dir_pin_address) |= m.dir_backward_bit_mask;
        break;
      case FORWARD:
        (*m.dir_pin_address) |= m.dir_forward_bit_mask;
        (*m.dir_pin_address) &= ~m.dir_backward_bit_mask;
        break;
      case LOCK:
        (*m.dir_pin_address) |=
          m.dir_forward_bit_mask | m.dir_backward_bit_mask;
        break;
      default: break;
    }
  }
}

uint8_t
MotorsDriver::getState(uint8_t motor) {
  register MotorData &m = motorsdata[motor];
  uint8_t forward = (*m.dir_pin_address) & m.dir_forward_bit_mask;
  uint8_t backward = (*m.dir_pin_address) & m.dir_backward_bit_mask;
  if (forward && backward)
    return LOCK;
  else if (forward)
    return FORWARD;
  else if (backward)
    return BACKWARD;
  else
    return IDLE;
}

#define WRITE_VALUE 0x0
#define READ_VALUE 0x1
#define SLAVE_ADDRESS 0x10
#define MOTOR_THREAD_IDLE_PERIOD_MS 250

DECLARE_EXTERN_TWI_HANDLERS(motor);

static THD_WORKING_AREA(waThreadMotor, 32);
static THD_FUNCTION(ThreadMotor, arg) {
  (void)arg;
  uint32_t delay;
  while (true) {
    if (MotorsController::motorscontroller->drive_mode_handler) {
      delay = MotorsController::motorscontroller->drive_mode_handler();
    } else {
      delay = MOTOR_THREAD_IDLE_PERIOD_MS;
    }
    chThdSleepMilliseconds(delay);
  }
}

typedef enum {
  WAIT_RAMP_UP_PWM_MIN_PERIOD = 125,
  WAIT_RAMP_UP_PWM_STATIC_MIN_PWM = 50,
  WAIT_PWM_SPEED_STABILIZE = 200,
} MotorDelays;

typedef enum {
  DELTA_PWM_MIN_PERIOD_RAMP_UP = 63,
  DELTA_PWM_MIN_PWM_RAMP_UP = 15,
  DELTA_PWM_MIN_PWM_RAMP_DOWN = 10,
} DeltaCalibrate;

void
calculate_parameters(void) {
  for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
    MotorsController::motorscontroller->m[i] =
      ((float)(255 - MotorsController::motorscontroller->dynamic_min_pwms[i]) /
       (float)(MotorsController::motorscontroller->min_periods[i] -
               MotorsController::motorscontroller->dynamic_max_periods[i]));
    MotorsController::motorscontroller->q[i] =
      MotorsController::motorscontroller->dynamic_min_pwms[i] -
      MotorsController::motorscontroller->m[i] *
      MotorsController::motorscontroller->dynamic_max_periods[i];
  }
}

uint32_t
calibrate_motors(void) {
  uint32_t wait_ms = 0;
  palSetPad(IOPORT2, PB5);
  switch (MotorsController::motorscontroller->drive_step) {
    case CALIBRATE_STATIC_MIN_PWM_INIT: {
      systime_t init_time = chVTGetSystemTimeX();
      for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
        TachoMotors::tachomotors->motors[i].resetMotorPeriod(init_time);
        MotorsController::motorscontroller->min_periods[i] = 0;
        MotorsController::motorscontroller->static_max_periods[i] = 0;
        MotorsController::motorscontroller->dynamic_max_periods[i] = 0;
        MotorsController::motorscontroller->static_min_pwms[i] = 0;
        MotorsController::motorscontroller->dynamic_min_pwms[i] = 0;
        MotorsController::motorscontroller->set_motor_raw(i, 0, FORWARD);
      }
      MotorsController::motorscontroller->drive_step =
        CALIBRATE_STATIC_MIN_PWM_RAMP_UP;
    } break;
    case CALIBRATE_STATIC_MIN_PWM_RAMP_UP: {
      bool done = TRUE;
      for (uint8_t i = 0; i < MOTORS_NUMBER; i++)
        if (TachoMotors::tachomotors->motors[i].getAvgPeriod() == 0) {
          done = FALSE;
          MotorsDriver::motorsdriver->
              increasePWM(i, DELTA_PWM_MIN_PWM_RAMP_UP);
        }
      if (done) {
        for (uint8_t i = 0; i < MOTORS_NUMBER; i++)
          MotorsController::motorscontroller->static_max_periods[i] =
              TachoMotors::tachomotors->motors[i].getAvgPeriod();
        MotorsController::motorscontroller->drive_step =
          CALIBRATE_STATIC_MIN_PWM_MEASURE;
      } else {
        wait_ms = WAIT_RAMP_UP_PWM_STATIC_MIN_PWM;
      }
    } break;
    case CALIBRATE_STATIC_MIN_PWM_MEASURE: {
      for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
        MotorsController::motorscontroller->static_min_pwms[i] =
            MotorsDriver::motorsdriver->getPWM(i);
      }
      MotorsController::motorscontroller->drive_step =
          CALIBRATE_MIN_PERIOD_RAMP_UP;
      wait_ms = WAIT_PWM_SPEED_STABILIZE;
    } break;
    case CALIBRATE_MIN_PERIOD_RAMP_UP: {
      bool done = TRUE;
      for (uint8_t i = 0; i < MOTORS_NUMBER; i++)
        if (MotorsDriver::motorsdriver->getPWM(i) != 0xff) {
          done = FALSE;
          MotorsDriver::motorsdriver->
              increasePWM(i, DELTA_PWM_MIN_PERIOD_RAMP_UP);
        }
      if (done) {
        MotorsController::motorscontroller->drive_step =
          CALIBRATE_MIN_PERIOD_MEASURE;
      } else {
        wait_ms = WAIT_RAMP_UP_PWM_MIN_PERIOD;
      }
    } break;
    case CALIBRATE_MIN_PERIOD_MEASURE: {
      for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
        MotorsController::motorscontroller->min_periods[i] =
            TachoMotors::tachomotors->motors[i].getAvgPeriod();
      }
      MotorsController::motorscontroller->drive_step =
            CALIBRATE_DYNAMIC_MIN_PWM_RAMP_DOWN;
    } break;
    case CALIBRATE_DYNAMIC_MIN_PWM_RAMP_DOWN: {
      systime_t current_time = chVTGetSystemTimeX();
      bool done = TRUE;
      for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
        systime_t elapsed_time =
            current_time - TachoMotors::tachomotors->motors[i].getLastEvent();
        if (elapsed_time < MS2ST(250)) {
          done = FALSE;
          MotorsDriver::motorsdriver->
              decreasePWM(i, DELTA_PWM_MIN_PWM_RAMP_DOWN);
        }
      }
      if (done) {
        MotorsController::motorscontroller->drive_step =
          CALIBRATE_DYNAMIC_MIN_PWM_MEASURE;
      } else {
        wait_ms = WAIT_PWM_SPEED_STABILIZE;
      }
    } break;
    case CALIBRATE_DYNAMIC_MIN_PWM_MEASURE: {
      for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
        MotorsController::motorscontroller->dynamic_min_pwms[i] =
            MotorsDriver::motorsdriver->getPWM(i) +
            DELTA_PWM_MIN_PWM_RAMP_DOWN;
        MotorsController::motorscontroller->set_motor_raw(i, 0, IDLE);
      }
      calculate_parameters();
      MotorsController::motorscontroller->drive_step = DRIVE_NOP;
      MotorsController::motorscontroller->drive_mode_handler = NULL;
    } break;
  }
  palClearPad(IOPORT2, PB5);
  return wait_ms;
}

uint8_t
convert(uint8_t motor, packed_period_t packed_period) {
  return 0;
}

//uint32_t
//cruise(void) {
//  for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
//    uint8_t current_direction = MotorsDriver::motorsdriver->getState(i);
//    systime_t current_period = 0;
//    if (current_direction != IDLE && current_direction != LOCK) {
//      current_period = TachoMotors::tachomotors->motors[i].getAvgPeriod();
//    }
//    systime_t target =
//      MotorsController::motorscontroller->real_target_periods[i];
///*    systime_t current =
//      TachoMotors::tachomotors->motors[i].getAvgPeriod() |
//      ((MotorsDriver::motorsdriver->getState(i) == BACKWARD) & 0x8000);*/
//  }
//  return WAIT_PWM_SPEED_STABILIZE;
//}

DriveModeHandler
MotorsController::drive_mode_handler = NULL;

uint8_t
MotorsController::drive_step = DRIVE_NOP;

MotorsController *
MotorsController::motorscontroller = NULL;

volatile packed_period_t
MotorsController::min_periods[MOTORS_NUMBER];

volatile packed_period_t
MotorsController::static_max_periods[MOTORS_NUMBER];

volatile packed_period_t
MotorsController::dynamic_max_periods[MOTORS_NUMBER];

volatile packed_period_t
MotorsController::target_periods[MOTORS_NUMBER];

volatile packed_period_t
MotorsController::real_target_periods[MOTORS_NUMBER];

volatile uint8_t
MotorsController::static_min_pwms[MOTORS_NUMBER];

volatile uint8_t
MotorsController::dynamic_min_pwms[MOTORS_NUMBER];

volatile float
MotorsController::m[MOTORS_NUMBER];

volatile float
MotorsController::q[MOTORS_NUMBER];

MotorsController::MotorsController(void) {
  motorscontroller = this;
  twi_runtime_handler_init(motor);
  twi_initialise((uint8_t)SLAVE_ADDRESS, (uint8_t)GENERAL_CALL_ADDRESS_TRUE);
}

void
MotorsController::set_all_raw(uint8_t pwm, uint8_t state) {
  for (uint8_t i = 0; i < MOTORS_NUMBER; i++) {
    MotorsDriver::motorsdriver->setPWM(i, pwm);
    MotorsDriver::motorsdriver->setState(i, (MotorStates)state);
  }
}

void
MotorsController::set_motor_raw(uint8_t motor, uint8_t pwm, uint8_t state) {
    MotorsDriver::motorsdriver->setPWM(motor, pwm);
    MotorsDriver::motorsdriver->setState(motor, (MotorStates)state);
}

uint8_t
twi_motor_handler(uint8_t mode) {
  switch (mode) {
    case TWI_HANDLER_WRITE: {
      switch (get_parm_field(twi_motor.twi_motor_buffer.reg)) {
        case TWI_motor_PWM: {
          MotorsDriver::motorsdriver->setPWM(
            (Motors)get_item_field(twi_motor.twi_motor_buffer.reg),
            twi_motor.twi_motor_buffer.value.pwm);
        } break;
        case TWI_motor_STATE: {
          MotorsDriver::motorsdriver->setState(
            (Motors)get_item_field(twi_motor.twi_motor_buffer.reg),
            (MotorStates)twi_motor.twi_motor_buffer.value.state);
        } break;
        case TWI_motor_TACHO_COUNT: {
          TachoMotors::tachomotors->motors[
            get_item_field(twi_motor.twi_motor_buffer.reg)].resetCounter();
        } break;
        case TWI_motor_TACHO_AVG_PERIOD: {
          MotorsController::target_periods[
            get_item_field(twi_motor.twi_motor_buffer.reg)] =
              twi_motor.twi_motor_buffer.value.tacho_avg_period;
        } break;
        case TWI_motor_SET_ACTION: {
          switch(twi_motor.twi_motor_buffer.value.command) {
            MotorsController::motorscontroller->drive_step = DRIVE_INIT;
            case DISABLED: {
              MotorsController::motorscontroller->drive_mode_handler = NULL;
            } break;
            case CALIBRATE: {
              MotorsController::motorscontroller->drive_mode_handler =
                calibrate_motors;
              MotorsController::motorscontroller->drive_step = DRIVE_INIT;
            } break;
          }
        } break;
      }
    } return 0;
    case TWI_HANDLER_READ: {
      switch (get_parm_field(twi_motor.twi_motor_buffer.reg)) {
        case TWI_motor_PWM: {
          twi_motor.twi_motor_buffer.value.pwm =
            MotorsDriver::motorsdriver->getPWM(
              (Motors)get_item_field(twi_motor.twi_motor_buffer.reg));
        } return sizeof(twi_motor.twi_motor_buffer.value.pwm);
        case TWI_motor_STATE: {
          twi_motor.twi_motor_buffer.value.state =
            MotorsDriver::motorsdriver->getState(
              (Motors)get_item_field(twi_motor.twi_motor_buffer.reg));
        } return sizeof(twi_motor.twi_motor_buffer.value.state);
        case TWI_motor_TACHO_CALIB_PERIOD: {
          twi_motor.twi_motor_buffer.value.tacho_calib_period =
            MotorsController::motorscontroller->min_periods[
              get_item_field(twi_motor.twi_motor_buffer.reg)];
        } return sizeof(twi_motor.twi_motor_buffer.value.tacho_calib_period);
        case TWI_motor_TACHO_AVG_PERIOD: {
          twi_motor.twi_motor_buffer.value.tacho_avg_period =
            TachoMotors::tachomotors->motors[
              get_item_field(twi_motor.twi_motor_buffer.reg)].getAvgPeriod();
        } return sizeof(twi_motor.twi_motor_buffer.value.tacho_avg_period);
        case TWI_motor_TACHO_COUNT: {
          twi_motor.twi_motor_buffer.value.tacho_count =
            TachoMotors::tachomotors->motors[
              get_item_field(twi_motor.twi_motor_buffer.reg)].getCounter();
        } return sizeof(twi_motor.twi_motor_buffer.value.tacho_count);
        case TWI_motor_PWM_M: {
          twi_motor.twi_motor_buffer.value.m =
            MotorsController::motorscontroller->m[
              get_item_field(twi_motor.twi_motor_buffer.reg)];
        } return sizeof(twi_motor.twi_motor_buffer.value.m);
        case TWI_motor_PWM_Q: {
          twi_motor.twi_motor_buffer.value.q =
            MotorsController::motorscontroller->q[
              get_item_field(twi_motor.twi_motor_buffer.reg)];
        } return sizeof(twi_motor.twi_motor_buffer.value.q);
      }
    } return 0;
  }
  return 0;
}

void createMotorThread(void) {
  chThdCreateStatic(waThreadMotor, sizeof(waThreadMotor), NORMALPRIO,
                    ThreadMotor, NULL);
}
