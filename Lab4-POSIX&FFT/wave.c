/****************************************************/
/*                                                  */
/*   CS-454/654 Embedded Systems Development        */
/*   Instructor: Renato Mancuso <rmancuso@bu.edu>   */
/*   Boston University                              */
/*                                                  */
/*   Description: template file for digital and     */
/*                analog square wave generation     */
/*                via the LabJack U3-LV USB DAQ     */
/*                                                  */
/****************************************************/

#include "u3.h"
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>

HANDLE hDevice;
u3CalibrationInfo caliInfo;
long digitalSignal = 0;
int analogState = 0;
double vHigh = 5, vLow = 0;

/* This function should initialize the DAQ and return a device
 * handle. The function takes as a parameter the calibratrion info to
 * be filled up with what obtained from the device. */
HANDLE init_DAQ()
{
	HANDLE hDevice;
	int localID = 1;
	
	/* Invoke openUSBConnection function here */
  hDevice = openUSBConnection(localID);
	
	/* Invoke getCalibrationInfo function here */
  if (getCalibrationInfo(hDevice, &caliInfo) != 0) {
    perror("getCalibrationInfo error");
    exit(1);
  }
	
	return hDevice;
}

timer_t init_timer(int SIG, float freq) 
{
  struct sigevent timer_event;
  timer_t timer; 
  
  // Zero out the structure and configure for use of the SIGRTMIN signal.
  memset(&timer_event, 0, sizeof(timer_event));
  timer_event.sigev_notify = SIGEV_SIGNAL; // There might be e
  timer_event.sigev_signo = SIG;

  struct itimerspec timer_time;
  freq = 0.5 / freq;
  time_t sec = (time_t) freq;
  long nsec = (long)((freq - sec) * 1e9);

  timer_time.it_value.tv_sec = sec;
  timer_time.it_value.tv_nsec = nsec;
  timer_time.it_interval.tv_sec = sec;
  timer_time.it_interval.tv_nsec = nsec;

  if (timer_create(CLOCK_REALTIME, &timer_event, &timer) != 0)
  {
    // If there is an error, print out a message and exit.
    perror("timer_create error");
    exit(1);
  }

  // Schedule the timer.
  if (timer_settime(timer, 0, &timer_time, NULL) != 0)
  {
    // If there is an error, print out a message and exit.
    perror("timer_settime");
    exit(1);
  }

  return timer; 
}

void rtmin_handler(int signum)
{
  // printf("SIGRTMIN Recieved.\n");
  digitalSignal = !digitalSignal;
  if (signum == SIGRTMIN){
    eDO(hDevice, 1, 2, digitalSignal); // digital output, FIO2, High (in order of parameters)
  }
}

void rtmax_handler(int signum)
{
  // printf("SIGRTMAX Recieved.\n");
  int voltage;
  if (signum == SIGRTMAX){
    if (analogState) {
      voltage = vHigh;
      analogState = 0;
    } else {
      voltage = vLow;
      analogState = 1;
    }

    eDAC(hDevice, &caliInfo, 1, 0, voltage, 0, 0, 0); // Channel 0
  }
}

int main(int argc, char **argv)
{
	/* Invoke init_DAQ and handle errors if needed */
  hDevice = init_DAQ();

	/* Provide prompt to the user for a voltage range between 0
	 * and 5 V. Require a new set of inputs if an invalid range
	 * has been entered. */
    printf("Enter the first (max) voltage (0-5 V): ");
    if (scanf("%le", &vHigh) != 1) {
        fprintf(stderr, "Error: Invalid input.\n");
        return EXIT_FAILURE;
    }

    // Check the range of the first voltage
    if (vHigh < 0.0f || vHigh > 5.0f) {
        fprintf(stderr, "Error: First voltage out of range.\n");
        return EXIT_FAILURE;
    }

    printf("Enter the second (min) voltage (0-5 V): ");
    if (scanf("%le", &vLow) != 1) {
        fprintf(stderr, "Error: Invalid input.\n");
        return EXIT_FAILURE;
    }
    // Check the range of the second voltage
    if (vLow < 0.0f || vLow > 5.0f) {
        fprintf(stderr, "Error: Second voltage out of range.\n");
        return EXIT_FAILURE;
    }

	/* Compute the maximum resolutiuon of the CLOCK_REALTIME
	 * system clock and output the theoretical maximum frequency
	 * for a square wave */
  struct timespec ts;
  clock_getres(CLOCK_REALTIME, &ts);
  printf("resolution: %f\n", 1.0 / (ts.tv_sec + ts.tv_nsec / 1e9));

	/* Provide prompt to the user to input a desired square wave
	 * frequency in Hz. */
    float freq;
    printf("What fakcing frequency (Hz): ");
    if (scanf("%f", &freq) != 1 || freq < 0) {
        fprintf(stderr, "Error: Invalid input.\n");
        return EXIT_FAILURE;
    }

    int type;
    printf("Would you like to do digital or analog: (1 for digital, 0 for analog): ");
    if (scanf("%d", &type) != 1 || !(type == 1 || type == 0)) {
        fprintf(stderr, "Error: Invalid input.\n");
        return EXIT_FAILURE;
    }
	
	/* Program a timer to deliver a SIGRTMIN signal, and the
	 * corresponding signal handler to output a square wave on
	 * BOTH digital output pin FIO2 and analog pin DAC0. */

  
  // Sigaction Init   
  if (type == 1) { // digital
    // Create Timer
    timer_t timer1 = init_timer(SIGRTMIN, freq);

    struct sigaction sa1;
    sa1.sa_flags = 0;
    sa1.sa_handler = rtmin_handler;
    sigemptyset(&sa1.sa_mask);

    if (sigaction(SIGRTMIN, &sa1, NULL) == -1) {
        perror("sigaction sa1");
        exit(1);
    }
  } else if (type == 0){ // analog
    timer_t timer2 = init_timer(SIGRTMAX, freq);

    struct sigaction sa2;
    sa2.sa_flags = 0;
    sa2.sa_handler = rtmax_handler;
    sigemptyset(&sa2.sa_mask);

    if (sigaction(SIGRTMAX, &sa2, NULL) == -1) {
        perror("sigaction sa2");
        exit(1);
    }
  }
 	
  
	/* The square wave generated on the DAC0 analog pin should
	 * have the voltage range specified by the user in the step
	 * above. */

    /* Display a prompt to the user such that if the "exit"
    * command is typed, the USB DAQ is released and the program
    * is terminated. */
    
  printf("You entered: %.2f V and %.2f V and %.2f\n", vHigh, vLow, freq);

  printf("Enter Command: ");
  
  while (1){
    char command[256];     
    if (scanf("%255s", command) == 1){
      if (strcmp(command, "exit") == 0){
        break;
      }
    }
  }

  closeUSBConnection(hDevice);
	return EXIT_SUCCESS;
}

