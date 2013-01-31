//******************************************************************************
// DCF77 home automation clock
//
// author: Ondrej Hejda
// date:   2.10.2012
//
// resources:
//
//  Using ACLK and the 32kHz Crystal .. thanks to:
//   "http://www.msp430launchpad.com/2012/03/using-aclk-and-32khz-crystal.html"
//  Interface MSP430 Launchpad with LCD Module (LCM) in 4 bit mode .. thanks to:
//   "http://cacheattack.blogspot.cz/2011/06/quick-overview-on-interfacing-msp430.html"
//  Buttons: MSP430G2xx3 Demo - Software Port Interrupt Service on P1.4 from LPM4
//
//
// hardware: MSP430G2553 (launchpad)
//
//                MSP4302553
//             -----------------
//         /|\|              XIN|----  -----------
//          | |                 |     | 32.768kHz |---
//          --|RST          XOUT|----  -----------    |
//            |                 |                    ---
//            |                 |
//            |           P1.1,2|--> UART (debug output 9.6kBaud)
//            |                 |
//            |             P1.0|--> RED LED (active high)
//            |             P1.6|--> GREEN LED (active high)
//            |                 |
//            |             P1.7|<---- IR receiver
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


// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;

	LED_INIT(); // leds
}

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	board_init(); // init dco and leds
	uart_init(); // init uart (communication)

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode (leave on rtc second event)
        LED_RED_SWAP();
	}

	return -1;
}
