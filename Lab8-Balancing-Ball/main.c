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
#include <stdbool.h>
#include <libpic30.h>
#include <xc.h>
#include <p33Fxxxx.h>
#include "types.h"
#include "lcd.h"

#define DB_THRESH 6
#define MAX_SIZE 4 // filter order + 1

// Function to add an element to the queue (Enqueue
// operation)
void pop_and_enqueue(float q[], float value) {
    q[3] = q[2];
    q[2] = q[1];
    q[1] = q[0];
    q[0] = value;
}

_FOSCSEL(FNOSC_PRIPLL);
// OSC2 Pin Function: OSC2 is Clock Output - Primary Oscillator Mode: XT Crystal
_FOSC(OSCIOFNC_OFF & POSCMD_XT); 
// Watchdog Timer Enabled/disabled by user software
_FWDT(FWDTEN_OFF);
// Disable Code Protection
_FGS(GCP_OFF); 

// Constants
//int X_SETPOINT = 1600; // vibed mid piont
//int X_SETPOINT = 1600;
int X_SETPOINT; // originally 1600 2300 works // Don't set it here but this is done inside the main
int Y_SETPOINT; // originally 1365, believed mid point need to recalibrate each time? 
//const float KP_x = 0.04; // KP = max tile(deg) / max error (count) = 90 / 1200 = 0.075, 1200 bc X_SETPOINT - X_MIN
//const float KD_x = 0.009;
const int F_RANGE = 2000;

const float KP_x = 0.06; // KP = max range(fx) / max error (count) = original .055
const float KI_x = 0.005;         // this one works for independent case
const float KD_x = 0.05;

const float KP_y = 0.06; // KP = max range(fy) / max error (count) = 
const float KI_y = 0.005; 
const float KD_y = 0.06;


// Coefficients for freq 10
//float B[] = {0.01809893, 0.0542968, 0.0542968, 0.01809893};
//float A[] = {1., -1.76004188, 1.18289326, -0.27805992};

// Coefficients for freq 5
float B[] = {0.00289819, 0.00869458, 0.00869458, 0.00289819};
float A[] = {1.0,-2.37409474, 1.92935567, -0.53207537};

float prev_error_x = 0;
float prev_error_y = 0;

float input_history_x[] = {0, 0, 0, 0};
float output_history_x[] = {0, 0, 0, 0};
float input_history_y[] = {0, 0, 0, 0}; 
float output_history_y[] = {0, 0, 0, 0};

// filter function
float filter(int val, int check_x){ // iff 1 then x otherwise y 
    float filtered_val = 0.0; 
    
    if (check_x == 1){
        pop_and_enqueue(input_history_x, (float)val);
        
        int i;
        for(i = 0; i < MAX_SIZE; i++){
            filtered_val += B[i] * input_history_x[i];
        }
        for(i = 0; i < MAX_SIZE -1; i++){
            filtered_val -= A[i + 1] * output_history_x[i];
        }
        pop_and_enqueue(output_history_x, filtered_val);
        
    } else {
        pop_and_enqueue(input_history_y, (float)val);
        
        int i;
        for(i = 0; i < MAX_SIZE; i++){
            filtered_val += B[i] * input_history_y[i];
        }
        for(i = 0; i < MAX_SIZE -1; i++){
            filtered_val -= A[i + 1] * output_history_y[i];
        }
        pop_and_enqueue(output_history_y, filtered_val);
    }
    
    return filtered_val;
}

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

// For interrupt (to sample and PID)
void init_timer1() {
    /* Disable Timer 1 */
    CLEARBIT(T1CONbits.TON);
    
    /* Setup Timer 1 for interrupts, 50ms period */
    CLEARBIT(T1CONbits.TCS);
    CLEARBIT(T1CONbits.TGATE);
    TMR1 = 0x00;
    T1CONbits.TCKPS = 0b10;
    SETBIT(IEC0bits.T1IE);
    CLEARBIT(IFS0bits.T1IF);
    PR1 = 10000;
    /* Enable Timer 1 */
    SETBIT(T1CONbits.TON);   
}

