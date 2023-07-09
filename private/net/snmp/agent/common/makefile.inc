evtlog.h : evtlog.mc
    mc -r ..\proxy -v evtlog.mc

renameit : $(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\$(TARGETNAME).lib
    copy $(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\$(TARGETNAME).lib $(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\snmp.lib
