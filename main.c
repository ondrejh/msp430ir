//******************************************************************************
// msp430 ir receiver
//
// author: Ondrej Hejda
// date:   10.2.2013
//
// resources:
//
//  some of my other repos
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
#include "irdecode.h"

// board (leds, button)
#define LED_INIT() {P1DIR|=0x41;P1OUT&=~0x41;}
#define LED_RED_ON() {P1OUT|=0x01;}
#define LED_RED_OFF() {P1OUT&=~0x01;}
#define LED_RED_SWAP() {P1OUT^=0x01;}
#define LED_GREEN_ON() {P1OUT|=0x40;}
#define LED_GREEN_OFF() {P1OUT&=~0x40;}
#define LED_GREEN_SWAP() {P1OUT^=0x40;}

// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;

    // leds
	LED_INIT();
}

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	uart_init();  // init uart (communication)
	board_init(); // init dco and leds

	irdecode_init(); // init irdecode module

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode (leave on rtc second event)
        if (is_ircode_present())
        {
            LED_GREEN_ON();
            ircode_uart_send();
            ircode_mark_used();
            LED_GREEN_OFF();
        }
	}

	return -1;
}

