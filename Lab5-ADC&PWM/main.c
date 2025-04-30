/*
 * File:   main.c
 * Author: team-3a
 *
 * Created on March 30, 2025, 8:18 PM
 */

//do not change the order of the following 3 definitions
#define FCY 12800000UL 
#include <stdio.h>
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

unsigned int triggerState = 1;
unsigned int samplesCount = 0;
unsigned int counter = 0;

void joystickInit(void)
{
    SETBIT(AD1PCFGHbits.PCFG20); // ADC1 Port Configuration Register High Channel 20
    SETBIT(TRISEbits.TRISE8);   // PortE Pin 8 I/O Configuration (TRIGGER)    
}

void initADC()
{
    CLEARBIT(AD2CON1bits.ADON);
    
    //x-axis
    SETBIT(TRISBbits.TRISB4); //set TRISE RB4 to input
    CLEARBIT(AD2PCFGLbits.PCFG4); 
    
    //y-axis
    SETBIT(TRISBbits.TRISB5); //set TRISE RB4 to input
    CLEARBIT(AD2PCFGLbits.PCFG4);
    
    //Configure AD1CON1
    CLEARBIT(AD2CON1bits.AD12B); //set 10b Operation Mode
    AD2CON1bits.FORM = 0; //set integer output
    AD2CON1bits.SSRC = 0x7; //set automatic conversion
    
    //Configure AD1CON2
    AD2CON2 = 0; //not using scanning sampling
    //Configure AD1CON3
    CLEARBIT(AD2CON3bits.ADRC); //internal clock source
    AD2CON3bits.SAMC = 0x1F; //sample-to-conversion clock = 31Tad
    AD2CON3bits.ADCS = 0x2; //Tad = 3Tcy (Time cycles)
    //Leave AD1CON4 at its default value
    //enable ADC
    SETBIT(AD2CON1bits.ADON);
    
    
}

//0x04 for X and 0x05 for Y
int joystickSample(int sample)
{
    AD2CHS0bits.CH0SA = sample; //set ADC to Sample AN20 pin
    SETBIT(AD2CON1bits.SAMP); //start to sample
    while(!AD2CON1bits.DONE); //wait for conversion to finish
    CLEARBIT(AD2CON1bits.DONE); //MUST HAVE! clear conversion done bit
    return ADC2BUF0; //return sample
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

int main(void) {
    joystickInit();
    lcd_initialize();
    lcd_clear();
    initADC();
        
    int xMax = 0,
        xMin = 0,
        yMax = 0,
        yMin = 0;
    int x = 0;
    float scalar = 0;
    
    while(1)
    {
        unsigned int currTriggerState = PORTEbits.RE8; // Trigger on 0 for PRESSED, 1 NOT PRESSED

        // DEBOUNCING START
        if(currTriggerState != triggerState)
        {
            samplesCount++;
        }
        else
        {
            samplesCount = 0;
        }

        if(samplesCount > DB_THRESH)
        {
            triggerState = currTriggerState;
            samplesCount = 0;

            if(triggerState == 0) //TRIGGER IS CONFIRMED TO PRESS
            {
                counter++;

            }
        }
        
        int xCurr;
        int yCurr;
        float xDegree, yDegree; 
        float xMs, yMs;
        float xMicro, yMicro;
        
        switch(counter)
        {
            case 0:
                xMax = joystickSample(0x04);
                lcd_locate(0,0);
                lcd_printf("xMax: %d  ", xMax);
                __delay_ms(10);
                lcd_locate(0,0);

                break;
            case 1:
                xMin = joystickSample(0x04);
                lcd_locate(0,1);
                lcd_printf("xMin: %d  ", xMin);
                __delay_ms(10);
                lcd_locate(0,1);

                break;
            case 2:
                yMax = joystickSample(0x05);
                lcd_locate(0,2);
                lcd_printf("yMax: %d  ", yMax);
                __delay_ms(10);
                lcd_locate(0,2);

                break;
            case 3:
                yMin = joystickSample(0x05);
                lcd_locate(0,3);
                lcd_printf("yMin: %d  ", yMin);
                __delay_ms(10);
                lcd_locate(0,3);

                break;
            case 4:
                // init motor 
                init_timer2();
                init_motor_x();
                init_motor_y();
                counter++;
                break;
                
            case 5:
                xCurr = joystickSample(0x04);
                yCurr = joystickSample(0x05);
                
                xDegree = ((float)(xCurr - xMin)/(xMax - xMin))*180;
                yDegree = ((float)(yCurr - yMin)/(yMax - yMin))*180;
                
                xMs = (1.0/150) * xDegree + 0.9;
                yMs = (1.0/150) * yDegree + 0.9;
                
                xMicro = xMs * 1000; 
                yMicro = yMs * 1000; 

                if (xMicro > 2100){
                    xMicro = 2100;
                } else if (xMicro < 900){
                    xMicro = 900;
                }
                if (yMicro > 2100){
                    yMicro = 2100;
                } else if (yMicro < 900){
                    yMicro = 900;
                }
                
                OC8RS = 4000 - (xMicro * 0.2);
                OC7RS = 4000 - (yMicro * 0.2);
                
                lcd_locate(0,4);
                lcd_printf("x pw: %.1f  ", xMicro);
                __delay_ms(10);
                lcd_locate(0,4);
                break;
                
            case 6:
                yCurr = joystickSample(0x05);
                
                yDegree = ((float)(yCurr - yMin)/(yMax - yMin))*180;
                
                yMs = (1.0/150) * yDegree + 0.9;
                
                yMicro = yMs * 1000; 
                
                if (yMicro > 2100){
                    yMicro = 2100;
                } else if (yMicro < 900){
                    yMicro = 900;
                }
                
                OC7RS = 4000 - (yMicro * 0.2);
                
                lcd_locate(0,5);
                lcd_printf("y pw: %.1f  ", yMicro);
                __delay_ms(10);
                lcd_locate(0,5);
                break;
                
            default:
                Nop();
                
//                lcd_locate(0,4);
//                lcd_printf("xMs: %.2f, yMs: %.2f", xMs, yMs);
//                lcd_locate(0,4);
//                __delay_ms(1);
        }
        
    }
  
    
}
