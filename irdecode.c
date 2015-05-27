/**
 * irdecode module gets and decodes ir code
 *
 *
 * how it works:
 *
 *  0) counter is cleared, pin interrupt ready
 *  1) when pin interrupt happen it starts timer with IRCODE_TIMEOUT
 *  2) when next pin interrupt happen it saves timer value into buffer and increments counter
 *      and so on with all pin interrupts, until ...
 *  3) timer interrupt happen sets code ready flag and falls out of the sleep mode
 *  4) after reading and decoding (using) code in the main program call ircode_marked_used()
 *      it marked code as used and starts timer with IRCODE_PAUSE
 *  5) if the pin interrupt happen in this state restart timer with IRCODE_PAUSE again
 *  6) when timer timeout interrupt happen clear counter and get ready for (1)
 *
 *
 * uses pin interrupt and timer 0 (with interrupt)
 *
 **/


/// includes

#include <msp430g2553.h>
//#include <msp430g2452.h>
//#include <msp430g2201.h>
#include "irdecode.h"
#include "uart.h"

#include "ircodes.h"

/// defines

// ir code variance (maximum difference of code pulse)
#define IRVARIANCE 150

// ir receiver signalisation led
#define IRLED_INIT() {P1DIR|=0x01;P1OUT&=~0x01;}
#define IRLED_ON() {P1OUT|=0x01;}
#define IRLED_OFF() {P1OUT&=~0x01;}


/// local functions

// ir buffer context
ir_buf_t ir_buf;

// timer initialization
void timer_init(void)
{
    CCTL0 = CCIE;               // CCR0 interrupt enabled
    CCR0 = IRCODE_TIMEOUT;
    TACTL = TASSEL_2 + MC_0;    // SMCLK, stop
}

// timer restart
void timer_restart(timeout)
{
    CCR0 = timeout;
    TACTL |= TACLR;             // clear timer
    TACTL = TASSEL_2 + MC_2;    // start timer
}

// ir input initialization (with interrupt)
void irpin_init(void)
{
	P1DIR &= ~IR_PIN_MASK; // input
	P1REN |=  IR_PIN_MASK; // pullup
	P1IES |=  IR_PIN_MASK; // hi/lo edge (first)
    P1IE  |=  IR_PIN_MASK; // interrupt enable
    P1IFG &= ~IR_PIN_MASK; // clear interrupt flag
}


/// global functions (interface)

// ir decode module init
void irdecode_init(void)
{
    IRLED_INIT();

    ir_buf.status = 0;

    irpin_init();
    timer_init();
}

// ir code received function (return true if code present, false otherwise)
bool is_ircode_present(void)
{
    return (ir_buf.status==IR_STAT_USING);
}

// ir code mark as used
void ircode_mark_used(void)
{
    ir_buf.status = IR_STAT_DONE;
    timer_restart(IRCODE_PAUSE);
}

#ifdef DEBUG
// ir code send via uart
void ircode_uart_send(void)
{
    uint16_t i;

    uart_putuint16(ir_buf.bufptr);
    uart_putc(':');

    for (i=0;i<ir_buf.bufptr;i++)
    {
        while(uart_ontx()) {};
        uart_putuint16(ir_buf.buffer[i]);
        if (i<(ir_buf.bufptr-1)) uart_putc(';');
    }

    uart_puts("\r\n");
}
#endif

// decode ir code (according to ircodes.h)
// return code value if code fits or -1 if no code fits
int8_t ircode_decode(void)
{
    int8_t code,pulse;

    // copy of the data pointer
    const uint16_t *codesptr = ircode_data;

    for (code=0;code<IRCODES;code++)
    {
        uint16_t codelen = *codesptr++;
        bool codepass = true;

        if (codelen!=ir_buf.bufptr) codepass = false;

        for (pulse=0;pulse<codelen;pulse++)
        {
            // even if the code doesn't fit read through all pulses
            uint16_t thispulse = *codesptr++;
            // test code pulses
            if (codepass)
            {
                // if code doesn't fit set flag
                if (thispulse<(ir_buf.buffer[pulse]-IRVARIANCE)) codepass=false;
                if (thispulse>(ir_buf.buffer[pulse]+IRVARIANCE)) codepass=false;
            }
        }

        if (codepass) break;
    }

    if (code!=IRCODES) return(code);

    return -1; // no code detected
}

/// interrupt routines

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    uint16_t tcap = TAR;
    uint8_t next_mask = P1IN&IR_PIN_MASK;

    P1IES = next_mask;      // change edge polarity
    P1IFG &= ~IR_PIN_MASK;  // clear IFG

    switch (ir_buf.status)
    {
        case IR_STAT_WAIT:
            timer_restart(IRCODE_TIMEOUT);
            IRLED_ON();
            ir_buf.bufptr=0;
            ir_buf.status=IR_STAT_GETTING;
            break;
        case IR_STAT_GETTING:
            //if (next_mask!=0) tcap|=0x8000;
            ir_buf.buffer[ir_buf.bufptr++]=tcap;
            if (ir_buf.bufptr==IR_BUFLEN) ir_buf.bufptr=(IR_BUFLEN-1);
            break;
        case IR_STAT_DONE:
            timer_restart(IRCODE_PAUSE);
            break;
        default:
            break;
    }
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
    TACTL &= ~(MC1|MC0);    // stop timer
    switch (ir_buf.status)
    {
        case IR_STAT_GETTING:
            IRLED_OFF();
            ir_buf.status=IR_STAT_USING;
            __bic_SR_register_on_exit(CPUOFF);  // Clear CPUOFF bit from 0(SR)
            break;
        case IR_STAT_DONE:
            ir_buf.status=IR_STAT_WAIT;
            break;
        default:
            break;
    }
}

