#
# Makefile for Console Telnet
# Last modified 4/15/2000 by Paul Brannan
#

SRCDIR=./src
OBJDIR=src
RESDIR=resource

SRC=$(wildcard $(SRCDIR)/*.cpp)
RESOURCES=$(wildcard $(RESDIR)/*.rc)
OBJ1=$(SRC:.c=.o)
OBJ=$(OBJ1:.cpp=.o) $(RESOURCES:.rc=.o)

INCLUDES=-I$(RESDIR)

OUT=telnet.exe

# Modify these for your system if necessary
# Note: DJGPP+RDXNTDJ configuration is untested.

# --CYGWIN--
#CC=gcc
#CCC=g++
#LDFLAGS=-lwsock32 -lmsvcrt
#CFLAGS=-O2 -Wall -mwindows -mno-cygwin -D__CYGWIN__
#CCFLAGS=$(CFLAGS)
#RES=
#RC=windres
#RCFLAGS=-O coff

# --MINGW32(+EGCS)--
CC=gcc
CCC=g++
LDFLAGS=-lkernel32 -luser32 -lgdi32 -lshell32 -lwsock32
CFLAGS=-O2 -Wall
CCFLAGS=$(CFLAGS)
RES=
RC=windres
RCFLAGS=

# --DJGPP+RSXNTDJ--
#CC=gcc -Zwin32 -Zmt -Zcrtdll
#CCC=$(CC)
#LDFLAGS=
#CFLAGS= -g
#CCFLAGS=$(CFLAGS)
#RES=rsrc
#RC=grc
#RCFLAGS=-r


# You should not have to modify anything below this line

all: dep $(OUT)

.SUFFIXES: .c .cpp .rc

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

.rc.o:
	$(RC) -i $< $(RCFLAGS) -o $@

$(OUT): $(OBJ)
	$(CCC) $(OBJ) $(LDFLAGS) $(LIBS) -o $(OUT)
	strip $(OUT)

depend: dep

dep:
	start /min makedepend -- $(CFLAGS) -- $(INCLUDES) $(SRC)

clean:
	del $(OBJDIR)\*.o
	del $(OUT)

# DO NOT DELETE

./src/ansiprsr.o: ./src/ansiprsr.h ./src/tnconfig.h ./src/tnerror.h
./src/ansiprsr.o: ./src/tnmsg.h ./src/tparser.h ./src/tconsole.h
./src/ansiprsr.o: ./src/keytrans.h ./src/tkeydef.h ./src/tkeymap.h
./src/ansiprsr.o: ./src/stl_bids.h ./src/tscroll.h ./src/tmouse.h
./src/ansiprsr.o: ./src/tnclip.h ./src/tnetwork.h ./src/tcharmap.h
./src/keytrans.o: ./src/keytrans.h ./src/tkeydef.h ./src/tkeymap.h
./src/keytrans.o: ./src/stl_bids.h ./src/tnerror.h ./src/tnmsg.h
./src/tcharmap.o: ./src/tcharmap.h ./src/tnconfig.h ./src/tnerror.h
./src/tcharmap.o: ./src/tnmsg.h
./src/tconsole.o: ./src/tconsole.h ./src/tnconfig.h ./src/tnerror.h
./src/tconsole.o: ./src/tnmsg.h
./src/tkeydef.o: ./src/tkeydef.h
./src/tkeymap.o: ./src/tkeymap.h ./src/stl_bids.h ./src/tkeydef.h
./src/tmapldr.o: ./src/tmapldr.h ./src/keytrans.h ./src/tkeydef.h
./src/tmapldr.o: ./src/tkeymap.h ./src/stl_bids.h ./src/tcharmap.h
./src/tmapldr.o: ./src/tnerror.h ./src/tnmsg.h ./src/tnconfig.h
./src/tmouse.o: ./src/tmouse.h ./src/tnclip.h ./src/tnetwork.h
./src/tmouse.o: ./src/tconsole.h ./src/tnconfig.h ./src/tnerror.h
./src/tmouse.o: ./src/tnmsg.h
./src/tnclass.o: ./src/tnclass.h ./src/tnconfig.h ./src/tnerror.h
./src/tnclass.o: ./src/tnmsg.h ./src/ttelhndl.h ./src/tparser.h
./src/tnclass.o: ./src/tconsole.h ./src/keytrans.h ./src/tkeydef.h
./src/tnclass.o: ./src/tkeymap.h ./src/stl_bids.h ./src/tscroll.h
./src/tnclass.o: ./src/tmouse.h ./src/tnclip.h ./src/tnetwork.h
./src/tnclass.o: ./src/tcharmap.h ./src/tncon.h ./src/tparams.h
./src/tnclass.o: ./src/ansiprsr.h ./src/tmapldr.h ./src/tnmisc.h
./src/tnclip.o: ./src/tnclip.h ./src/tnetwork.h
./src/tncon.o: ./src/tncon.h ./src/tparams.h ./src/ttelhndl.h ./src/tparser.h
./src/tncon.o: ./src/tconsole.h ./src/tnconfig.h ./src/tnerror.h
./src/tncon.o: ./src/tnmsg.h ./src/keytrans.h ./src/tkeydef.h ./src/tkeymap.h
./src/tncon.o: ./src/stl_bids.h ./src/tscroll.h ./src/tmouse.h ./src/tnclip.h
./src/tncon.o: ./src/tnetwork.h ./src/tcharmap.h
./src/tnconfig.o: ./src/tnconfig.h ./src/tnerror.h ./src/tnmsg.h
./src/tnerror.o: ./src/tnerror.h ./src/tnmsg.h ./src/ttelhndl.h
./src/tnerror.o: ./src/tparser.h ./src/tconsole.h ./src/tnconfig.h
./src/tnerror.o: ./src/keytrans.h ./src/tkeydef.h ./src/tkeymap.h
./src/tnerror.o: ./src/stl_bids.h ./src/tscroll.h ./src/tmouse.h
./src/tnerror.o: ./src/tnclip.h ./src/tnetwork.h ./src/tcharmap.h
./src/tnetwork.o: ./src/tnetwork.h
./src/tnmain.o: ./src/tnmain.h ./src/tncon.h ./src/tparams.h ./src/ttelhndl.h
./src/tnmain.o: ./src/tparser.h ./src/tconsole.h ./src/tnconfig.h
./src/tnmain.o: ./src/tnerror.h ./src/tnmsg.h ./src/keytrans.h
./src/tnmain.o: ./src/tkeydef.h ./src/tkeymap.h ./src/stl_bids.h
./src/tnmain.o: ./src/tscroll.h ./src/tmouse.h ./src/tnclip.h
./src/tnmain.o: ./src/tnetwork.h ./src/tcharmap.h ./src/tnclass.h
./src/tnmain.o: ./src/ansiprsr.h ./src/tmapldr.h ./src/tnmisc.h
./src/tnmisc.o: ./src/tnmisc.h
./src/tscript.o: ./src/tscript.h ./src/tnetwork.h
./src/tscroll.o: ./src/tscroll.h ./src/tconsole.h ./src/tnconfig.h
./src/tscroll.o: ./src/tnerror.h ./src/tnmsg.h ./src/tmouse.h ./src/tnclip.h
./src/tscroll.o: ./src/tnetwork.h ./src/tncon.h ./src/tparams.h
./src/tscroll.o: ./src/ttelhndl.h ./src/tparser.h ./src/keytrans.h
./src/tscroll.o: ./src/tkeydef.h ./src/tkeymap.h ./src/stl_bids.h
./src/tscroll.o: ./src/tcharmap.h
./src/ttelhndl.o: ./src/ttelhndl.h ./src/tparser.h ./src/tconsole.h
./src/ttelhndl.o: ./src/tnconfig.h ./src/tnerror.h ./src/tnmsg.h
./src/ttelhndl.o: ./src/keytrans.h ./src/tkeydef.h ./src/tkeymap.h
./src/ttelhndl.o: ./src/stl_bids.h ./src/tscroll.h ./src/tmouse.h
./src/ttelhndl.o: ./src/tnclip.h ./src/tnetwork.h ./src/tcharmap.h
./src/ttelhndl.o: ./src/telnet.h ./src/tparams.h
