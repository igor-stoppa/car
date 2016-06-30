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

#ifndef MOTORS_H
#define MOTORS_H

typedef enum {
  LEFT_REAR     = 0,
  RIGHT_REAR    = 1,
  RIGHT_FRONT   = 2,
  LEFT_FRONT    = 3,
  MOTORS_NUMBER = 4,
  ALL_MOTORS    = MOTORS_NUMBER,
} Motors;

typedef enum {
  IDLE = 0,
  BACKWARD = 1,
  FORWARD = 2,
  LOCK = 3,
  MOTOR_STATES = 4,
} MotorStates;

#endif
