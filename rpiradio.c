#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <time.h>

#define DEBUG 0

// Use GPIO Pin 18, which is Pin 1 for wiringPi library
#define BUTTON1 7
#define BUTTON2 0
#define LEDPIN 4

// Debounce time in mS
#define	DEBOUNCE_TIME	500

// the event counter 
//volatile int eventCounter = 0;
int millisLastInterrupt = 0; 
//struct mpd_connection *conn;

/*static int handle_error(struct mpd_connection *c)
{
	assert(mpd_connection_get_error(c) != MPD_ERROR_SUCCESS);

	fprintf(stderr, "error handler: %s\n", mpd_connection_get_error_message(c));
	
	mpd_connection_free(c);
	return EXIT_FAILURE;
}*/


// -------------------------------------------------------------------------
// myInterrupt:  called every time an event occurs
void myInterrupt() {
	//char cmd[] = "mpc toggle";
	struct mpd_connection *conn;
	struct mpd_status * status;

	if(millis() > millisLastInterrupt+DEBOUNCE_TIME){
		//system(cmd);
		piLock(0);
		printf("1 has the lock\n");		
		
		//we connect
		conn = mpd_connection_new("127.0.0.1", 6600, 30000);
		if (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS){
			//connection ok
			printf("1 connected\n");
			
			// get the status and handle errors
			mpd_send_status(conn);
			status = mpd_recv_status(conn);			
			if (status != NULL){
				//if status is null -> we display the error
				if (mpd_status_get_error(status) == NULL){				
					if(mpd_run_toggle_pause(conn) == 1){
						printf("toggled pause\n");
					}	
					else{
						printf("error: pause toggle\n");
					}		
				}	
				else{
					printf("error: status 1: %s\n", mpd_status_get_error(status));
				}			
			}
			else{
				fprintf(stderr,"error: null status 1\n");
			}
		}
		else {
			//connection failed
		}
		//Interrrupt was valid -> we update millisLastInterrupt and give back the lock
		millisLastInterrupt = millis();
		fflush(stderr);
		fflush(stdout);
		printf("1 is giving back the lock\n");
		piUnlock(0);
	}
	else{
		//printf("warning: debouncing 1...\n");
	}	
}


void myInterrupt2(void) {
	//char cmd[] = "mpc next";
	struct mpd_connection *conn;
	struct mpd_status *status;

	if(millis() > millisLastInterrupt+DEBOUNCE_TIME){
		//system(cmd);
		piLock(0);
		printf("2 has the lock\n");

		//we connect
		conn = mpd_connection_new("127.0.0.1", 6600, 30000);
		if (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS){
			//connection ok			
			printf("2 connected\n");
			
			mpd_send_status(conn); //send the "status" command
			status = mpd_recv_status(conn); //receive the response and handle the error			
			if (status != NULL){
				if (mpd_status_get_error(status) == NULL){				
					unsigned l = mpd_status_get_queue_length(status);
					unsigned p = mpd_status_get_song_pos(status);				
					//we can do next
					if (p+1 < l){
						if(mpd_run_next(conn) == 1){
							printf("next\n");
						}	
						else{
							printf("error: next\n");
						}	
					}	
					//we need to go pack to the beginning of the playlist
					else{
						if(mpd_run_play_pos(conn, 0) == 1){
							printf("play 0\n");
						}	
						else {
							printf("error: play 0\n");
						}	
					}				
				}
				else{
					printf("error: status 2: %s\n", mpd_status_get_error(status));
					//printf("error: status: can not retrieve\n");
				}			
			}
			else{
				fprintf(stderr,"error: null status 2\n");
			}		
		}
		else {
			//connection failed
		}
		//Interrrupt was valid -> we update millisLastInterrupt and give back the lock
		millisLastInterrupt = millis();
		fflush(stderr);
		fflush(stdout);
		printf("2 is giving back the lock\n");
		piUnlock(0);
	}
	else{
		//printf("warning: debouncing 2...\n");
	}	
}

