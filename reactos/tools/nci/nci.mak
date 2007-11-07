NCI_BASE = $(TOOLS_BASE_)nci
NCI_BASE_ = $(NCI_BASE)$(SEP)
NCI_INT = $(INTERMEDIATE_)$(NCI_BASE)
NCI_INT_ = $(NCI_INT)$(SEP)
NCI_OUT = $(OUTPUT_)$(NCI_BASE)
NCI_OUT_ = $(NCI_OUT)$(SEP)

$(NCI_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(NCI_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

NCI_TARGET = \
	$(NCI_OUT_)nci$(EXEPOSTFIX)

NCI_SOURCES = \
	$(NCI_BASE_)ncitool.c

NCI_OBJECTS = \
    $(addprefix $(INTERMEDIATE_), $(NCI_SOURCES:.c=.o))

NCI_HOST_CFLAGS = -Iinclude $(TOOLS_CFLAGS)

NCI_HOST_LFLAGS = $(TOOLS_LFLAGS)

$(NCI_TARGET): $(NCI_OBJECTS) | $(NCI_OUT)
	$(ECHO_LD)
	${host_gcc} $(NCI_OBJECTS) $(NCI_HOST_LFLAGS) -o $@

$(NCI_INT_)ncitool.o: $(NCI_BASE_)ncitool.c | $(NCI_INT)
	$(ECHO_CC)
	${host_gcc} $(NCI_HOST_CFLAGS) -c $< -o $@

.PHONY: nci
nci: $(NCI_TARGET)

.PHONY: nci_clean
nci_clean:
	-@$(rm) $(NCI_TARGET) $(NCI_OBJECTS) 2>$(NUL)
clean: nci_clean

# WIN32K.SYS
WIN32K_SVC_DB = subsystems$(SEP)win32$(SEP)win32k$(SEP)w32ksvc.db
WIN32K_SERVICE_TABLE = $(INTERMEDIATE_)subsystems$(SEP)win32$(SEP)win32k$(SEP)include$(SEP)napi.h
WIN32K_STUBS = $(INTERMEDIATE_)lib$(SEP)win32ksys$(SEP)win32k.S



# NTOSKRNL.EXE
KERNEL_SVC_DB = ntoskrnl$(SEP)sysfuncs.lst
KERNEL_SERVICE_TABLE = $(INTERMEDIATE_)ntoskrnl$(SEP)include$(SEP)internal$(SEP)napi.h
NTDLL_STUBS = $(INTERMEDIATE_)dll$(SEP)ntdll$(SEP)napi.S
KERNEL_STUBS = $(INTERMEDIATE_)ntoskrnl$(SEP)ex$(SEP)zw.S

NCI_SERVICE_FILES = \
	$(KERNEL_SERVICE_TABLE) \
	$(WIN32K_SERVICE_TABLE) \
	$(NTDLL_STUBS) \
	$(KERNEL_STUBS) \
	$(WIN32K_STUBS)

$(NCI_SERVICE_FILES): $(NCI_TARGET) $(KERNEL_SVC_DB) $(WIN32K_SVC_DB)
	$(ECHO_NCI)
	${mkdir} $(INTERMEDIATE_)ntoskrnl$(SEP)include$(SEP)internal 2>$(NUL)
	${mkdir} $(INTERMEDIATE_)dll$(SEP)ntdll 2>$(NUL)
	${mkdir} $(INTERMEDIATE_)ntoskrnl$(SEP)ex$(SEP) 2>$(NUL)
	${mkdir} $(INTERMEDIATE_)subsystems$(SEP)win32$(SEP)win32k$(SEP)include 2>$(NUL)
	${mkdir} $(INTERMEDIATE_)lib$(SEP)win32ksys 2>$(NUL)

	$(Q)$(NCI_TARGET) -arch $(ARCH) \
		$(KERNEL_SVC_DB) \
		$(WIN32K_SVC_DB) \
		$(KERNEL_SERVICE_TABLE) \
		$(WIN32K_SERVICE_TABLE) \
		$(NTDLL_STUBS) \
		$(KERNEL_STUBS) \
		$(WIN32K_STUBS)

.PHONY: nci_service_files_clean
nci_service_files_clean:
	-@$(rm) $(NCI_SERVICE_FILES) 2>$(NUL)
clean: nci_service_files_clean
