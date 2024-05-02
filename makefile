# makefile for gulam

# 890111 kbad	hacked it out. Note: some of these files may benefit from
#		MWC optimization flagz, but I haven't bothered.
#
# Use these defines for MWC
#CFLAGS	=	-DMWC -VPEEP -NOVSUVAR -NOVSNREG
#LIBS = -lc
#CC = cc
#
#.c.o:
#	cc $(CFLAGS) -c $*.c
#
# Use these defines for GCC
CROSS = m68k-atari-mint-
WARN = -Wall -Wstrict-prototypes -Wmissing-prototypes -Wold-style-declaration -Wold-style-definition -Werror
CC = $(CROSS)gcc
CFLAGS = -O2 -fomit-frame-pointer -funsigned-char $(WARN) -DTOS
LDFLAGS = -s
LIBS =

# gasmmwc.o removed, gasmgnu.o added

UEFILES	=	basic.o buffer.o display.o error.o file.o \
		fio.o function.o gasmgnu.o kb.o line.o misc.o pmalloc.o random.o \
		regexp.o region.o rsearch.o teb.o ue.o window.o word.o

GUFILES	=	cs.o do.o ex.o fop.o gcoatari.o \
		gfsatari.o gioatari.o gmcatari.o hist.o \
		lex.o ls.o main.o pregrep.o rehash.o tv.o \
		util.o xmdm.o

OFILES	= $(UEFILES) $(GUFILES)

all: gulam.ttp

gulam.ttp:	$(OFILES)
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OFILES) $(LIBS)

$(OFILES):	ue.h gu.h keynames.h regexp.h sysdpend.h

gasmmwc.o:	gasmmwc.s
		as -o $@ $<

clean::
	rm -f *.o *.ttp
