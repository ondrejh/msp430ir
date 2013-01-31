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
//         |  |                 |
//          --|RST              |
//            |                 |
//            |                 |
//            |           P1.1,2|--> UART (debug output 9.6kBaud)
//            |                 |
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

#define TIMER_PRESET 10000

#define IR_BUFLEN 128
uint16_t ir_buffer[IR_BUFLEN];
int16_t ir_bufptr = -1;
bool ir_buflocked = false;

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

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	board_init(); // init dco and leds
	uart_init();  // init uart (communication)
	timer_init(); // init timer

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode (leave on rtc second event)
        LED_GREEN_ON();
        int16_t ir_cmdlen = ir_bufptr;
        ir_buflocked = true;
        ir_bufptr = -1;
        int16_t i;
        uart_putuint16(ir_cmdlen);
        uart_putc(':');
        for (i=0;i<ir_cmdlen;i++)
        {
            while(uart_ontx()) {};
            uart_putuint16(ir_buffer[i]);
            if (i<(ir_cmdlen-1)) uart_putc(';');
        }
        uart_puts("\n\r\n\r");
        LED_GREEN_OFF();
	}

	return -1;
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    uint16_t tcap = TAR;
    TACTL |= TACLR;             // clear timer
    P1IFG &= ~IR_PIN_MASK;      // clear IFG
    P1IES = (P1IN&IR_PIN_MASK); // change edge polarity
    TACTL = TASSEL_2 + MC_2;    // start timer

    if (!ir_buflocked)
    {
        if (ir_bufptr<0)
        {
            LED_RED_ON();
        }
        else
        {
            ir_buffer[ir_bufptr]=tcap;
            if (P1IN&IR_PIN_MASK) ir_buffer[ir_bufptr]|=0x8000;
        }

        ir_bufptr++;
        if (ir_bufptr==IR_BUFLEN) ir_bufptr=(IR_BUFLEN-1);
    }

    //__bic_SR_register_on_exit(CPUOFF);  // Clear CPUOFF bit from 0(SR)
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
    TACTL &= ~(MC1|MC0);    // stop timer
    LED_RED_OFF();
    __bic_SR_register_on_exit(CPUOFF);  // Clear CPUOFF bit from 0(SR)
}