// For motor (iirc)
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
    // Clamps between 10 and 170 degrees
    xDegree = (xDegree > 170) ? 170 : xDegree;
    xDegree = (xDegree < 10) ? 10 : xDegree;
    yDegree = (yDegree > 170) ? 170 : yDegree;
    yDegree = (yDegree < 10) ? 10 : yDegree;
    
    // Do math
    float xMs, yMs;
    float xMicro, yMicro;
    xMs = (1.0/150) * xDegree + 0.9;
    yMs = (1.0/150) * yDegree + 0.9;

    xMicro = xMs * 1000; 
    yMicro = yMs * 1000; 

    OC8RS = 4000 -(int)(xMicro * 0.2);
    OC7RS = 4000 - (int)(yMicro * 0.2);
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
    
    __delay_ms(10); // have to have so that the register can be prepared before reading
    AD2CHS0bits.CH0SA = pin; //set ADC to Sample pin
    SETBIT(AD2CON1bits.SAMP); //start to sample
    while(!AD2CON1bits.DONE); //wait for conversion to finish
    CLEARBIT(AD2CON1bits.DONE); //MUST HAVE! clear conversion done bit
    return ADC2BUF0; //return sample
}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void) {
    // Clear interrupt flag bit
    CLEARBIT(IFS0bits.T1IF);
    
    // Read ball position
    int curr_x = touchSample(0x0f);
    int curr_y = touchSample(0x09);
    
    //////////////////////////////////////////
    //          Apply filter                //
    //                                      //
    /////////////////////////////////////////
    float filtered_x = filter(curr_x, 1); // filter(val , check_x)
    float filtered_y = filter(curr_y, 0);
    
    //////////////////////////////////////////
    // PD Control: Calc force after filter //
    //                                     //
    /////////////////////////////////////////    
    float dt = 0.05;

    float curr_error_x = (float) X_SETPOINT - filtered_x;
    float de_x = (curr_error_x - prev_error_x) / dt; // 0.05s because 50ms interrupts
    prev_error_x = curr_error_x;
    
    float curr_error_y = (float) Y_SETPOINT - filtered_y; 
    float de_y = (curr_error_y - prev_error_y) / dt;
    prev_error_y = curr_error_y;

    // Conversion calculations    
    // Note: KI is incorrectly calculated, should be change of errors over time
    float f_x = (KP_x * curr_error_x) + (KI_x * curr_error_x) + (KD_x * de_x); // Still does not scale properly with different values of kp
    float f_y = (KP_y * curr_error_y) + (KI_y * curr_error_y) + (KD_y * de_y);
    
    // for both position the angles x and y the thing that happens is that at the right side of things the force values is not reaching 10 but the lower limit is actuall yhigher 
    // Start at small value but scale more
    // Move the motors
    float deg_x = f_x * (180.0/F_RANGE) + 90;
    float deg_y = f_y * (180.0/F_RANGE) + 90; //this is way off
 
    move_dg((int)(deg_x), (int)(deg_y));
}

void calibrate() {
    int sample_x_min[5];
    int sample_x_max[5];
    int sample_y_min[5];
    int sample_y_max[5];
    int i; 
    
    move_dg(0, 0);
    __delay_ms(4000);
    for(i = 0; i < 5; i++) {
        sample_x_min[i] = touchSample(0x0f); // X
        sample_y_min[i] = touchSample(0x09); // Y
    }
    
    qsort(sample_x_min, 5, sizeof(int), compare);
    qsort(sample_y_min, 5, sizeof(int), compare);
    int x_min = sample_x_min[2];
    int y_min = sample_y_min[2];
    
    lcd_locate(0,0);
    lcd_printf_d("(0,0) Done");
    
    // Corner 2
    move_dg(180, 180);
    __delay_ms(4000);
            
    for (i = 0; i < 5; i++) {
        sample_x_max[i] = touchSample(0x0f); // X
        sample_y_max[i] = touchSample(0x09); // Y
    }
    
    qsort(sample_x_max, 5, sizeof(int), compare);
    qsort(sample_y_max, 5, sizeof(int), compare);
    int x_max = sample_x_max[2];
    int y_max = sample_y_max[2];
    
    X_SETPOINT = x_min + (int)((x_max - x_min)/(2.0))+ 600;
    Y_SETPOINT = y_min + (int)((y_max - y_min)/(2.0));
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
    
    // Calibrate Set Point
//    calibrate();
    X_SETPOINT = 5000;
    Y_SETPOINT = 3340;

    lcd_locate(0,0);
    lcd_printf_d("X-Set: %d    ", X_SETPOINT);
    lcd_locate(0,1);
    lcd_printf_d("Y-Set: %d    ", Y_SETPOINT);    
    
    // Start interrupts
    init_timer1();
    
    while(1){ }
}
  
 
