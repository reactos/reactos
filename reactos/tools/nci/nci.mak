NCI_BASE = tools$(SEP)nci

CDMAKE_BASE_DIR = $(INTERMEDIATE)$(NCI_BASE)

$(CDMAKE_BASE_DIR): $(INTERMEDIATE_NO_SLASH) $(RMKDIR_TARGET)
	${mkdir} $(INTERMEDIATE)$(NCI_BASE)

NCI_TARGET = \
	$(INTERMEDIATE)$(NCI_BASE)$(SEP)nci$(EXEPOSTFIX)

NCI_SOURCES = \
	$(NCI_BASE)$(SEP)ncitool.c

NCI_OBJECTS = \
    $(addprefix $(INTERMEDIATE), $(NCI_SOURCES:.c=.o))

NCI_HOST_CFLAGS = -Iinclude -g -Werror -Wall

NCI_HOST_LFLAGS = -g

$(NCI_TARGET): $(CDMAKE_BASE_DIR) $(NCI_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(NCI_OBJECTS) $(NCI_HOST_CFLAGS) -o $(NCI_TARGET)

$(INTERMEDIATE)$(NCI_BASE)$(SEP)ncitool.o: $(CDMAKE_BASE_DIR) $(NCI_BASE)$(SEP)ncitool.c
	$(ECHO_CC)
	${host_gcc} $(NCI_HOST_CFLAGS) -c $(NCI_BASE)$(SEP)ncitool.c -o $(INTERMEDIATE)$(NCI_BASE)$(SEP)ncitool.o

.PHONY: nci_clean
nci_clean:
	-@$(rm) $(NCI_TARGET) $(NCI_OBJECTS) 2>$(NUL)
clean: nci_clean

# WIN32K.SYS
WIN32K_SVC_DB = $(NCI_BASE)$(SEP)w32ksvc.db
WIN32K_SERVICE_TABLE = subsys$(SEP)win32k$(SEP)main$(SEP)svctab.c
WIN32K_GDI_STUBS = lib$(SEP)gdi32$(SEP)misc$(SEP)win32k.S
WIN32K_USER_STUBS = lib$(SEP)user32$(SEP)misc$(SEP)win32k.S

# NTOSKRNL.EXE
KERNEL_SVC_DB = $(NCI_BASE)$(SEP)sysfuncs.lst
KERNEL_SERVICE_TABLE = include$(SEP)ntdll$(SEP)napi.h
NTDLL_STUBS = lib$(SEP)ntdll$(SEP)napi.S
KERNEL_STUBS = ntoskrnl$(SEP)ex$(SEP)zw.S

NCI_SERVICE_FILES = \
	$(KERNEL_SERVICE_TABLE) \
	$(WIN32K_SERVICE_TABLE) \
	$(NTDLL_STUBS) \
	$(KERNEL_STUBS) \
	$(WIN32K_GDI_STUBS) \
	$(WIN32K_USER_STUBS)

$(NCI_SERVICE_FILES): $(NCI_TARGET)
	$(ECHO_NCI)
	$(Q)$(EXEPREFIX)$(NCI_TARGET) \
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
