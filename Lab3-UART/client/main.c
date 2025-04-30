//do not change the order of the following 3 definitions
#define FCY 12800000UL 
#include <stdio.h>
#include <stdlib.h>
#include <libpic30.h>

#include <p33Fxxxx.h>
#include "types.h"
#include "uart.h"
#include "crc16.h"
#include "lab3.h"
#include "lcd.h"
#include "timer.h"

// Primary (XT, HS, EC) Oscillator without PLL
_FOSCSEL(FNOSC_PRIPLL);
// OSC2 Pin Function: OSC2 is Clock Output - Primary Oscillator Mode: XT Crystanl
_FOSC(OSCIOFNC_ON & POSCMD_XT);
// Watchdog Timer Enabled/disabled by user software
_FWDT(FWDTEN_OFF);
// Disable Code Protection
_FGS(GCP_OFF);   

// Timer setup for interrupts:
void init_timer1(void) {
    //enable LPOSCEN
    __builtin_write_OSCCONL(OSCCONL | 2);
    
    CLEARBIT(T1CONbits.TON); //Disable Timer
    
    SETBIT(T1CONbits.TCS); //Select external clock
    CLEARBIT(T1CONbits.TSYNC); //Disable Synchronization
    T1CONbits.TCKPS = 0b00; //Select 1:1 Prescaler
    TMR1 = 0x00; //Clear timer register
    PR1 = 32767; //Load the period value
    
    IPC0bits.T1IP = 0x01; // Set Timer1 Interrupt Priority Level
    CLEARBIT(IFS0bits.T1IF); // Clear Timer1 Interrupt Flag
    CLEARBIT(IEC0bits.T1IE); // Enable Timer1 interrupt ISR
    
    // Timer is started in main loop
}

// void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt()
// {
//     /* Copy new data into buffer and advance write ptr */
//     /* Check if the write pointer is past the end wrap around if necessary */
//     /* Clear interrupt flag */
// }

int r;
int check = 0;
int main(void)
{	
	/* Q: What is my purpose? */
	/* A: You pass butter. */
	/* Q: Oh. My. God. */

    /* LCD Initialization Sequence */
	__C30_UART=1;	
	lcd_initialize();
	lcd_clear();
    
    init_timer1();
    uart2_init(9600);
    
    uint8_t byte = 0;
    unsigned char header[4];
    unsigned char msg[MSG_BYTES_MSG];
    int hasHeader = 0;
    
    int i = 0; // index of next byte in array
    int N = 0; // size of msg
    int failed = 0; // number of times send failed
    
    uint16_t crc = 0;
    uint16_t recd_crc; // recieved crc
    
    char final_msg[MSG_BYTES_MSG] = "";
    
    while (1) {        
        if (U2STAbits.URXDA) {
            if (i == 0) {
                // reset timer
               SETBIT(T1CONbits.TON);
            }
            
            // read byte and check for error
            if (uart2_recv(&byte) < 0) {
              lcd_locate(0,0);
              lcd_printf_d("recv error");
              continue;
            }

            if (hasHeader) { // if has header, store byte in msg and update crc
                msg[i] = byte;
                crc = crc_update(crc, byte);
            } else { // otherwise store in header array         
              header[i] = byte;
            }

            // increment index
            i++;
        } 
        
        // Clear error bit if set
        if (U2STAbits.OERR) {
            U2STAbits.OERR = 0;
        }
        
        // When we have the full header
        if (!hasHeader && i == 4) {
            N = (int) header[3];
            hasHeader = 1;
            recd_crc = (header[1] << 8) | header[2];
//            lcd_locate(0,0);
//            lcd_printf_d("H: %x %x %x %x N:%d i:%d", header[0], header[1], header[2], header[3], N, i);
            
            i = 0; // reset index for recieving msg
            U2STAbits.OERR = 0;
        }
        
        // If Timer triggered -- used for recv timeout
        if (IFS0bits.T1IF) {
            N = i;
            CLEARBIT(IFS0bits.T1IF);
            lcd_locate(0,3);
            lcd_printf_d("Timer triggered");
        }
        
        // When we have the full msg
        if (hasHeader && i == N) {
//            crc = 0;            
            lcd_clear();
            lcd_locate(0,0);
            lcd_printf_d("Recv Failed: %d times", failed);
            lcd_locate(0,1);
            lcd_printf_d("CRC:%x", crc);
            lcd_locate(0,2);
            
            if (recd_crc != crc){
                failed++;
                uart2_send_8(MSG_NACK);
                
            } else {
                sprintf(final_msg, "%.*s", N, msg);
                failed = 0;
                
                lcd_printf_d("THISS:%s", final_msg);
                uart2_send_8(MSG_ACK);
            }

            N = 0;
            i = 0;
            hasHeader = 0;
            crc = 0;
            U2STAbits.OERR = 0;
            
            CLEARBIT(T1CONbits.TON); // Disable timer
            TMR1 = 0x00; // Reset timer
        }
    }
    
}	

