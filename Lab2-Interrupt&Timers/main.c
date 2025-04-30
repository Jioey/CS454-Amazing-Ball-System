/****************************************************/
/*                                                  */
/*   CS-454/654 Embedded Systems Development        */
/*   Instructor: Renato Mancuso <rmancuso@bu.edu>   */
/*   Boston University   
 * lab 2                           */
/*                                                  */
/*   Description: simple HelloWorld application     */
/*                for Amazing Ball platform         */
/*                                                  */
/****************************************************/

#include <p33Fxxxx.h>
//do not change the order of the following 3 definitions
#define FCY 12800000UL 
#include <stdio.h>
#include <libpic30.h>
#include <uart.h>
#include <timer.h>

#include "lcd.h"
#include "led.h"

/* Initial configuration by EE */
// Primary (XT, HS, EC) Oscillator with PLL
_FOSCSEL(FNOSC_PRIPLL);

// OSC2 Pin Function: OSC2 is Clock Output - Primary Oscillator Mode: XT Crystal
_FOSC(OSCIOFNC_OFF & POSCMD_XT); 

// Watchdog Timer Enabled/disabled by user software
_FWDT(FWDTEN_OFF);

// Disable Code Protection
_FGS(GCP_OFF);  

uint32_t cnt = 0;
uint8_t ready = 0;

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void) {
    ++cnt;
    ready = 1;
    CLEARBIT(IFS0bits.T1IF);
}

void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt(void) {
    if (cnt & 0x1 == 0) {
        cnt += 2;
    }
    ready = 1;
    CLEARBIT(IFS0bits.T2IF);
}

int main() {
    lcd_clear();
    CLEARBIT(T1CONbits.TON);
    CLEARBIT(T1CONbits.TCS);
    CLEARBIT(T1CONbits.TGATE);

    CLEARBIT(T2CONbits.TON);
    CLEARBIT(T2CONbits.TCS);
    CLEARBIT(T2CONbits.TGATE);

    TMR1 = 0x00;
    T1CONbits.TCKPS = 0b11;
    PR1 = 0x61A8UL;
    IPC0bits.T1IP = 0x02;

    TMR2 = 0x00;
    T2CONbits.TCKPS = 0b11;
    PR2 = 0x61A8UL;
    IPC1bits.T2IP = 0x01;

    CLEARBIT(IFS0bits.T1IF);
    SETBIT(IEC0bits.T1IE);

    CLEARBIT(IFS0bits.T2IF);
    SETBIT(IEC0bits.T2IE);

    SETBIT(T1CONbits.TON);
    SETBIT(T2CONbits.TON);

    while(1) {
        if (ready == 1) {
            ready = 0;
            lcd_locate(0, 0);
            lcd_printf("hii");
            lcd_locate(0, 0);

        } 
            
    }

    return 0;
}