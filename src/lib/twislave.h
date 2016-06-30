/*
    Two-Wire Slave driver for use with ChibiOS

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


#ifndef _SLAVE_H_
#define _SLAVE_H_
#include "ch.h"
#include "hal.h"
#include "iterators.h"


#define TWI_HANDLER_WRITE 0
#define TWI_HANDLER_READ  1

#define GENERAL_CALL_ADDRESS_TRUE 1
#define GENERAL_CALL_ADDRESS_FALSE 0

#define PARM_MASK (_BV(7) | _BV(6) | _BV(5) | _BV(4))
#define reset_parm_field(reg) ((reg) & ~PARM_MASK)
#define set_parm_field(reg, parm) (((parm) << 4) | reset_parm_field(reg))
#define get_parm_field(reg) ((reg) >> 4)

#define ITEM_MASK (_BV(3) | _BV(2) | _BV(1) | _BV(0))
#define reset_item_field(reg) ((reg) & ~ITEM_MASK)
#define set_item_field(reg, item) ((item ) | reset_item_field(reg))
#define get_item_field(reg) ((reg) & ITEM_MASK)

typedef uint8_t (*twi_handler)(uint8_t mode);


typedef struct {
  twi_handler handler;
  uint8_t parm_low;
  uint8_t parm_high;
  uint8_t data_total_len;
  uint8_t data_counter;
  uint8_t data[0];
} __attribute__((__packed__))
  __attribute__((__may_alias__)) TWI_Buffer;

typedef struct {
  uint16_t items_nr;
  TWI_Buffer *items[0];
} __attribute__((__packed__))
  __attribute__((__may_alias__)) TWI_Buffers;


// Not for direct use.
#define twi_buf_type(name) \
  typedef struct { \
    TWI_Buffer twi_buffer; \
    TWI##name##Buffer twi_##name##_buffer; \
  } __attribute__((__packed__)) \
    __attribute__((__may_alias__)) TWI_Buffer_##name; \

// Not for direct use.
// Creates the descriptor for the ranged TWI handler
#define DECLARE_TWI_HANDLER(name) \
  extern uint8_t twi_##name##_handler(uint8_t mode); \
  twi_buf_type(name) \
  TWI_Buffer_##name twi_##name = { \
    .twi_buffer = { \
      .parm_low = TWI_##name##_START_RANGE, \
      .parm_high = TWI_##name##_END_RANGE, \
      .data_total_len = sizeof(TWI##name##Buffer), \
      .data_counter = 0, \
    }, \
  };

// To be called during the initialization of the TWI handler.
#define twi_runtime_handler_init(name) { \
  ((TWI_Buffer*)(&twi_##name))->handler = &twi_##name##_handler; \
}
// Not for direct use.
#define extern_twi_buf(name) \
  extern TWI_Buffer_##name twi_##name;

// Not for direct use.
#define addr_to_twi_buf(name) \
  ((TWI_Buffer*)(&(twi_##name)))

// To be called from a centralized place, with all the TWI handlers.
#define DECLARE_TWI_HANDLERS(...) \
  ITERATE_SEMICOLON(DECLARE_TWI_HANDLER, __VA_ARGS__) \
  ITERATE_SEMICOLON(extern_twi_buf, __VA_ARGS__)      \
  typedef struct { \
    TWI_Buffers twi_buffers; \
    TWI_Buffer *items[VA_NARGS(__VA_ARGS__)]; \
  } __attribute__((__packed__)) \
  __attribute__((__may_alias__)) TWI_Buffers_real; \
  TWI_Buffers_real twi_buffers= { \
    .twi_buffers = { \
      .items_nr = VA_NARGS(__VA_ARGS__), \
    }, \
    .items = { \
      ITERATE_COMMA(addr_to_twi_buf, __VA_ARGS__)  \
    }, \
  }; \
  TWI_Buffers *twi_bf = (TWI_Buffers*)&twi_buffers;

#define DECLARE_EXTERN_TWI_HANDLERS(...) \
  ITERATE_SEMICOLON(twi_buf_type, __VA_ARGS__)      \
  ITERATE_SEMICOLON(extern_twi_buf, __VA_ARGS__)      \
  typedef struct { \
    TWI_Buffers twi_buffers; \
    TWI_Buffer *items[VA_NARGS(__VA_ARGS__)]; \
  } __attribute__((__packed__)) \
  __attribute__((__may_alias__)) TWI_Buffers_real; \
  extern TWI_Buffers_real twi_buffers;

#define TWI_CREATE_COMMANDS(name, start, ...) \
  typedef enum { \
    TWI_##name##_START_RANGE = start, \
    TWI_##name##_DUMMY_START = TWI_##name##_START_RANGE - 1, \
    ITERATE_PREPEND(TWI_, ITERATE_PREPEND(name##_, __VA_ARGS__)), \
    TWI_##name##_DUMMY_END, \
    TWI_##name##_END_RANGE = TWI_##name##_DUMMY_END - 1, \
  }TWI_##name##_COMMANDS

extern TWI_Buffers *twi_bf;
extern void twi_get_bf_addr(void);
extern void *tester(void);
void twi_initialise(const uint8_t slave_address, const uint8_t general);
#endif
