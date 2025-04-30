/****************************************************/
/*                                                  */
/*   CS-454/654 Embedded Systems Development        */
/*   Instructor: Renato Mancuso <rmancuso@bu.edu>   */
/*   Boston University                              */
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



int main(){
	/* LCD Initialization Sequence */
	__C30_UART=1;	
	lcd_initialize();
	lcd_clear();
	lcd_locate(0,0);
	lcd_printf("Rigs");	
    lcd_locate(0,1);
	lcd_printf("Brian");	
    lcd_locate(0,2);
	lcd_printf("Joey");	
    // Part 1:  
    
//    led_initialize();
    CLEARBIT(LED4_TRIS); //set Pin to Output
    CLEARLED(LED4_PORT); //clear if has avlue already
    int i;  //looper
    for(i = 0; i <= 2; i++){
        SETLED(LED4_PORT); // turn on
        __delay_ms(100); // stay on for 100 ms
        TOGGLELED(LED4_PORT); // turn off
        __delay_ms(1000); // stay off for exactly 1 second
    }
	
    //Problem 3
    CLEARBIT(LED1_TRIS); //set Pin to Output
    CLEARLED(LED1_PORT); //clear if has avlue already
    
    CLEARBIT(LED2_TRIS); //set Pin to Output
    CLEARLED(LED2_PORT);
    
    CLEARBIT(LED3_TRIS); //set Pin to Output
    CLEARLED(LED3_PORT);
        
    SETBIT(TRISEbits.TRISE8); //PortE Pin 8 I/O Configuration        
    SETBIT(TRISDbits.TRISD10);
    SETBIT(AD1PCFGHbits.PCFG20);
    
    uint8_t btn1_cnt = 0;
    
    lcd_locate(0,3);
    lcd_printf("%d", btn1_cnt);
    
    while (1) {
        // Button 1
        if (PORTEbits.RE8 == 0) {
            btn1_cnt += 1;
            lcd_locate(0,3);
            lcd_printf("%d", btn1_cnt);	
        
            SETLED(LED1_PORT);
            __delay_ms(100);
            while(PORTEbits.RE8 == 0){
                // Button 2
                if (PORTDbits.RD10 == 0) {
                    SETLED(LED2_PORT); 
                } else {
                    CLEARLED(LED2_PORT);
                }
                // Q 5
                if (PORTEbits.RE8 == PORTDbits.RD10) {
                    CLEARLED(LED3_PORT); 
                } else {
                    SETLED(LED3_PORT);
                }
                __delay_ms(100);
            }
            
        } else {
            CLEARLED(LED1_PORT);
        }
        // Button 2
        if (PORTDbits.RD10 == 0) {
            SETLED(LED2_PORT); 
        } else {
            CLEARLED(LED2_PORT);
        }
        // Q 5
        if (PORTEbits.RE8 == PORTDbits.RD10) {
            CLEARLED(LED3_PORT); 
        } else {
            SETLED(LED3_PORT);
        }
        
        Nop();
    }
    lcd_locate(0,3);
	lcd_printf("While exit");	
    
    return 0;
}


