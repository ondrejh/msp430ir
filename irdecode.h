/** irdecode module header **/

#ifndef __IRDECODE_H__
#define __IRDECODE_H__

#include <inttypes.h>
#include <stdbool.h>

/// ir receiver input
#define IR_PIN_MASK BIT7

/// ir receiver timer presets
#define IRCODE_TIMEOUT 25000 // max time to capture code
#define IRCODE_PAUSE 10000   // min pause between codes

/// ir decode status
#define IR_STAT_WAIT    0
#define IR_STAT_GETTING 1
#define IR_STAT_USING   2
#define IR_STAT_DONE    3

/// length of ir buffer
#define IR_BUFLEN 64

/// ir buffer data structure
typedef struct ir_buf_struct
{
    uint16_t status;
    uint16_t buffer[IR_BUFLEN];
    uint16_t bufptr;
} ir_buf_t;

/// interface functions

void irdecode_init(void);

bool is_ircode_present(void);
void ircode_mark_used(void);

#ifdef DEBUG
void ircode_uart_send(void);
#endif

int8_t ircode_decode();

#endif
