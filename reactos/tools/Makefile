PATH_TO_TOP = ..

include $(PATH_TO_TOP)/rules.mak

CFLAGS += -Wall -Werror
ifeq ($(HOST),mingw32-linux)
CFLAGS += -DUNIX_PATHS
rm := @rm
endif
ifeq ($(HOST),mingw32-windows)
CFLAGS += -DDOS_PATHS
rm := -@del
endif

TOOLS = \
	regtests$(EXE_POSTFIX) \
	rcopy$(EXE_POSTFIX) \
	rdel$(EXE_POSTFIX) \
	rline$(EXE_POSTFIX) \
	rmkdir$(EXE_POSTFIX) \
	rrmdir$(EXE_POSTFIX) \
	rsym$(EXE_POSTFIX) \
	raddr2line$(EXE_POSTFIX) \
	rtouch$(EXE_POSTFIX) \
	mkflpimg$(EXE_POSTFIX) \
	ppc-le2be$(EXE_POSTFIX) \
	hack-coff$(EXE_POSTFIX) \
	depends$(EXE_POSTFIX)

LIBS = lib_unicode lib_wpp

CLEAN_FILES = $(TOOLS) tools-check.tmp tools-check.h

all: $(TOOLS) tools_check $(LIBS) zlib_target wmc_target cabman_target cdmake_target mkhive_target rgenstat_target \
	wine2ros_target pipetools_target winebuild_target bin2res_target wrc_target widl_target \
	buildno_target

tools_check:
	$(MAKE) -f tools-check.mak

regtests$(EXE_POSTFIX): regtests.c
	@$(HOST_CC) $(CFLAGS) -o $@ $<

rcopy$(EXE_POSTFIX): rcopy.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

rdel$(EXE_POSTFIX): rdel.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

rline$(EXE_POSTFIX): rline.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

rmkdir$(EXE_POSTFIX): rmkdir.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

rrmdir$(EXE_POSTFIX): rrmdir.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

rsym_common.o: rsym_common.c rsym.h
	@$(HOST_CC) $(CFLAGS) -c $< -o $@

rsym.o: rsym.c rsym.h
	@$(HOST_CC) $(CFLAGS) -c $< -o $@

rsym$(EXE_POSTFIX): rsym.o rsym_common.o
	@$(HOST_CC) $(CFLAGS) $^ -o $@

raddr2line.o: raddr2line.c rsym.h
	@$(HOST_CC) $(CFLAGS) -c $< -o $@

raddr2line$(EXE_POSTFIX): raddr2line.o rsym_common.o
	@$(HOST_CC) $(CFLAGS) $^ -o $@

rtouch$(EXE_POSTFIX): rtouch.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

mkflpimg$(EXE_POSTFIX): mkflpimg.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

hack-coff$(EXE_POSTFIX): hack-coff.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

ppc-le2be$(EXE_POSTFIX): ppc-le2be.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

depends$(EXE_POSTFIX): depends.c
	@$(HOST_CC) $(CFLAGS) $< -o $@

.PHONY: zlib_target wmc_target cdmake_target mkhive_target rgenstat_target pipetools_target wrc_target \
        widl_target buildno_target lib_unicode lib_wpp

zlib_target:
	$(MAKE) --silent -C ../lib/zlib -f Makefile.host

wmc_target:
	$(MAKE) --silent -C wmc wmc$(EXE_POSTFIX)

cabman_target:
	$(MAKE) --silent -C cabman cabman$(EXE_POSTFIX)

cdmake_target:
	$(MAKE) --silent -C cdmake cdmake$(EXE_POSTFIX)

mkhive_target:
	$(MAKE) --silent -C mkhive mkhive$(EXE_POSTFIX)

rgenstat_target:
	$(MAKE) --silent -C rgenstat rgenstat$(EXE_POSTFIX)

wine2ros_target:
	$(MAKE) --silent -C wine2ros wine2ros$(EXE_POSTFIX)

pipetools_target:
ifeq ($(HOST),mingw32-windows)
	$(MAKE) --silent -C pipetools
endif

winebuild_target:
	$(MAKE) --silent -C winebuild winebuild$(EXE_POSTFIX)

bin2res_target:
	$(MAKE) --silent -C bin2res bin2res$(EXE_POSTFIX)

wrc_target: $(LIBS)
	$(MAKE) --silent -C wrc wrc$(EXE_POSTFIX)

widl_target: lib_wpp
	$(MAKE) --silent -C widl widl$(EXE_POSTFIX)

buildno_target:
	$(MAKE) --silent -C buildno buildno$(EXE_POSTFIX)

lib_unicode:
	$(MAKE) -C unicode

lib_wpp:
	$(MAKE) -C wpp


clean:
	$(MAKE) --silent -C buildno clean
	$(MAKE) --silent -C widl clean
	$(MAKE) --silent -C wrc clean
	$(MAKE) --silent -C cabman clean
	$(MAKE) --silent -C cdmake clean
	$(MAKE) --silent -C mkhive clean
	$(MAKE) --silent -C wmc clean
	$(MAKE) --silent -C rgenstat clean
	$(MAKE) --silent -C wine2ros clean
	$(MAKE) --silent -C winebuild clean
	$(MAKE) --silent -C bin2res clean
	$(MAKE) --silent -C ../lib/zlib -f Makefile.host clean
	$(MAKE) -C wpp clean
	$(MAKE) -C unicode clean
ifeq ($(HOST),mingw32-linux)
	@rm mkconfig
	@rm $(TOOLS)
endif
ifeq ($(HOST),mingw32-windows)
	$(MAKE) --silent -C pipetools clean
	$(rm) *$(EXE_POSTFIX)
endif

.PHONY: all clean
