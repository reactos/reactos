PATH_TO_TOP = ..

TOOLS = \
  buildno$(EXE_POSTFIX) \
  depends$(EXE_POSTFIX) \
  rcopy$(EXE_POSTFIX) \
  rdel$(EXE_POSTFIX) \
  rline$(EXE_POSTFIX) \
  rmkdir$(EXE_POSTFIX) \
  rrmdir$(EXE_POSTFIX) \
  rsym$(EXE_POSTFIX) \
  rtouch$(EXE_POSTFIX) \
  mkflpimg$(EXE_POSTFIX)

CLEAN_FILES = $(TOOLS)

all: $(TOOLS) wmc_directory_target cdmake_directory_target

buildno$(EXE_POSTFIX): buildno.c ../include/reactos/version.h
	$(HOST_CC) $(CFLAGS) -o buildno$(EXE_POSTFIX) buildno.c
	$(EXE_PREFIX)buildno$(EXE_POSTFIX)

depends$(EXE_POSTFIX): depends.c
	$(HOST_CC) $(CFLAGS) -o depends$(EXE_POSTFIX) depends.c

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
rline$(EXE_POSTFIX): rline.c
	$(HOST_CC) $(CFLAGS) -DUNIX_PATHS rline.c -o rline$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
rline$(EXE_POSTFIX): rmkdir.c
	$(HOST_CC) $(CFLAGS) -DDOS_PATHS rline.c -o rline$(EXE_POSTFIX)
endif

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

ifeq ($(HOST),mingw32-linux)
rsym$(EXE_POSTFIX): rsym.c
	$(HOST_CC) $(CFLAGS) -DUNIX_PATHS rsym.c -o rsym$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
rsym$(EXE_POSTFIX): rsym.c
	$(HOST_CC) $(CFLAGS) -DDOS_PATHS rsym.c -o rsym$(EXE_POSTFIX)
endif

ifeq ($(HOST),mingw32-linux)
rtouch$(EXE_POSTFIX): rtouch.c
	$(HOST_CC) $(CFLAGS) -DUNIX_PATHS rtouch.c -o rtouch$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
rtouch$(EXE_POSTFIX): rtouch.c
	$(HOST_CC) $(CFLAGS) -DDOS_PATHS rtouch.c -o rtouch$(EXE_POSTFIX)
endif

ifeq ($(HOST),mingw32-linux)
mkflpimg$(EXE_POSTFIX): mkflpimg.c
	$(HOST_CC) $(CFLAGS) -DUNIX_PATHS mkflpimg.c -o mkflpimg$(EXE_POSTFIX)
endif
ifeq ($(HOST),mingw32-windows)
mkflpimg$(EXE_POSTFIX): mkflpimg.c
	$(HOST_CC) $(CFLAGS) -DDOS_PATHS mkflpimg.c -o mkflpimg$(EXE_POSTFIX)
endif


wmc_directory_target:
	$(MAKE) -C wmc wmc$(EXE_POSTFIX)

cdmake_directory_target:
	$(MAKE) -C cdmake cdmake$(EXE_POSTFIX)

ifeq ($(HOST),mingw32-linux)
clean:
	$(MAKE) -C cdmake clean
	$(MAKE) -C wmc clean
	rm mkconfig
	rm $(TOOLS)
endif
ifeq ($(HOST),mingw32-windows)
clean:
	$(MAKE) -C cdmake clean
	$(MAKE) -C wmc clean
	del *$(EXE_POSTFIX)
endif

.PHONY: all clean

include $(PATH_TO_TOP)/rules.mak
