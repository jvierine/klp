# MAKEFILE
#
# Makefile for KLP (KAIRA Local Pipeline)
#

VERSION=1.4
CFLAGS=-O3
PREFIX=/usr
SRC=klp_core.c klp_core.h filewriter.c averagepower.c nullsink.c README Makefile

all:
	gcc $(CFLAGS) -o klp_filewriter -lm -ffast-math -pthread klp_core.c filewriter.c
	gcc $(CFLAGS) -o klp_nullsink -lm -ffast-math -pthread klp_core.c nullsink.c
	gcc $(CFLAGS) -o klp_avgpwr -lm -ffast-math -pthread klp_core.c averagepower.c

install: all
	cp klp_filewriter klp_nullsink klp_avgpwr $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/klp_*

dist:
	mkdir -p klp-$(VERSION)
	cp $(SRC) klp-$(VERSION)
	tar cvfz klp-$(VERSION).tar.gz klp-$(VERSION)

clean:
	rm -Rf data-*
	rm -Rf klp-$(VERSION)
	rm -f *.tar.gz
	rm -f *.o
	rm -f \#*
	rm -f *~
	rm -f klp_filewriter klp_nullsink klp_avgpwr

# EOF
