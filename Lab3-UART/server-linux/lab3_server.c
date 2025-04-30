/****************************************************/
/*                                                  */
/*   CS-454/654 Embedded Systems Development        */
/*   Instructor: Renato Mancuso <rmancuso@bu.edu>   */
/*   Boston University                              */
/*                                                  */
/*   Description: template file for serial          */
/*                communication server              */
/*                                                  */
/****************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "pc_crc16.h"
#include "lab3.h"

#define GREETING_STR						\
    "CS454/654 - Lab 3 Server\n"				\
    "Author: Renato Mancuso (BU)\n"				\
    "(Enter a message to send.  Type \"quit\" to exit)\n"

#define USAGE_STR							\
	"\nUSAGE: %s [-v] [-t percentage] <dev>\n"			\
	"   -v \t\t Verbose output\n"					\
	"   -t \t\t Invoke troll with specified bit flipping percentage\n" \
	"   <dev> \t Path to serial terminal device to use, e.g. /dev/ttyUSB0\n\n"

#define TROLL_PATH "./lab3_troll"

int main(int argc, char* argv[])
{
	double troll_pct=0.3;		// Perturbation % for the troll (if needed)
	int ifd, ofd, i, N, troll=0;	// Input and Output file descriptors (serial/troll)
	char str[MSG_BYTES_MSG],opt;	// String input
	struct termios oldtio, tio;	// Serial configuration parameters
	int VERBOSE = 0;		// Verbose output - can be overriden with -v
	int dev_name_len;
	char * dev_name = NULL;
	
	/* Parse command line options */
	while ((opt = getopt(argc, argv, "-t:v")) != -1) {
		switch (opt) {
		case 1:
			dev_name_len = strlen(optarg);
			dev_name = (char *)malloc(dev_name_len);
			strncpy(dev_name, optarg, dev_name_len);
			break;
		case 't':
			troll = 1; 
			troll_pct = atof(optarg);                    
			break;
		case 'v':
			VERBOSE = 1;
			break;
		default:
			break;
		}
	}

	/* Check if a device name has been passed */
	if (!dev_name) {
		fprintf(stderr, USAGE_STR, argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Open the serial port (/dev/ttyS1) read-write
	ifd = open(dev_name, O_RDWR | O_NOCTTY);
	if (ifd < 0) {
		perror(dev_name);
		exit(EXIT_FAILURE);
	}

	printf(GREETING_STR);

	// Start the troll if necessary
	if (troll)
	{
		// Open troll process (lab5_troll) for output only
		FILE * pfile;		// Process FILE for troll (used locally only)
		char cmd[128];		// Shell command

		snprintf(cmd, 128, TROLL_PATH " -p%f %s %s", troll_pct,
			 (VERBOSE) ? "-v" : "", dev_name);

		pfile = popen(cmd, "w");
		if (!pfile) { perror(TROLL_PATH); exit(-1); }
		ofd = fileno(pfile);
	}
	else ofd = ifd;		// Use the serial port for both input and output

	//
 	// WRITE ME: Set up the serial port parameters and data format
	//

	// *START BRIAN NEW CODE for Set up the serial port parameters and data format

	tcgetattr(ifd, &oldtio);

	/*
    Control modes for cflag: 
    B2400:	9600 Baud
    CS8:	8 data bits
    CLOCAL:	Ignore modem control lines
    CREAD:	Enable receiving
	*/
	tio.c_cflag 	= B9600 | CS8 | CLOCAL | CREAD; 
	tio.c_iflag 	= IGNPAR; // Input mode, ignore parity error
	tio.c_oflag 	= 0; // Output mode default
	tio.c_lflag 	= 0; // Default local modes
	tio.c_cc[VMIN]	= 1; // Control characters, VMIN is the minimum recv length
	
	tcflush(ifd, TCIFLUSH); // Flush any pending request on the port
	tcsetattr(ifd, TCSANOW, &tio); // Set new attributes for serial port


	// *END BRIAN NEW CODE for Set up the serial port parameters and data format

	while(1)
	{
		//
		// WRITE ME: Read a line of input (Hint: use fgetc(stdin) to read each character)
		//

		// *START BRIAN NEW CODE for Read a line of input (Hint: use fgetc(stdin) to read each character)

		unsigned char ack = MSG_NACK;
		int crc = 0xff, attempts=0;

		printf("> ");

		// Read msg from stdin
		// i is number of bytes of the msg
		for (i = 0; (i < MSG_BYTES_MSG); i ++) {
		  str[i] = fgetc(stdin); // Gets next character from stdin 

		  // Break when we read line break
		  if (str[i] == '\n') {
		    break;
		  }       
		}

		if (i >= MSG_BYTES_MSG - 1) {
		  printf("Exceeds 255 character limit");
		}

		str[i] = 0; // Removing \n
		N = i; // Setting message length

		// *END BRIAN NEW CODE for Read a line of input (Hint: use fgetc(stdin) to read each character)

		if (strcmp(str, "quit") == 0) break;

		//
		// WRITE ME: Compute crc (only lowest 16 bits are returned)
		//

		// *START BRIAN NEW CODE for Compute crc (only lowest 16 bits are returned)
		crc = pc_crc16(str, N);
		printf("crc: %x\n", crc); // print crc in hex
		// *END BRIAN NEW CODE for Compute crc (only lowest 16 bits are returned)
	
		while (!ack)
		{
			printf("Sending (attempt %d)...\n", ++attempts);
			
			// 
			// WRITE ME: Send message
			//
			// Starting byte
			dprintf(ofd, "%c", 0x0);

			// CRC
			dprintf(ofd, "%c", crc >> 8);
			dprintf(ofd, "%c", crc);

			// Message Length
			dprintf(ofd, "%c", N);

			dprintf(ofd, "%s", str);
			// 
			// END of our code
			//
			
			printf("Message sent, waiting for ack... ");

			//
			// WRITE ME: Wait for MSG_ACK or MSG_NACK
			//
			// reads response into ack variable
			int bytes_read = read(ifd, &ack, sizeof(ack));
      
			// Catch unsuccessful reads
			if (bytes_read < 0) {
			  perror("Error reading from serial port");
			}
			//
			// END of our code
			//

			printf("%s\n", ack ? "ACK" : "NACK, resending");
		}
		printf("\n");
	}


	//
	// WRITE ME: Reset the serial port parameters
	//

	// *START BRIAN NEW CODE Reset the serial port parameters
	tcflush(ifd, TCIFLUSH);
	tcsetattr(ifd, TCSANOW, &oldtio);
	// #END BRIAN NEW CODE Reset the serial port parameters
	
	// Close the serial port
	close(ifd);
	
	return EXIT_SUCCESS;
}

