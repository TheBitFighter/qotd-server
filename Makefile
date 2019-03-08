# Macros
# ------
CC      = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c99 -pedantic -Wall -g -O0

LD      = gcc
LDFLAGS = -lm

CFILES  = src/main.c src/util.c src/ssocket.c
OFILES  = $(CFILES:.c=.o)

DEPENDFILE = .depend

# Rules
# -----
%.o: %.c
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@

# Targets
# -------
all: qotd-server

clean:
	rm -f $(OFILES)

dep: depend

depend: 
	$(CC) $(CFLAGS) -MM $(CFILES) > $(DEPENDFILE)

qotd-server: $(OFILES)
	$(LD) -o $@ $(OFILES) $(LDFLAGS)

-include $(DEPENDFILE)
