/**
 * @file main.c
 * @author TheBitFighter <flip97.fp@gmail.com>
 * @date 07.03.2019
 *
 * @brief The main program module
 *
 * This is a simple implementation of the RFC 865 Quote of the day protocol.
 * If a packet is received on port 17 (if not otherwise specified) a random quote is returned.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h> 
#include <assert.h>
#include "util.h"
#include "ssocket.h"
#include <signal.h>

bool index_set = false;
char * port = "17";
bool port_set = false;
volatile sig_atomic_t quit = 0;

static int read_flags (int argc, char ** argv);
static void clean_up (void);
void handle_signal (int signal);

int main (int argc, char ** argv) {

	// Set the program name 
	prg_name = argv[0];

	// Set the signal handler
	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));

	sa.sa_handler = handle_signal;
	sigaction (SIGINT, &sa, NULL);
	sigaction (SIGTERM, &sa, NULL);
	
	// Parse the arguments
	if (read_flags (argc, argv) < 0) {
		print_usage ();
		clean_up();
		exit (EXIT_FAILURE);
	}

	if (open_server_socket (port) < 0) {
		print_usage ();
		clean_up ();
		exit (EXIT_FAILURE);
	}

	printf ("Opened socket on port %s\n", port);

	while (!quit) {
		int response_code;
	
		response_code = accept_client_connection ();

		printf ("Answered client request with response code %d\n", response_code);
	}
	
	if (close_server_socket () < 0) {
		print_usage ();
		clean_up ();
		exit (EXIT_FAILURE);
	}


	clean_up();
	exit (EXIT_SUCCESS);
}

static int read_flags (int argc, char ** argv) {
	int opt = 0;

	while ((opt = getopt (argc, argv, "p:")) != -1) {
		switch (opt) {
			case 'p' :
				// Port was set

				// Check if port was set more than once
				if (port_set) {
					print_error ("Port was set more than once\n");
					return -1;
				}

				// Check if port can be parsed to integer
				char * endptr;
				int check_port = strtol (optarg, &endptr, 10);

				if (errno != 0 || endptr == optarg) {
					print_error ("Port number could not be parsed\n");
					return -1;
				}

				// Check if portnumber is positive
				if (check_port < 1) {
					print_error ("Port number must be positive\n");
					return -1;
				}

				// Set the port parameter
				port = malloc (sizeof (char) * (strlen (optarg) + 1));
				port = strcpy (port, optarg);

				// Set the port_set flag 
				port_set = true;

				break;
			case '?' :
				// Invalid option was set
				print_error ("Invalid option\n");
				return -1;
			default : 
				// Unreachable code
				assert (0);
				return -1;
		}
	}

	// Read and check the document root

	// Check if the number of arguments is valid
	// In this case it is only valid if the number of args is even
	if (argc < 2 || argc % 2 != 0) {
		print_error ("Invalid usage\n");
		return -1;
	}
	doc_root = malloc (sizeof (char) * strlen (argv[argc-1]));
	doc_root = strcpy (doc_root, argv[argc-1]);

	DIR* dir = opendir (doc_root);

	// Check if given directory exists
	if (doc_root) {
		// Directory exists
		closedir (dir);
	} else if (ENOENT == errno) {
		// Directory does not exist
		print_error ("The given quote file does not exist\n");
		return -1;
	} else  {
		// Directory exists but could not be opened
		print_error ("The given quote file could not be opened\n");
		return -1;
	}

	// Everything is ok
	return 0;

}

static void clean_up (void) {
	// Free everything and set the pointers to NULL
	if (port_set) free (port);
	port = NULL;
	
}

void handle_signal (int signal) {
	print_error ("Received signal %d \n", signal);
	print_error ("Closing connections...\n");
	quit = 1;
}
