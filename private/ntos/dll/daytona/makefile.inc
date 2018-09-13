!if exist(..\$(TARGET_DIRECTORY).inc)
!include ..\$(TARGET_DIRECTORY).inc
!endif

SERVICES_DIR=$(TARGET_DIRECTORY)
SERVICES_TAB=$(SERVICES_DIR)\services.tab
SERVICES_STB=..\..\ke\$(TARGET_DIRECTORY)\services.stb
NTDLL_XTR=..\$(TARGET_DIRECTORY)\ntdll.xtr

$(SERVICES_TAB): ..\..\ke\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

obj\$(TARGET_DIRECTORY)\usrstubs.obj: \
    $(TARGET_DIRECTORY)\usrstubs.$(ASM_SUFFIX) obj\$(TARGET_DIRECTORY)\ntdll.def

$(NTDLL_XTR): $(SERVICES_TAB)
    gensrv -f $(NTDLL_XTR) -s ..\..\ke $(SERVICES_DIR)

$(TARGET_DIRECTORY)\usrstubs.$(ASM_SUFFIX): $(SERVICES_TAB) $(SERVICES_STB)
    gensrv -d $(TARGET_DIRECTORY) -e $(ASM_SUFFIX) $(TARGET_BRACES) -s ..\..\ke $(SERVICES_DIR)

obj\$(TARGET_DIRECTORY)\ntdll.def: ..\ntdlldef.src ..\$(TARGET_DIRECTORY)def.src $(NTDLL_XTR)
    copy ..\ntdlldef.src+..\$(TARGET_DIRECTORY)def.src+$(NTDLL_XTR) obj\$(TARGET_DIRECTORY)\ntdll.pp
    $(TARGET_CPP) /EP $(CDEFINES) obj\$(TARGET_DIRECTORY)\ntdll.pp > obj\$(TARGET_DIRECTORY)\ntdll.def
    -del obj\$(TARGET_DIRECTORY)\ntdll.pp

..\ntdll.rc: $(BASEDIR)\private\genx\nls\obj\$(TARGET_DIRECTORY)\ntstatus.rc $(BASEDIR)\private\genx\nls\obj\$(TARGET_DIRECTORY)\MSG00001.bin

