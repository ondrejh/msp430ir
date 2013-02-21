//******************************************************************************
// msp430 ir receiver
//
// author: Ondrej Hejda
// date:   2.10.2012
//
// resources:
//
//  msp430 dcf77 clock repo
//
//
// hardware: MSP430G2553 (launchpad)
//
//                MSP4302553
//             -----------------
//        /|\ |                 |
//         |  |           P1.1,2|--> UART (debug output 9.6kBaud)
//          --|RST              |
//            |             P1.0|--> RED LED (active high)
//            |             P1.6|--> GREEN LED (active high)
//            |                 |
//            |             P1.7|<-- IR receiver
//            |                 |
//
//******************************************************************************

// include section
#include <msp430g2553.h>

#include "uart.h"

// board (leds, button)
#define LED_INIT() {P1DIR|=0x41;P1OUT&=~0x41;}
#define LED_RED_ON() {P1OUT|=0x01;}
#define LED_RED_OFF() {P1OUT&=~0x01;}
#define LED_RED_SWAP() {P1OUT^=0x01;}
#define LED_GREEN_ON() {P1OUT|=0x40;}
#define LED_GREEN_OFF() {P1OUT&=~0x40;}
#define LED_GREEN_SWAP() {P1OUT^=0x40;}

// ir receiver input
#define IR_PIN_MASK BIT7

#define TIMER_PRESET 25000

#define IR_STAT_WAIT    0
#define IR_STAT_GETTING 1
#define IR_STAT_USING   2
#define IR_STAT_DONE    3
#define IR_BUFLEN 64

typedef struct ir_buf_struct
{
    uint16_t status;
    uint16_t buffer[IR_BUFLEN];
    uint16_t bufptr;
} ir_buf_t;

ir_buf_t ir_buf;

// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;

    // leds
	LED_INIT();

	// ir input (with interrupt)
	P1DIR &= ~IR_PIN_MASK; // input
	P1REN |=  IR_PIN_MASK; // pullup
	P1IES |=  IR_PIN_MASK; // hi/lo edge (first)
    P1IE  |=  IR_PIN_MASK; // interrupt enable
    P1IFG &= ~IR_PIN_MASK; // clear interrupt flag
}

// timer initialization
void timer_init(void)
{
    CCTL0 = CCIE;               // CCR0 interrupt enabled
    CCR0 = TIMER_PRESET;
    TACTL = TASSEL_2 + MC_0;    // SMCLK, stop
}

// timer restart
void timer_restart(void)
{
    TACTL |= TACLR;             // clear timer
    TACTL = TASSEL_2 + MC_2;    // start timer
}

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	ir_buf.status = 0;

	board_init(); // init dco and leds
	uart_init();  // init uart (communication)
	timer_init(); // init timer

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode (leave on rtc second event)
        if (ir_buf.status==IR_STAT_USING)
        {
            LED_GREEN_ON();
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
            TACTL |= TACLR;          // clear timer
            TACTL = TASSEL_2 + MC_2; // start timer
            ir_buf.status = IR_STAT_DONE;
            LED_GREEN_OFF();
        }
	}

	return -1;
}

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
            timer_restart();
            LED_RED_ON();
            ir_buf.bufptr=0;
            ir_buf.status=IR_STAT_GETTING;
            break;
        case IR_STAT_GETTING:
            //if (next_mask!=0) tcap|=0x8000;
            ir_buf.buffer[ir_buf.bufptr++]=tcap;
            if (ir_buf.bufptr==IR_BUFLEN) ir_buf.bufptr=(IR_BUFLEN-1);
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
            LED_RED_OFF();
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
