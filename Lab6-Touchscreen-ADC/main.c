/*
 * File:   main.c
 * Author: team-3a
 *
 * Created on March 30, 2025, 8:18 PM
 */

//do not change the order of the following 3 definitions
#define FCY 12800000UL 
#include <stdio.h>
#include <stdlib.h>
#include <libpic30.h>
#include <xc.h>
#include <p33Fxxxx.h>
#include "types.h"
#include "lcd.h"

#define DB_THRESH 6

_FOSCSEL(FNOSC_PRIPLL);

// OSC2 Pin Function: OSC2 is Clock Output - Primary Oscillator Mode: XT Crystal
_FOSC(OSCIOFNC_OFF & POSCMD_XT); 

// Watchdog Timer Enabled/disabled by user software
_FWDT(FWDTEN_OFF);

// Disable Code Protection
_FGS(GCP_OFF); 

void initADC()
{
    CLEARBIT(AD2CON1bits.ADON);
    
    // Touchscreen y-axis
    SETBIT(TRISBbits.TRISB9); //set TRISE RB9 to input
    CLEARBIT(AD2PCFGLbits.PCFG9); //set AD2 AN9 input pin as analog
    
    // Touchscreen x-axis
    SETBIT(TRISBbits.TRISB15); //set TRISE RB15 to input
    CLEARBIT(AD2PCFGLbits.PCFG15); //set AD2 AN15 input pin as analog
    
    //Configure AD2CON1
    SETBIT(AD2CON1bits.AD12B); //set 12b Operation Mode
    AD2CON1bits.FORM = 0; //set integer output
    AD2CON1bits.SSRC = 0x7; //set automatic conversion
    
    //Configure AD2CON2
    AD2CON2 = 0; //not using scanning sampling
    //Configure AD2CON3
    CLEARBIT(AD2CON3bits.ADRC); //internal clock source
    AD2CON3bits.SAMC = 0x1F; //sample-to-conversion clock = 31Tad
    AD2CON3bits.ADCS = 0x2; //Tad = 3Tcy (Time cycles)
    //Leave AD2CON4 at its default value
    
    //enable ADC
    SETBIT(AD2CON1bits.ADON);    
}

void init_timer2() {
    /* Disable Timer 2 */
    CLEARBIT(T2CONbits.TON);
    
    /* Setup Timer 2 for no interrupts, 20ms period */
    CLEARBIT(T2CONbits.TCS);
    CLEARBIT(T2CONbits.TGATE);
    TMR2 = 0x00;
    T2CONbits.TCKPS = 0b10;
    CLEARBIT(IEC0bits.T2IE);
    CLEARBIT(IFS0bits.T2IF);
    PR2 = 4000;
    
    /* Enable Timer 2 */
    SETBIT(T2CONbits.TON);   
}

void init_motor_x()
{
    /* Configure Output Compare 8 (X-axis motor) */
    CLEARBIT(TRISDbits.TRISD7); // Set OC8 = RD7 as output
    OC8R = 300; // 
    OC8RS = 300; // Next duty cycle of 1.5 ms
    OC8CONbits.OCM = 0b110; // Set PWM, no fault mode 
}

void init_motor_y()
{    
    /* Configure Output Compare 7 (Y-axis motor) */
    CLEARBIT(TRISDbits.TRISD6); 
    OC7R = 300; // 
    OC7RS = 300; // Next duty cycle of 1.5 ms
    OC7CONbits.OCM = 0b110; // Set PWM, no fault mode
}

void move_dg (int xDegree, int yDegree) 
{
    float xMs, yMs;
    float xMicro, yMicro;
    xMs = (1.0/150) * xDegree + 0.9;
    yMs = (1.0/150) * yDegree + 0.9;

    xMicro = xMs * 1000; 
    yMicro = yMs * 1000; 

    OC8RS = 4000 - (xMicro * 0.2);
    OC7RS = 4000 - (yMicro * 0.2);
}

