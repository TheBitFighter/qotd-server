#include "util.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

int socketfd = -1;
struct addrinfo * ai = NULL;
char** quotes = NULL;
int quote_count = 0;
bool online = false;

static void read_request (int fd);
static char * random_quote (void);

int open_server_socket (char * port) {
	
	if (online) {
		// The connection is already open
		errno = EADDRINUSE;
		return -1;
	}

	socketfd = -1;
	errno = 0;

	// Get address info
	struct addrinfo hints;
	int err;

	memset (&hints, 0, sizeof (struct addrinfo));

	// If no port was provided, return an error
	if (port == NULL) {
		print_error ("No port was provided by caller\n");
		return -1;
	}

	// Set the needed information in hints
	hints.ai_flags = 1;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_addrlen = 0;
	hints.ai_addr = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;

	// Try to get the hostname
	err = getaddrinfo (NULL, port, &hints, &ai);

	// If an error was thrown by getaddrinfo, print the message
	if (err != 0) {
		print_error ("Could not get address info. Error was: %i %s\n", err, gai_strerror (err));
		return -1;
	}


	// If no address info could be resolved, throw an error
	if (ai == NULL) {
		print_error ("Address info could not be obtained\n");
		return -1;
	}


	// Generate a socket
	if ((socketfd = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0) {
		print_error ("Socket could not be opened. Error: %i %s\n", errno, strerror (errno));
		return -1;
	}

	// Bind the address
	if (bind (socketfd, ai->ai_addr, ai->ai_addrlen) < 0) {
		print_error ("Socket could not be bound. Error: %i %s\n", errno, strerror (errno));
		return -1;
	}

	// Listen to the generated socket
	if (listen (socketfd, 1) < 0) {
		print_error ("Could not listen to socket. Error: %i %s\n", errno, strerror (errno));
		return -1;
	}

	online = true;

	return 0;
}


/**
 * @brief Accept a client connection and send a quote as a response
 *
 */
char* accept_client_connection (void) {

	if (socketfd < 0 || ai == NULL) {
		print_error ("Socket has not been openend\n");
		return NULL;
	}

	// Accept incoming connection
	int cfd = accept (socketfd, ai->ai_addr, &(ai->ai_addrlen));

	if (cfd < 0) {
		if (errno == 4) {
			print_error ("Connection was interrupted by a signal. Terminating connections.\n");
			return NULL;
		}

		print_error ("Connection could not be established. Error: %i %s\n", errno, strerror (errno));
		return NULL;
	}

	// Receive header
	read_request (cfd);

	// Build response
	char* response = random_quote ();
	int response_size = sizeof (char) * strlen (response);

	// Send the data
	if (write(cfd, response, response_size) < 0) {
		close (cfd);
		return NULL;
	}

	// Close the connection
	close (cfd);
	return response;
}


/**
 * @brief Close the server socket if it has been opened
 * @details If the server socket was opened before, close it and open it for reuse
 *
 * @returns 0 on success.
 */
int close_server_socket (void) {
	
	// Check if we are online
	if (!online) {
		return 0;
	}

	// Free the address info
	int optval = 1;
	free (ai);
	ai = NULL;

	// Set socket to be reused
	setsockopt (socketfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));
	if (close (socketfd) < 0) {
		return -1;
	}
	return 0;
}


/**
 * @brief Read the incoming transmission
 * @details The incoming transmission gets read from the input buffer and discarded.
 *
 * @param fd The socket file descriptor to read from
 */
static void read_request (int fd) {
	int read_length = sizeof (char);
	int read_bytes = -1;
	while ((read_bytes = read (fd, NULL, read_length)) > 0) {
	}
}


/**
 * @brief Get a random quote
 * @details Select a random quote from the loaded quotes file and return it
 * as a char pointer.
 * If no quote file is loaded NULL is returned and ERRNO is set appropriately.
 *
 * @returns A random quote from the quote file.
 */
char* random_quote (void) {
	if (quotes == NULL) {
		return NULL;
	}

	int random = rand () % quote_count;
	return quotes[random];
}


/**
 * @brief Load the file to read quotes from
 * @details Opens the file to read quotes from. If the file could not
 * be loaded, an error is returned and ERRNO is set appropriately.
 *
 * @param file_path The path to the file to read quotes from
 * @returns 0 on success.
 */
int load_file (char* file_path) {
	// Try to open the quotes file
	FILE* quotes_file = fopen (file_path, "r");

	// If it could not be opened, return an error
	if (quotes_file == NULL) {
		return -1;
	}
	print_error ("File %s opened\n", file_path);

	int size = 10;

	// Create the array for the quotes to live in
	quotes = malloc (size * sizeof (char*));

	// Check if there is enough memory left
	if (quotes == NULL) {
		fclose (quotes_file);
		return -1;
	}

	char* line = NULL;
	errno = 0;
	size_t read_size = 0;

	// Read the quotes into ram; ommit empty lines
	while (getline (&line, &read_size, quotes_file) > -1) {
		
		// If the line is not empty save it as a quote
		if (strlen (line) > 1) {
			quote_count++;
			
			// Check if enough space is left to save it
			if (quote_count > size) {
				size *= 2;
				quotes = realloc (quotes, size * sizeof (char*));
				
				// Check if enough memory was left
				if (quotes == NULL) {
					fclose (quotes_file);
					return -1;
				}
			}

			// If there is a newline at the end delete it
			if (line [strlen (line) - 1] == '\n') {
				line [strlen (line) - 1] = 0;
			}

			quotes[quote_count - 1] = line;
			line = NULL;
		}
	}

	// Check if errno is set; if it is an error has occured
	if (errno != 0) {
		fclose (quotes_file);
		return -1;
	}

	// Check if there was at least one line in the file
	if (quote_count < 1) {
		fclose (quotes_file);
		errno = ENOENT;
		return -1;
	}

	print_error ("Success! %i quotes were loaded!\n", quote_count);

	// Try to close the quotes file
	if (fclose (quotes_file) == EOF) {
		return -1;
	}

	return 0;
}


/**
 * @brief Unload quotes from ram
 * @details Frees the memory where the quotes had been loaded
 */
void unload_quotes (void) {
	for (int i = 0; i < quote_count; i++) {
		free (quotes[i]);
		quotes[i] = NULL;
	}
}
