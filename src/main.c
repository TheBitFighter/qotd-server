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

/**
 * @brief Main method
 * @details Main method of the program. Opens a server socket and keeps it open 
 * until a signal is received.
 *
 * @param argc The argument counter
 * @param argv The argument vector
 * @return Returns EXIT_SUCCESS
 */
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

	// Open the server
	if (open_server_socket (port) < 0) {
		print_usage ();
		clean_up ();
		exit (EXIT_FAILURE);
	}

	print_error ("Opened socket on port %s\n", port);

	// While the server has not been aborted, answer requests
	while (!quit) {
	
		char* response = accept_client_connection ();
		
		if (response != NULL) {
			print_error ("Answered client request with quote %s\n", response);
		}
	}
	
	// Clean up the environment and exit
	clean_up();
	exit (EXIT_SUCCESS);
}


/**
 * @brief Parse the arguments
 * @details Take the argument vector and parse the indiviual arguments from it.
 * It also looks up the supplied quotes file. If an invalid argument is given or 
 * the quotes file does not exist, an error is returned.
 *
 * @param argc The argument counter
 * @param argv The argument vector
 * @returns 0 on success
 */
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
				port = malloc (sizeof (char) * strlen (optarg));
				sprintf(port, "%i", check_port);

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

	// Check if the number of arguments is valid
	// In this case it is only valid if the number of args is even
	if (argc < 2 || argc % 2 != 0) {
		print_error ("Invalid usage\n");
		return -1;
	}

	// Load the quotes file into the server
	if (load_file (argv[argc-1]) < 0) {
		print_error ("Quotes file could not be loaded. Error: %i %s\n", errno, strerror (errno));
		return -1;
	}

	// Everything is ok
	return 0;

}


/**
 * @brief Clean up the used memory
 */
static void clean_up (void) {
	// Close the server socket
	if (close_server_socket () < 0) {
		print_error ("Server socket could not be closed. Error: %i %s\n", errno, strerror (errno));
	}

	// Unload the quotes from ram
	unload_quotes ();

	if (port != NULL && port_set) {
		free (port);
		port = NULL;
	}
}

/**
 * @brief Handle an incoming signal
 * @details If SIGINT or SIGTERM is received, the quit variable is set to 1
 * which terminates the program.
 */
void handle_signal (int signal) {
	print_error ("Received signal %d \n", signal);
	print_error ("Closing connections...\n");
	quit = 1;
}
