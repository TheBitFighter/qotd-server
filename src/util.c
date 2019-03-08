#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

char* prg_name;

void print_error (char * message, ...) {
	char * to_print = NULL;
	to_print = malloc (sizeof (char) * (strlen (message) + strlen (prg_name) + 3));

	to_print = strcpy (to_print, "[");
	to_print = strcat (to_print, prg_name);
	to_print = strcat (to_print, "] ");
	to_print = strcat (to_print, message);

	va_list arguments;
	va_start (arguments, message);

	vfprintf (stderr, to_print, arguments);

	free (to_print);
	va_end (arguments);
	to_print = NULL;
}

void print_usage (void) {
	fprintf (stderr, "[%s] Usage: %s [-p PORT] QUOTE_FILE\n", prg_name, prg_name);
}
