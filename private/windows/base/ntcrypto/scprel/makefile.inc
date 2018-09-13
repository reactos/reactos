
!if "$(_NTTREE)" != ""
NTTARGETFILE1 = \
    $(NTTARGETFILE1) \
    $(_NTTREE)\Symbols\dll\rsabase.dbg
!endif


all:	$(NTTARGETFILE1)

clean:
    -del $(NTTARGETFILE1)

$(TARGETPATH)\$(TARGET_DIRECTORY)\rsabase.lib:	$(O)\rsabase.lib
    copy $(O)\rsabase.lib $(TARGETPATH)\$(TARGET_DIRECTORY)\rsabase.lib

$(TARGETPATH)\$(TARGET_DIRECTORY)\rsabase.exp:	$(O)\rsabase.exp
    copy $(O)\rsabase.exp $(TARGETPATH)\$(TARGET_DIRECTORY)\rsabase.exp

!if "$(_NTTREE)" != ""
$(_NTTREE)\Symbols\dll\rsabase.dbg:	$(O)\rsabase.dbg
	-md $(_NTTREE)\Symbols >nul 2>&1
	-md $(_NTTREE)\Symbols\dll >nul 2>&1
	copy $(O)\rsabase.dbg $(_NTTREE)\Symbols\dll\rsabase.dbg
!endif