// -------------------------------------------------------------------------
// main
int main(void) {
	struct mpd_connection *conn;
	struct mpd_status *status;

	if(DEBUG){
		printf("DEBUG MODE!\n");
		if (freopen("/var/log/mpd/rpiradio.log", "w+", stdout) == NULL){
			perror("freopen()");
			fprintf(stderr,"freopen() failed in file %s at line # %d\n", __FILE__,__LINE__-3);
			exit(EXIT_FAILURE);
		}
		printf("stdout is redirected to this file\n");
		fflush(stdout);
		
		if (freopen("/var/log/mpd/rpiradio.log", "w+", stderr) == NULL){
			perror("freopen()");			
			exit(EXIT_FAILURE);
		}
	}

	//try until MPD is UP
	conn = mpd_connection_new("127.0.0.1", 6600, 30000);	
	while(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS){
		fprintf(stderr, "error: %s : waiting for 5s\n", mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		delay(5000);
		conn = mpd_connection_new("127.0.0.1", 6600, 30000);
	}		
	//from now on, MPD is supposed to answer
	printf("MPD UP?\n");	
	
	//unnecessary delay?
	delay(5000);

	/*
	//just in case, we reset the connection
	//because MPD is not yet fully available	
	mpd_connection_free(conn);
	printf("WAIT 5s and reset the connection\n");	
	delay(5000);
	conn = mpd_connection_new("127.0.0.1", 6600, 30000);
	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS){
		fprintf(stderr, "FUCK: %s \n", mpd_connection_get_error_message(conn));
		fflush(stderr);
		fflush(stdout);
		return 1;
	}
	fflush(stderr);
	fflush(stdout);
	*/
	
	//now we take care of wiringPi
	printf("Setting up wiringPi\n");	
	// sets up the wiringPi library
	if (wiringPiSetup () < 0) {
		fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
		return 1;
	}
	
	//Set up the correct pins
	pinMode (BUTTON1, INPUT) ;
	pinMode (BUTTON2, INPUT) ;
	pinMode (LEDPIN, OUTPUT) ;
	pullUpDnControl (BUTTON1, PUD_UP) ;
	pullUpDnControl (BUTTON2, PUD_UP) ;	

	// set Pin to generate an interrupt on high-to-low transitions
	// and attach myInterrupt() to the interrupt
	if ( wiringPiISR (BUTTON1, INT_EDGE_FALLING, &myInterrupt) < 0 ) {
		fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
		return 1;
	}
	if ( wiringPiISR (BUTTON2, INT_EDGE_FALLING, &myInterrupt2) < 0 ) {
		fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
		return 1;
	}
	
	status = mpd_run_status(conn);
	unsigned l = mpd_status_get_queue_length(status);
	printf("Queue: %d\n",l);

	fprintf(stderr, "DEBUG(err): Entering main loop \n");
	printf("DEBUG: Entering main loop \n");	
	// main loop
	while ( 1 ) {		
		if(DEBUG){
			status = mpd_run_status(conn);
			time_t clk = time(NULL);
			printf("%s", ctime(&clk));

			printf("Status: ");
			if (mpd_status_get_state(status) == MPD_STATE_PLAY)
			printf("playing");
			if (mpd_status_get_state(status) == MPD_STATE_PAUSE)
			printf("paused");
			if (mpd_status_get_state(status) == MPD_STATE_STOP)
			printf("stopped");
			if (mpd_status_get_state(status) == MPD_STATE_UNKNOWN)
			printf("UNKNOWN");
			printf("\n");

			fflush(stdout);
		}
		//fprintf(stderr, "DEBUG(err): Blink Da Bitch \n");
		//printf("DEBUG: Blink Da Bitch \n");
		fflush(stderr);
		fflush(stdout);
		//blink the led
		digitalWrite (LEDPIN,!digitalRead(LEDPIN));		
		delay( 2000 ); // wait 1 second
	}

	//we never reached this anyway...
	fclose(stdout);
	mpd_connection_free(conn);
	return 0;
}
