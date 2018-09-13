wowit.c wowit.h: ..\genwowit.c ..\wow.it
    -erase wow.it
    -erase $(WINDIR)\idw\genwowit.exe
    -erase obj\$(TARGET_DIRECTORY)\genwowit.obj
    -erase obj\$(TARGET_DIRECTORY)\genwowit.exe
    set PASS0ONLY=
    set MAKEDLL=
    nmake obj\$(TARGET_DIRECTORY)\genwowit.obj USE_MSVCRT=1 MAKE_GENWOWIT=1
    nmake obj\$(TARGET_DIRECTORY)\genwowit.res USE_MSVCRT=1 MAKE_GENWOWIT=1
    link -out:obj\$(TARGET_DIRECTORY)\genwowit.exe @<<
-STACK:262144,4096
-MERGE:_PAGE=PAGE 
-MERGE:_TEXT=.text 
-SECTION:INIT,d 
-OPT:REF 
-OPT:ICF 
-INCREMENTAL:NO 
-RELEASE 
-FULLBUILD 
-FORCE:MULTIPLE 
-IGNORE:4001,4037,4039,4065,4070,4078,4087,4089 
-NODEFAULTLIB 
-debug:notmapped,MINIMAL 
-debugtype:coff 
-version:5.00 
-osversion:5.00 
-PDB:NONE 
-subsystem:console,4.00
-base:@$(BASEDIR)\public\sdk\lib\coffbase.txt,usermode
-entry:mainCRTStartup 
-optidata 
obj\$(TARGET_DIRECTORY)\genwowit.obj
obj\$(TARGET_DIRECTORY)\genwowit.res
$(SDK_LIB_PATH)\msvcrt.lib
<<
    obj\$(TARGET_DIRECTORY)\genwowit.exe ..\wow.it
