PATH_TO_TOP = ..

TOOLS = \
  rcopy$(EXE_POSTFIX) \
  rdel$(EXE_POSTFIX) \
  rmkdir$(EXE_POSTFIX) \
  rrmdir$(EXE_POSTFIX) 

CLEAN_FILES = $(TOOLS)

all: $(TOOLS)

ifeq ($(HOST),mingw32-linux)
rcopy$(EXE_POSTFIX): rcopy.c
	$(HOST_CC) $(CFLAGS) -DUNIX_PATHS rcopy.c -o rcopy$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
rcopy$(EXE_POSTFIX): rcopy.c
	$(HOST_CC) $(CFLAGS) -DDOS_PATHS rcopy.c -o rcopy$(EXE_POSTFIX)
endif

rdel$(EXE_POSTFIX): rdel.c
	$(HOST_CC) $(CFLAGS) rdel.c -o rdel$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
rmkdir$(EXE_POSTFIX): rmkdir.c
	$(HOST_CC) $(CFLAGS) -DUNIX_PATHS rmkdir.c -o rmkdir$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
rmkdir$(EXE_POSTFIX): rmkdir.c
	$(HOST_CC) $(CFLAGS) -DDOS_PATHS rmkdir.c -o rmkdir$(EXE_POSTFIX)
endif

ifeq ($(HOST),mingw32-linux)
rrmdir$(EXE_POSTFIX): rrmdir.c
	$(HOST_CC) $(CFLAGS) -DUNIX_PATHS rrmdir.c -o rrmdir$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
rrmdir$(EXE_POSTFIX): rrmdir.c
	$(HOST_CC) $(CFLAGS) -DDOS_PATHS rrmdir.c -o rrmdir$(EXE_POSTFIX)
endif

clean:
	- $(RM) $(CLEAN_FILES)

.PHONY: all clean

include $(PATH_TO_TOP)/rules.mk