int compare( const void* a, const void* b)
{
     int int_a = * ( (int*) a );
     int int_b = * ( (int*) b );
     
     if ( int_a == int_b ) return 0;
     else if ( int_a < int_b ) return -1;
     else return 1;
}

//0x0f for X and 0x09 for Y
int touchSample(int pin)
{
    if (pin == 0x09) {
        // Sampling y
        SETBIT(LATEbits.LATE1);
        CLEARBIT(LATEbits.LATE2);
        CLEARBIT(LATEbits.LATE3);
    } else if (pin == 0x0f){
        // Sampling x
        CLEARBIT(LATEbits.LATE1);
        SETBIT(LATEbits.LATE2);
        SETBIT(LATEbits.LATE3);
    } else {
        return -1;
    }
    
    
    int sample[5];
    int j;
    for(j = 0; j < 5; j++){
        AD2CHS0bits.CH0SA = pin; //set ADC to Sample pin
        SETBIT(AD2CON1bits.SAMP); //start to sample
        while(!AD2CON1bits.DONE); //wait for conversion to finish
        CLEARBIT(AD2CON1bits.DONE); //MUST HAVE! clear conversion done bit
        sample[j] = ADC2BUF0;
        __delay_ms(10);
    }
    qsort(sample, 5, sizeof(int), compare);
    
//    AD2CHS0bits.CH0SA = pin; //set ADC to Sample pin
//    SETBIT(AD2CON1bits.SAMP); //start to sample
//    while(!AD2CON1bits.DONE); //wait for conversion to finish
//    CLEARBIT(AD2CON1bits.DONE); //MUST HAVE! clear conversion done bit
    return sample[2]; //return sample
}

int main(void) {
    lcd_initialize();
    lcd_clear();
    initADC();
    init_timer2();
    init_motor_x();
    init_motor_y();
    
    // Init touchscreen reading
    //set up the I/O pins E1, E2, E3 to be output pins
    CLEARBIT(TRISEbits.TRISE1); //I/O pin set to output
    CLEARBIT(TRISEbits.TRISE2); //I/O pin set to output
    CLEARBIT(TRISEbits.TRISE3); //I/O pin set to output
    // Set to standby
    CLEARBIT(LATEbits.LATE1);
    CLEARBIT(LATEbits.LATE2);
    CLEARBIT(LATEbits.LATE3);
        
    float xMax = 0,
        xMin = 0,
        yMax = 0,
        yMin = 0;
    int x = 0;
    float scalar = 0;
    move_dg(90, 90);
    
    while(1)
    {        
        int xCurr, yCurr;
        
        // xMin yMin C1
        move_dg(1, 1);
        
        __delay_ms(2000);
        
        xCurr = touchSample(0x0f);
        yCurr = touchSample(0x09);
        
        lcd_locate(0,0);
        lcd_printf_d("C1: X=%d, Y= %d  ", xCurr, yCurr);
        lcd_locate(0,0);
              
        // xMax yMin C2
        move_dg(179, 1);
        
        __delay_ms(2000);
        
        xCurr = touchSample(0x0f);
        yCurr = touchSample(0x09);
        
        lcd_locate(0,1);
        lcd_printf_d("C2: X=%d, Y= %d    ", xCurr, yCurr);
        lcd_locate(0,1);
       
        // xMax yMax C3
        move_dg(179, 179);
        
        __delay_ms(2000);
        
        xCurr = touchSample(0x0f);
        yCurr = touchSample(0x09);
        
        lcd_locate(0,2);
        lcd_printf_d("C3: X=%d, Y= %d  ", xCurr, yCurr);
        lcd_locate(0,2);
       
        // xMin yMax C4
        move_dg(1, 179);
        
        __delay_ms(2000);
        
        xCurr = touchSample(0x0f);
        yCurr = touchSample(0x09);
             
        lcd_locate(0,3);
        lcd_printf_d("C4: X=%d, Y= %d  ", xCurr, yCurr);
        lcd_locate(0,3);

        }
        
    }
  
 
