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
// alternatively the MSP430G2452(20pin) or MSP430G2201(20pin)
//    can be used (no uart)
//
//******************************************************************************

// include section
#include <msp430g2553.h>
//#include <msp430g2452.h>
//#include <msp430g2201.h>

#ifdef DEBUG
#include "uart.h"
#endif

#include "irdecode.h"

// board (leds, button)
#define LED_INIT() {P1DIR|=0x41;P1OUT&=~0x41;}
#define LED_RED_ON() {P1OUT|=0x01;}
#define LED_RED_OFF() {P1OUT&=~0x01;}
#define LED_RED_SWAP() {P1OUT^=0x01;}
#define LED_GREEN_ON() {P1OUT|=0x40;}
#define LED_GREEN_OFF() {P1OUT&=~0x40;}
#define LED_GREEN_SWAP() {P1OUT^=0x40;}


// timeout timer for led swithing off (uses wdt)
#define LED_TIMEOUT_PRESET 25
uint8_t led_timeout = 0;

// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;

    // leds
	LED_INIT();
}


// init timer (wdt used)
void wdt_timer_init(void)
{
    WDTCTL = WDT_MDLY_8;                     // Set Watchdog Timer interval to ~8ms
    IE1 |= WDTIE;                             // Enable WDT interrupt
}


// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

    #ifdef DEBUG
	uart_init();  // init uart (communication)
	#endif

	board_init(); // init dco and leds

	irdecode_init(); // init irdecode module
	wdt_timer_init(); // init wdt timer (used for led blinking)

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode (leave on rtc second event)
        if (is_ircode_present())
        {
            #ifdef DEBUG
            // debug mode prints whole received code on uart
            LED_GREEN_ON();
            ircode_uart_send();
            //uart_putint8(ircode_decode());
            //uart_puts("\r\n");
            ircode_mark_used();
            LED_GREEN_OFF();
            #else
            // normal mode tries to decode received code
            int8_t code = ircode_decode();
            if (code>=0)
            {
                led_timeout = LED_TIMEOUT_PRESET;
                LED_GREEN_ON();
            }
            ircode_mark_used();
            #endif
        }
	}

	return -1;
}

// Watchdog Timer interrupt service routine
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    if (led_timeout>0)
    {
        led_timeout--;
        if (led_timeout==0) LED_GREEN_OFF();
    }
}
