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

#include <avr/io.h>
#include <avr/interrupt.h>
#include "twislave.h"
#include "ch.h"
#include "hal.h"

#define TWCR_SEND_ACK (_BV(TWEN) | _BV(TWIE) |_BV(TWINT) | _BV(TWEA))
#define TWCR_SEND_NACK (_BV(TWEN) | _BV(TWIE) |_BV(TWINT))


// TWI Slave Transmitter status codes
#define TWI_STX_ADR_ACK            0xA8  // Own SLA+R has been received; ACK has been returned
#define TWI_STX_DATA_ACK           0xB8  // Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_NACK          0xC0  // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8  // Last data byte in TWDR has been transmitted (TWEA = �0�); ACK has been received

// TWI Slave Receiver status codes
#define TWI_SRX_ADR_ACK            0x60  // Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST 0x68  // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWI_SRX_GEN_ACK            0x70  // General call address has been received; ACK has been returned
#define TWI_SRX_GEN_ACK_M_ARB_LOST 0x78  // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_ACK       0x80  // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_NACK      0x88  // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWI_SRX_GEN_DATA_ACK       0x90  // Previously addressed with general call; data has been received; ACK has been returned
#define TWI_SRX_GEN_DATA_NACK      0x98  // Previously addressed with general call; data has been received; NOT ACK has been returned
#define TWI_SRX_STOP_RESTART       0xA0  // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWI Miscellaneous status codes
#define TWI_NO_STATE               0xF8  // No relevant state information available; TWINT = �0�
#define TWI_BUS_ERROR              0x00  // Bus error due to an illegal START or STOP condition

typedef enum {
  WAITING_FOR_ADDRESS = 0,
  WAITING_FOR_COMMAND,
  WAITING_FOR_WRITE,
  WAITING_FOR_READ,
  WAITING_FOR_STOP,
  WAITING_FOR_RESTART_OR_WRITE,
} States;

static volatile uint8_t status;
static volatile TWI_Buffer *active_buffer;

void
twi_initialise(const uint8_t slave_address, const uint8_t general) {
  TWAR = (slave_address << 1) | general;
  status = WAITING_FOR_ADDRESS;
  TWCR = _BV(TWINT);
  TWCR = TWCR_SEND_ACK;
}

ISR(TWI_vect) {
  static uint8_t answer_len;
  switch (TWSR) {
    case TWI_SRX_GEN_ACK:         // Gen Address is Write Only
    case TWI_SRX_ADR_ACK:         // Got Write Address
      if (status == WAITING_FOR_ADDRESS)
        status = WAITING_FOR_COMMAND;
      else
        status = WAITING_FOR_ADDRESS;
      TWCR = TWCR_SEND_ACK;
      break;

    case TWI_SRX_GEN_DATA_ACK:
    case TWI_SRX_ADR_DATA_ACK:    // Got Data
      switch (status) {
        case WAITING_FOR_COMMAND: {
          register uint8_t i;
          for (i = 0; i < twi_bf->items_nr; i++) {
            active_buffer = twi_bf->items[i];
            register uint8_t parm = get_parm_field(TWDR);
            if ((active_buffer->parm_low <= parm) &&
                (active_buffer->parm_high >= parm)){
              active_buffer->data[0] = TWDR;
              active_buffer->data_counter = 1;
              status = WAITING_FOR_RESTART_OR_WRITE;
              TWCR = TWCR_SEND_ACK;
              return;
            }
          }
          status = WAITING_FOR_COMMAND;
          TWCR = TWCR_SEND_NACK;
        } break;
        case WAITING_FOR_RESTART_OR_WRITE:
          status = WAITING_FOR_WRITE;
        case WAITING_FOR_WRITE:
          active_buffer->data[active_buffer->data_counter++] = TWDR;
          if ((active_buffer->data_counter) ==
              (active_buffer->data_total_len)) {
            status = WAITING_FOR_STOP;
            TWCR = TWCR_SEND_NACK;
          }
          else
            TWCR = TWCR_SEND_ACK;
          break;
        default:
          status = WAITING_FOR_ADDRESS;
          TWCR = TWCR_SEND_ACK;
      }
      break;

    case TWI_SRX_STOP_RESTART:
      switch (status) {
        case WAITING_FOR_WRITE:
        case WAITING_FOR_STOP:
          (*(active_buffer->handler))(TWI_HANDLER_WRITE);
        case WAITING_FOR_RESTART_OR_WRITE:
        default:
          status = WAITING_FOR_ADDRESS;
          TWCR = TWCR_SEND_ACK;
      }
      break;

    case TWI_STX_ADR_ACK:
      switch (status) {
        case WAITING_FOR_ADDRESS:
          answer_len = 1 + (*(active_buffer->handler))(TWI_HANDLER_READ);
          active_buffer->data_counter = 1;
          TWDR = active_buffer->data[active_buffer->data_counter++];
          if (active_buffer->data_counter ==
              answer_len)
            status = WAITING_FOR_STOP;
          else
            status = WAITING_FOR_READ;
          TWCR = TWCR_SEND_ACK;
          break;
        default:
          status = WAITING_FOR_ADDRESS;
          TWCR = TWCR_SEND_ACK;
      }
      break;

    case TWI_STX_DATA_ACK:
      switch (status) {
        case WAITING_FOR_READ:
          TWDR = active_buffer->data[active_buffer->data_counter++];
          if (active_buffer->data_counter ==
              answer_len)
            status = WAITING_FOR_STOP;
          TWCR = TWCR_SEND_ACK;
          break;
        default:
          status = WAITING_FOR_ADDRESS;
          TWCR = TWCR_SEND_ACK;
      }
      break;

    case TWI_STX_DATA_NACK:
      status = WAITING_FOR_ADDRESS;
      TWCR = TWCR_SEND_ACK;
      break;

    case TWI_SRX_ADR_DATA_NACK:
    case TWI_SRX_GEN_DATA_NACK:
    case TWI_STX_DATA_ACK_LAST_BYTE:
    case TWI_BUS_ERROR:
      TWCR =   (1<<TWSTO)|(1<<TWINT);
      break;

    default:
      status = WAITING_FOR_ADDRESS;
      TWCR = TWCR_SEND_ACK;
  }
}
