PATH_TO_TOP = ../..

TARGET=rgenstat$(EXE_POSTFIX)

all: $(TARGET)

OBJECTS = rgenstat.o llmosrt.o

CLEAN_FILES = *.o rgenstat$(EXE_POSTFIX)

HOST_CFLAGS = -I.

rgenstat.o: rgenstat.c
	$(HOST_CC) $(HOST_CFLAGS) -c rgenstat.c -o rgenstat.o

llmosrt.o: llmosrt.c
	$(HOST_CC) $(HOST_CFLAGS) -c llmosrt.c -o llmosrt.o

rgenstat$(EXE_POSTFIX): $(OBJECTS)
	$(HOST_CC) $(OBJECTS) -o rgenstat$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	rm -f *.o
	rm -f rgenstat$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	del *.o
	del rgenstat$(EXE_POSTFIX)
endif

web/apistatus.html: web/apistatus.xml web/rapistatus.xsl
	$(XSLTPROC) -o web/apistatus.html web/rapistatus.xsl web/apistatus.xml

web: web/apistatus.html
	-

.phony: clean web

include $(PATH_TO_TOP)/rules.mak

# EOF
