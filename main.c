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
//            |             P2.2|--> PWM (Output)
//            |                 |
//            |             P2.3|--> LED
//            |             P2.4|<-- BTN 1
//            |             P2.5|<-- BTN 2
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

#define BTN_LED_INIT() do{P2DIR|=0x08;P2OUT&=~0x08;P2DIR&=~0x30;}while(0)
#define LED_ON() do{P2OUT|=0x08;}while(0)
#define LED_OFF() do{P2OUT&=~0x08;}while(0)
#define BTN1 ((P2IN&0x10)==0)
#define BTN2 ((P2IN&0x20)==0)

#define OUT_INIT() do{P2DIR|=0x04;P2OUT&=~0x04;}while(0)
#define OUT_ON() do{P2OUT|=0x04;}while(0)
#define OUT_OFF() do{P2OUT&=~0x04;}while(0)


// timeout timer for led swithing off (uses wdt)
#define LED_TIMEOUT_PRESET 25
uint8_t led_timeout = 0;

uint16_t pwm = 0;

// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;

    // leds
	LED_INIT();
	BTN_LED_INIT();
	OUT_INIT();
}


// init timer (wdt used)
void wdt_timer_init(void)
{
    WDTCTL = WDT_MDLY_8;                    // Set Watchdog Timer interval to ~8ms
    IE1 |= WDTIE;                             // Enable WDT interrupt
}

#define PWM_MAX 0x80

void timer1_init(void)
{
    P2SEL|=0x04;
    P2SEL2&=~0x04;

    TA1CTL = TASSEL_2 | ID_0 | MC_1;
    TA1CCR0 = PWM_MAX;
    TA1CCTL1 = OUTMOD_7;
    TA1CCR1 = 0x0000;
}

#define set_pwm(x) do{TA1CCR1=x;}while(0)

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

	timer1_init();

	uint16_t pwm = 0,pwm_preset = 0;

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode
        int8_t code = -1;
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
            code = ircode_decode();
            if (code>=0)
            {
                led_timeout = LED_TIMEOUT_PRESET;
                LED_GREEN_ON();
            }
            ircode_mark_used();
            #endif
        }

        if ((BTN1) || (code==1)) {
            LED_ON();
            pwm_preset = 0x80;
        }
        if ((BTN2) || (code==2)) {
            LED_OFF();
            pwm_preset = 0;
        }

        if (pwm<pwm_preset)
            pwm++;
        if (pwm>pwm_preset)
            pwm--;

        set_pwm(pwm);

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

    __bic_SR_register_on_exit(CPUOFF);  // Clear CPUOFF bit from 0(SR)
}
