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
#define PWM_MIN 0x20

#define DEFAULT_START_SPEED 0x50

#define BTN_SPEED_STEP 0x08
#define REMOTE_SPEED_STEP 0x02

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

#define BTN_FILTER_MAX        25
#define BTN_FILTER_THOLD_HIGH 20
#define BTN_FILTER_THOLD_LOW  15

// accelerate (button or remote)
void pwm_more(int16_t *pwm_preset, uint16_t how_much_more)
{
    *pwm_preset += how_much_more;

    if (*pwm_preset<PWM_MIN)
        *pwm_preset = PWM_MIN;

    if (*pwm_preset>PWM_MAX)
        *pwm_preset = PWM_MAX;
}

// decelerate (button or remote)
void pwm_less(int16_t *pwm_preset, uint16_t how_much_less)
{
    *pwm_preset -= how_much_less;

    if (*pwm_preset<PWM_MIN)
        *pwm_preset = 0;
}

// main program body
int main(void)
{
    int16_t fbtn1=0,fbtn2=0,fbtnboth=0;
    bool btn1=false,btn2=false,btnboth=false;
	uint16_t pwm = 0;
	int16_t pwm_preset = 0;
	uint16_t pwm_start = DEFAULT_START_SPEED;

	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

    #ifdef DEBUG
	uart_init();  // init uart (communication)
	#endif

	board_init(); // init dco and leds

	irdecode_init(); // init irdecode module
	wdt_timer_init(); // init wdt timer (used for led blinking)

	timer1_init();

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode
        int8_t code = -1;
        if (is_ircode_present())
        {
            #ifdef DEBUG
            // debug mode prints whole received code on uart
            LED_GREEN_ON();
            code = ircode_decode();
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

        // filter count
        fbtnboth += (BTN1 &&  BTN2) ? 1 : -1;
        fbtn1    += (BTN1 && !BTN2) ? 1 : -1;
        fbtn2    += (BTN2 && !BTN1) ? 1 : -1;
        // filter count limits
        bool btn1_re=false, btn2_re=false, btnboth_re=false;
        if (fbtnboth > BTN_FILTER_MAX) fbtnboth = BTN_FILTER_MAX;
        if (fbtn1    > BTN_FILTER_MAX) fbtn1    = BTN_FILTER_MAX;
        if (fbtn2    > BTN_FILTER_MAX) fbtn2    = BTN_FILTER_MAX;
        if (fbtnboth < 0) fbtnboth = 0;
        if (fbtn1    < 0) fbtn1    = 0;
        if (fbtn2    < 0) fbtn2    = 0;
        if ((btn1)    && (fbtn1<BTN_FILTER_THOLD_LOW))    btn1    = false;
        if ((btn2)    && (fbtn2<BTN_FILTER_THOLD_LOW))    btn2    = false;
        if ((btnboth) && (fbtnboth<BTN_FILTER_THOLD_LOW)) btnboth = false;
        if ((!btn1)    && (fbtn1>BTN_FILTER_THOLD_HIGH))    {btn1_re=true;    btn1=true;}
        if ((!btn2)    && (fbtn2>BTN_FILTER_THOLD_HIGH))    {btn2_re=true;    btn2=true;}
        if ((!btnboth) && (fbtnboth>BTN_FILTER_THOLD_HIGH)) {btnboth_re=true; btnboth=true;}

        // use rising edge signals
        if (btnboth_re) // start/stop (both buttons)
            pwm_preset = (pwm_preset==0) ? pwm_start : 0;
        if (btn1_re) // plus (button1)
            pwm_more(&pwm_preset,BTN_SPEED_STEP);
        if (btn2_re) // minus (button2)
            pwm_less(&pwm_preset,BTN_SPEED_STEP);

        // use remote codes
        if (code==1) pwm_preset = pwm_start; // start
        if (code==2) pwm_preset = 0; // stop
        if (code==4) pwm_more(&pwm_preset,REMOTE_SPEED_STEP); // forward (more power)
        if (code==3) pwm_less(&pwm_preset,REMOTE_SPEED_STEP); // backward (less power)
        if ((code==0) && (pwm_preset!=0)) // program .. save preset
            pwm_start = pwm_preset;

        // ramps
        if (pwm<pwm_preset) pwm++;
        if (pwm>pwm_preset) pwm--;

        // led
        if (pwm!=0) LED_ON();
        else LED_OFF();

        // output
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
