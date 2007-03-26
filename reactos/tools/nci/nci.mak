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
	$(EXEPREFIX)$(NCI_OUT_)nci$(EXEPOSTFIX)

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

.PHONY: nci_clean
nci_clean:
	-@$(rm) $(NCI_TARGET) $(NCI_OBJECTS) 2>$(NUL)
clean: nci_clean

# WIN32K.SYS
WIN32K_SVC_DB = $(NCI_BASE_)w32ksvc.db
WIN32K_SERVICE_TABLE = subsystems$(SEP)win32$(SEP)win32k$(SEP)include$(SEP)napi.h
WIN32K_GDI_STUBS = dll$(SEP)win32$(SEP)gdi32$(SEP)misc$(SEP)$(ARCH)$(SEP)win32k.S
WIN32K_USER_STUBS = dll$(SEP)win32$(SEP)user32$(SEP)misc$(SEP)$(ARCH)$(SEP)win32k.S

# NTOSKRNL.EXE
KERNEL_SVC_DB = $(NCI_BASE_)sysfuncs.lst
KERNEL_SERVICE_TABLE = ntoskrnl$(SEP)include$(SEP)internal$(SEP)napi.h
NTDLL_STUBS = dll$(SEP)ntdll$(SEP)$(ARCH)$(SEP)napi.S
KERNEL_STUBS = ntoskrnl$(SEP)ex$(SEP)$(ARCH)$(SEP)zw.S

NCI_SERVICE_FILES = \
	$(KERNEL_SERVICE_TABLE) \
	$(WIN32K_SERVICE_TABLE) \
	$(NTDLL_STUBS) \
	$(KERNEL_STUBS) \
	$(WIN32K_GDI_STUBS) \
	$(WIN32K_USER_STUBS)

$(NCI_SERVICE_FILES): $(NCI_TARGET) $(KERNEL_SVC_DB) $(WIN32K_SVC_DB)
	$(ECHO_NCI)
	${mkdir} dll$(SEP)ntdll$(SEP)$(ARCH) 2>$(NUL)
	${mkdir} ntoskrnl$(SEP)ex$(SEP)$(ARCH) 2>$(NUL)
	${mkdir} dll$(SEP)win32$(SEP)gdi32$(SEP)misc$(SEP)$(ARCH) 2>$(NUL)
	${mkdir} dll$(SEP)win32$(SEP)user32$(SEP)misc$(SEP)$(ARCH) 2>$(NUL)
	$(Q)$(NCI_TARGET) -arch $(ARCH) \
		$(KERNEL_SVC_DB) \
		$(WIN32K_SVC_DB) \
		$(KERNEL_SERVICE_TABLE) \
		$(WIN32K_SERVICE_TABLE) \
		$(NTDLL_STUBS) \
		$(KERNEL_STUBS) \
		$(WIN32K_GDI_STUBS) \
		$(WIN32K_USER_STUBS)

.PHONY: nci_service_files_clean
nci_service_files_clean:
	-@$(rm) $(NCI_SERVICE_FILES) 2>$(NUL)
clean: nci_service_files_clean
