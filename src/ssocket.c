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
char* index_name = "index.html";
struct addrinfo * ai = NULL;
char * doc_root;

static int check_header (char * header, int header_length, char ** file, char ** response);
static int read_request (int fd, char ** message);
static int look_for_terminator (char * message, int message_size);
static int attach_payload (char ** message, FILE* payload, int payload_size);
static bool get_file_type (char * file, char ** file_type);
static void compile_file_path (char ** file_path, char * file);
static int create_response (char ** response, int * response_code, char * response_message, char* payload);
static void get_date (char ** date);

int open_server_socket (char * port) {
	socketfd = -1;
	errno = 0;

	/*	
		Get address info
	 */

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
		print_error ("Could not get address info. Error was: %s\n", gai_strerror (err));
		return -1;
	}


	// If no address info could be resolved, throw an error
	if (ai == NULL) {
		print_error ("Address info could not be obtained\n");
		return -1;
	}


	/*
	   Generate a socket
	 */

	if ((socketfd = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0) {
		print_error ("Socket could not be opened. Error: %s\n", strerror (errno));
		return -1;
	}

	/*
	   Bind the address
	 */

	if (bind (socketfd, ai->ai_addr, ai->ai_addrlen) < 0) {
		print_error ("Socket could not be bound. Error: %s\n", strerror (errno));
		return -1;
	}

	/*
	   Listen to the generated socket
	 */

	if (listen (socketfd, 1) < 0) {
		print_error ("Could not listen to socket. Error: %s\n", strerror (errno));
		return -1;
	}

	return 0;
}

int accept_client_connection (void) {

	if (socketfd < 0 || ai == NULL) {
		print_error ("Socket has not been openend\n");
	}

	/*
	   Accept incoming connection
	 */

	int cfd = accept (socketfd, ai->ai_addr, &(ai->ai_addrlen));

	if (cfd < 0) {
		print_error ("Connection could not be established. Error: %s\n", strerror (errno));
		return -1;
	}

	/*
	   Receive header
	 */
	
	read_request (cfd, &message);
	int response_code = 200;

	/* 
	   Build response
	 */

	char* response;
	int response_size = create_response (&response, &response_code, response_message, filepath);

	/*
	   Send the data
	 */

	if (write(cfd, response, response_size) < 0) {
		free (file);
		free (response);
		free (message);
		close (cfd);
		return -1;
	}

	/*
	   Close the connection
	 */

	free (file);
	free (response);
	free (message);
	close (cfd);
	return response_code;
}

int close_server_socket (void) {
	int optval = 1;
	free (ai);
	free (doc_root);
	setsockopt (socketfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));
	if (close (socketfd) < 0) {
		print_error ("Socket could not be closed. Error: %s\n", strerror (errno));
		return -1;
	}
	return 0;
}


static void read_request (int fd) {
	int read_length = sizeof (char);
	int read_bytes = -1;
	while (read_bytes = read (fd, current, read_length) > 0) {
	}
}


static int create_response (char ** response, int * response_code, char * response_message, char* payload) {
	int file_size = 0;
	bool read_file = false;
	FILE * payload_file = NULL;
	if (payload != NULL) {
		struct stat st;
		if (stat (payload, &st) != 0) {
			print_error ("Could not get payload stats for file %s. Error: %d %s\n", payload, errno, strerror (errno));
			*response_code = 404;
			response_message = "Not Found";
		} else {
			file_size = st.st_size;
			payload_file = fopen (payload, "r");
		}
	}

	/*
	 * Load the payload
	 */


	if (payload != NULL && payload_file == NULL) {
		print_error ("The file %s could not be opened. Error: %d %s\n", payload, errno, strerror (errno));
		*response_code = 404;
		response_message = "Not Found";
	} else {
		read_file = true;
	}

	/*
	 * Get the date
	 */

	char* date;
	get_date (&date);

	// Convert the integers to strings

	char * response_code_string = malloc (24);
	sprintf (response_code_string, "%d", *response_code);

	char * content_length_string = malloc (24);
	sprintf (content_length_string, "%d", file_size);


	/*
	 * Build the response
	 */

	int response_size = strlen (response_message) + 150 + file_size + 24;
	*response = calloc (response_size, sizeof (char));

	// HTTP/1.1 xxx Response message
	*response = strcpy (*response, "HTTP/1.1 ");
	*response = strcat (*response, response_code_string);
	*response = strcat (*response, " ");
	*response = strcat (*response, response_message);
	*response = strcat (*response, "\r\n");

	if (*response_code == 200) {
		// Date: Sun, 11 Nov 2018 22:55:00 GMT
		*response = strcat (*response, "Date: ");
		*response = strcat (*response, date);
		*response = strcat (*response, "\r\n");

		// Content-Length: xxx
		*response = strcat (*response, "Content-Length: ");
		*response = strcat (*response, content_length_string);
		*response = strcat (*response, "\r\n");
	}

	// Connection: close
	*response = strcat (*response, "Connection: close");
	*response = strcat (*response, "\r\n");

	free (date);

	// Content-Type
	char * file_type;
	if (payload != NULL && read_file) {
		bool known = get_file_type (payload, &file_type);
		if (known) {
			*response = strcat (*response, "Content-Type: ");
			*response = strcat (*response, file_type);
			*response = strcat (*response, "\r\n");
		}
	}

	// Close the header
	*response = strcat (*response, "\r\n");

	if (read_file) {
		attach_payload (response, payload_file, file_size);
	}

	// If there was a payload file, close it
	if (payload_file != NULL) fclose (payload_file);

	return response_size;

}

