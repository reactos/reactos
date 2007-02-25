CMLIB_BASE = $(LIB_BASE_)cmlib
CMLIB_BASE_ = $(CMLIB_BASE)$(SEP)
CMLIB_INT = $(INTERMEDIATE_)$(CMLIB_BASE)_host
CMLIB_INT_ = $(INTERMEDIATE_)$(CMLIB_BASE)_host$(SEP)
CMLIB_OUT = $(OUTPUT_)$(CMLIB_BASE)_host
CMLIB_OUT_ = $(OUTPUT_)$(CMLIB_BASE)_host$(SEP)

$(CMLIB_INT): | $(LIB_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(CMLIB_OUT): | $(OUTPUT_)$(LIB_BASE)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

CMLIB_HOST_TARGET = \
	$(CMLIB_OUT)$(SEP)cmlib.a

CMLIB_HOST_SOURCES = $(addprefix $(CMLIB_BASE_), \
	cminit.c \
	hivebin.c \
	hivecell.c \
	hiveinit.c \
	hivesum.c \
	hivewrt.c \
	)

CMLIB_HOST_OBJECTS = \
	$(subst $(CMLIB_BASE), $(CMLIB_INT), $(CMLIB_HOST_SOURCES:.c=.o))

CMLIB_HOST_CFLAGS = -O3 -Wall -Wwrite-strings -Wpointer-arith \
  -D_X86_ -D__i386__ -D_REACTOS_ -D_NTOSKRNL_ -D_NTSYSTEM_ \
  -DCMLIB_HOST -D_M_IX86 -I$(CMLIB_BASE) -Iinclude/reactos -Iinclude/psdk -Iinclude/ddk -Iinclude/crt \
  -D__NO_CTYPE_INLINES

$(CMLIB_HOST_TARGET): $(CMLIB_HOST_OBJECTS) | $(CMLIB_OUT)
	$(ECHO_AR)
	$(host_ar) -r $@ $(CMLIB_HOST_OBJECTS)

$(CMLIB_INT_)cminit.o: $(CMLIB_BASE_)cminit.c | $(CMLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(CMLIB_HOST_CFLAGS) -c $< -o $@

$(CMLIB_INT_)hivebin.o: $(CMLIB_BASE_)hivebin.c | $(CMLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(CMLIB_HOST_CFLAGS) -c $< -o $@

$(CMLIB_INT_)hivecell.o: $(CMLIB_BASE_)hivecell.c | $(CMLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(CMLIB_HOST_CFLAGS) -c $< -o $@

$(CMLIB_INT_)hiveinit.o: $(CMLIB_BASE_)hiveinit.c | $(CMLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(CMLIB_HOST_CFLAGS) -c $< -o $@

$(CMLIB_INT_)hivesum.o: $(CMLIB_BASE_)hivesum.c | $(CMLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(CMLIB_HOST_CFLAGS) -c $< -o $@

$(CMLIB_INT_)hivewrt.o: $(CMLIB_BASE_)hivewrt.c | $(CMLIB_INT)
	$(ECHO_CC)
	${host_gcc} $(CMLIB_HOST_CFLAGS) -c $< -o $@

.PHONY: cmlib_host
cmlib_host: $(CMLIB_HOST_TARGET)

.PHONY: cmlib_host_clean
cmlib_host_clean:
	-@$(rm) $(CMLIB_HOST_TARGET) $(CMLIB_HOST_OBJECTS) 2>$(NUL)
clean: cmlib_host_clean
