# Asuka Makefile for use with GNU Make 
# and Mingw GNU gcc/g++


O = o
A = a
C = c
CPP = cpp
CC = g++
LD = g++
GC = gcc
CFLAGS = -O2 -I.
CPPFLAGS =
PGNAME = Asuka.exe

#
# SRC Directory
#

SRCDIR=

#
# Target
#

SRCS =${SRCDIR}main.${CPP} ${SRCDIR}parser.${CPP} ${SRCDIR}module.${CPP} \
	${SRCDIR}generator.${CPP} ${SRCDIR}filepath.${CPP} ${SRCDIR}scanner.${CPP} \
	${SRCDIR}stringx.${CPP} ${SRCDIR}table.${CPP} ${SRCDIR}textmodule.${CPP}

OBJS = ${SRCDIR}main.${O} ${SRCDIR}parser.${O} ${SRCDIR}module.${O} \
	${SRCDIR}generator.${O} ${SRCDIR}filepath.${O} ${SRCDIR}scanner.${O} \
	${SRCDIR}stringx.${O} ${SRCDIR}table.${O} ${SRCDIR}textmodule.${O}

%.${O} : ${SRCDIR}%.${CPP}
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.${CPP}

all: ${PGNAME}

${PGNAME}: ${OBJS}
	${LD} -o ${PGNAME} ${OBJS} ${LDFLAGS}
	strip ${PGNAME}

distclean: clean

clean:
	rm -f *.o
	rm -f ${PGNAME}

# Are You Ready to Be A Member of OSASK Project Now?