static int attach_payload (char ** message, FILE* payload, int payload_size) {
	int read_bytes = 0;

	int pos = strlen (*message);

	char * current = malloc (sizeof (char) * (payload_size +1));

	// As long as there is something to read, read it
	while (payload_size > 0 && (read_bytes = fread (current, sizeof (char), payload_size, payload)) > 0) {
		payload_size -= read_bytes;
		int i = 0;
		while (i < read_bytes) {
			(*message)[pos] = current[i];
			pos++;
			i++;
		}
	}

	// In case of a read error, return -1
	if (read_bytes < 1) return -1;
	return 0;
}

static void get_date (char ** date) {
	time_t t = time (NULL);
	struct tm tm = * gmtime (&t);
	
	*date = malloc (sizeof (char) * 50);
	*date = strcpy(*date, "");
	switch (tm.tm_wday) {
		case 0:
			*date = strcat (*date, "Mon, ");
			break;
		case 1:
			*date = strcat (*date, "Tue, ");
			break;
		case 2: 
			*date = strcat (*date, "Wed, ");
			break;
		case 3:
			*date = strcat (*date, "Thu, ");
			break;
		case 4: 
			*date = strcat (*date, "Fri, ");
			break;
		case 5: 
			*date = strcat (*date, "Sat, ");
			break;
		case 6 :
			*date = strcat (*date, "Sun, ");
			break;
	}

	char * number_text = malloc (sizeof (char) * 12);
	sprintf (number_text, "%d", tm.tm_mday);

	*date = strcat (*date, number_text);

	switch (tm.tm_mon) {
		case 0 :
			*date = strcat (*date, " Jan ");
			break;
		case 1: 
			*date = strcat (*date, " Feb ");
			break;
		case 2: 
			*date = strcat (*date, " Mar ");
			break;
		case 3: 
			*date = strcat (*date, " Apr ");
			break;
		case 4: 
			*date = strcat (*date, " May ");
			break;
		case 5: 
			*date = strcat (*date, " Jun ");
			break;
		case 6: 
			*date = strcat (*date, " Jul ");
			break;
		case 7: 
			*date = strcat (*date, " Aug ");
			break;
		case 8: 
			*date = strcat (*date, " Sep ");
			break;
		case 9: 
			*date = strcat (*date, " Oct ");
			break;
		case 10: 
			*date = strcat (*date, " Nov ");
			break;
		case 11: 
			*date = strcat (*date, " Dec ");
			break;
	}

	// Add the year
	sprintf (number_text, "%d", (tm.tm_year + 1900));
	*date = strcat (*date, number_text);
	*date = strcat (*date, " ");
	
	// Add the hour
	sprintf (number_text, "%d", tm.tm_hour);
	*date = strcat (*date, number_text);
	*date = strcat (*date, ":");
	
	// Add the minute
	sprintf (number_text, "%d", tm.tm_min);
	*date = strcat (*date, number_text);
	*date = strcat (*date, ":");

	// Add the second
	sprintf (number_text, "%d", tm.tm_sec);
	*date = strcat (*date, number_text);
	*date = strcat (*date, " GMT");

}

static bool get_file_type (char * file, char ** file_type) {

	*file_type = "";

	int length = strlen (file);

	if (strlen (file) < 3) return false;
	char* two_letter = &(file[length-3]);

	// Found .js
	if (strcmp (two_letter, ".js") == 0) {
		*file_type = "application/javascript";
		return true;
	}

	if (strlen (file) < 4) return false;
	char * three_letter = &(file[length-4]);

	// Found .htm
	if (strcmp (three_letter, ".htm") == 0) {
		*file_type = "text/html";
		return true;
	}

	// Found .css
	if (strcmp (three_letter, ".css") == 0) {
		*file_type = "text/css";
		return true;
	}

	// Found .png
	if (strcmp (three_letter, ".png") == 0) {
		*file_type = "image/png";
		return true;
	}

	// Found .pdf
	if (strcmp (three_letter, ".pdf") == 0) {
		*file_type = "application/pdf";
		return true;
	}

	if (strlen (file) < 5) return false;
	char* four_letter = &(file[length-5]);

	// Found .html
	if (strcmp (four_letter, ".html") == 0) {
		*file_type = "text/html";
		return true;
	}
	return false;
}
