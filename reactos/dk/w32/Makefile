# $Id: Makefile,v 1.9 2004/05/28 18:18:43 sedwards Exp $
#
# PROJECT    : ReactOS Operating System
# FILE       : reactos/dk/w32/Makefile
# DESCRIPTION: Build the development kit for user mode application
# DESCRIPTION: to be run in the subsystems 2 (WINDOWS_GUI)
# DESCRIPTION: and 3 (WINDOWS_CUI).
# LICENSE    : GNU GPL V2.0
#
PATH_TO_TOP=../..

SHELL32_ALIAS  = roshel32
COMCTL32_ALIAS = rosctl32


include $(PATH_TO_TOP)/rules.mak


TARGET_FOLDER_LIB=lib
TARGET_FOLDER_INC=include

DTFLAGS = -k -l $@

MODULES_CORE=\
	$(TARGET_FOLDER_LIB)/kernel32.a\
	$(TARGET_FOLDER_LIB)/gdi32.a\
	$(TARGET_FOLDER_LIB)/user32.a

MODULES_ADVANCED=\
	$(TARGET_FOLDER_LIB)/advapi32.a\
	$(TARGET_FOLDER_LIB)/secur32.a\
	$(TARGET_FOLDER_LIB)/imagehlp.a

MODULES_SHELL=\
	$(TARGET_FOLDER_LIB)/comctl32.a\
	$(TARGET_FOLDER_LIB)/shell32.a\
	$(TARGET_FOLDER_LIB)/$(COMCTL32_ALIAS).a\
	$(TARGET_FOLDER_LIB)/$(SHELL32_ALIAS).a

MODULES_NET=\
	$(TARGET_FOLDER_LIB)/iphlpapi.a\
	$(TARGET_FOLDER_LIB)/msafd.a\
	$(TARGET_FOLDER_LIB)/snmpapi.a\
	$(TARGET_FOLDER_LIB)/ws2_32.a\
	$(TARGET_FOLDER_LIB)/wshirda.a\
	$(TARGET_FOLDER_LIB)/wsock32.a

#MODULES_COM=\
#	$(TARGET_FOLDER_LIB)/ole32.a\
#	$(TARGET_FOLDER_LIB)/oleaut32.a\
#	$(TARGET_FOLDER_LIB)/rpcrt4.a

MODULES_CRT=\
	$(TARGET_FOLDER_LIB)/crtdll.a\
	$(TARGET_FOLDER_LIB)/msvcrt.a

MODULES_MISC=\
	$(TARGET_FOLDER_LIB)/version.a\
	$(TARGET_FOLDER_LIB)/winmm.a

MODULES_WINE=\
	$(TARGET_FOLDER_LIB)/rpcrt4.a\
	$(TARGET_FOLDER_LIB)/shlwapi.a\
	$(TARGET_FOLDER_LIB)/ole32.a\
	$(TARGET_FOLDER_LIB)/oleaut32.a\
	$(TARGET_FOLDER_LIB)/lz32.a\
	$(TARGET_FOLDER_LIB)/wininet.a\
	$(TARGET_FOLDER_LIB)/wine_unicode.a

#	$(TARGET_FOLDER_LIB)/libwine.a


MODULES_CONTRIB=\
	$(TARGET_FOLDER_LIB)/perl58.a

MODULES =\
	$(MODULES_CORE)\
	$(MODULES_COM)\
	$(MODULES_ADVANCED)\
	$(MODULES_SHELL)\
	$(MODULES_NET)\
	$(MODULES_CRT)\
	$(MODULES_MISC)\
	$(MODULES_WINE)

#	$(MODULES_CONTRIB)\

# --- Rules ---

all: $(MODULES)

# --- CORE ---



$(TARGET_FOLDER_LIB)/kernel32.a: $(PATH_TO_TOP)/lib/kernel32/kernel32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D kernel32.dll\
		-d $(PATH_TO_TOP)/lib/kernel32/kernel32.def
	
$(TARGET_FOLDER_LIB)/gdi32.a: $(PATH_TO_TOP)/lib/gdi32/gdi32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D gdi32.dll\
		-d $(PATH_TO_TOP)/lib/gdi32/gdi32.def

$(TARGET_FOLDER_LIB)/user32.a: $(PATH_TO_TOP)/lib/user32/user32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D user32.dll\
		-d $(PATH_TO_TOP)/lib/user32/user32.def

# --- ADVANCED ---

$(TARGET_FOLDER_LIB)/advapi32.a: $(PATH_TO_TOP)/lib/advapi32/advapi32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D advapi32.dll\
		-d $(PATH_TO_TOP)/lib/advapi32/advapi32.def

$(TARGET_FOLDER_LIB)/secur32.a: $(PATH_TO_TOP)/lib/secur32/secur32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D secur32.dll\
		-d $(PATH_TO_TOP)/lib/secur32/secur32.def

$(TARGET_FOLDER_LIB)/imagehlp.a: $(PATH_TO_TOP)/lib/imagehlp/imagehlp.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D imagehlp.dll\
		-d $(PATH_TO_TOP)/lib/imagehlp/imagehlp.def
# --- SHELL ---

$(TARGET_FOLDER_LIB)/shell32.a: $(PATH_TO_TOP)/lib/shell32/shell32.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D shell32.dll\
		-d $(PATH_TO_TOP)/lib/shell32/shell32.spec.def

$(TARGET_FOLDER_LIB)/comctl32.a: $(PATH_TO_TOP)/lib/comctl32/comctl32.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D comctl32.dll\
		-d $(PATH_TO_TOP)/lib/comctl32/comctl32.spec.def


$(TARGET_FOLDER_LIB)/$(SHELL32_ALIAS).a: $(PATH_TO_TOP)/lib/shell32/shell32.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D $(SHELL32_ALIAS).dll\
		-d $(PATH_TO_TOP)/lib/shell32/shell32.spec.def

$(TARGET_FOLDER_LIB)/$(COMCTL32_ALIAS).a: $(PATH_TO_TOP)/lib/comctl32/comctl32.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D $(COMCTL32_ALIAS).dll\
		-d $(PATH_TO_TOP)/lib/comctl32/comctl32.spec.def

#$(TARGET_FOLDER_LIB)/$(COMCTL32_ALIAS).a: $(PATH_TO_TOP)/lib/comctl32/comctl32.def
#		-d $(PATH_TO_TOP)/lib/comctl32/comctl32.def

# --- NET ---

$(TARGET_FOLDER_LIB)/iphlpapi.a: $(PATH_TO_TOP)/lib/iphlpapi/iphlpapi.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D iphlpapi.dll\
		-d $(PATH_TO_TOP)/lib/iphlpapi/iphlpapi.spec.def

$(TARGET_FOLDER_LIB)/msafd.a: $(PATH_TO_TOP)/lib/msafd/msafd.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D msafd.dll\
		-d $(PATH_TO_TOP)/lib/msafd/msafd.def

$(TARGET_FOLDER_LIB)/snmpapi.a: $(PATH_TO_TOP)/lib/snmpapi/snmpapi.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D snmpapi.dll\
		-d $(PATH_TO_TOP)/lib/snmpapi/snmpapi.def

$(TARGET_FOLDER_LIB)/ws2_32.a: $(PATH_TO_TOP)/lib/ws2_32/ws2_32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D ws2_32.dll\
		-d $(PATH_TO_TOP)/lib/ws2_32/ws2_32.def

$(TARGET_FOLDER_LIB)/wshirda.a: $(PATH_TO_TOP)/lib/wshirda/wshirda.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D wshirda.dll\
		-d $(PATH_TO_TOP)/lib/wshirda/wshirda.def

$(TARGET_FOLDER_LIB)/wsock32.a: $(PATH_TO_TOP)/lib/wsock32/wsock32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D wsock32.dll\
		-d $(PATH_TO_TOP)/lib/wsock32/wsock32.def

# --- COM ---

$(TARGET_FOLDER_LIB)/ole32.a: $(PATH_TO_TOP)/lib/ole32/ole32.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D ole32.dll\
		-d $(PATH_TO_TOP)/lib/ole32/ole32.spec.def

$(TARGET_FOLDER_LIB)/oleaut32.a: $(PATH_TO_TOP)/lib/oleaut32/oleaut32.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D oleaut32.dll\
		-d $(PATH_TO_TOP)/lib/oleaut32/oleaut32.spec.def

$(TARGET_FOLDER_LIB)/rpcrt4.a: $(PATH_TO_TOP)/lib/rpcrt4/rpcrt4.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D rpcrt4.dll\
		-d $(PATH_TO_TOP)/lib/rpcrt4/rpcrt4.spec.def

# --- CRT ---

$(TARGET_FOLDER_LIB)/crtdll.a: $(PATH_TO_TOP)/lib/crtdll/crtdll.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D crtdll.dll\
		-d $(PATH_TO_TOP)/lib/crtdll/crtdll.def

$(TARGET_FOLDER_LIB)/msvcrt.a: $(PATH_TO_TOP)/lib/msvcrt/msvcrt.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D msvcrt.dll\
		-d $(PATH_TO_TOP)/lib/msvcrt/msvcrt.def

# --- MISC ---

$(TARGET_FOLDER_LIB)/version.a: $(PATH_TO_TOP)/lib/version/version.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D version.dll\
		-d $(PATH_TO_TOP)/lib/version/version.def

$(TARGET_FOLDER_LIB)/winmm.a: $(PATH_TO_TOP)/lib/winmm/winmm.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D winmm.dll\
		-d $(PATH_TO_TOP)/lib/winmm/winmm.spec.def

# --- WINE ---

#$(TARGET_FOLDER_LIB)/ole32.a: $(PATH_TO_TOP)/../wine/dlls/ole32/ole32.spec.def
#	$(DLLTOOL) \
#		$(DTFLAGS)\
#		-D ole32.dll\
#		-d $(PATH_TO_TOP)/../wine/dlls/ole32/ole32.spec.def

#$(TARGET_FOLDER_LIB)/oleaut32.a: $(PATH_TO_TOP)/../wine/dlls/oleaut32/oleaut32.spec.def
#	$(DLLTOOL) \
#		$(DTFLAGS)\
#		-D oleaut32.dll\
#		-d $(PATH_TO_TOP)/../wine/dlls/oleaut32/oleaut32.spec.def

#$(TARGET_FOLDER_LIB)/rpcrt4.a: $(PATH_TO_TOP)/../wine/dlls/rpcrt4/rpcrt4.def
#	$(DLLTOOL) \
#		$(DTFLAGS)\
#		-D rpcrt4.dll\
#		-d $(PATH_TO_TOP)/../wine/dlls/rpcrt4/rpcrt4.def

$(TARGET_FOLDER_LIB)/shlwapi.a: $(PATH_TO_TOP)/lib/shlwapi/shlwapi.spec.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D shlwapi.dll\
		-d $(PATH_TO_TOP)/lib/shlwapi/shlwapi.spec.def


$(TARGET_FOLDER_LIB)/wininet.a: $(PATH_TO_TOP)/lib/wininet/wininet.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D wininet.dll\
		-d $(PATH_TO_TOP)/lib/wininet/wininet.def

$(TARGET_FOLDER_LIB)/lz32.a: $(PATH_TO_TOP)/lib/lzexpand/lz32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D lz32.dll\
		-d $(PATH_TO_TOP)/lib/lzexpand/lz32.def

#$(TARGET_FOLDER_LIB)/libwine.a: $(PATH_TO_TOP)/../wine/libs/wine/wine.def
#	$(DLLTOOL) \
#		$(DTFLAGS)\
#		-D libwine.dll\
#		-d $(PATH_TO_TOP)/../wine/libs/wine/wine.def

$(TARGET_FOLDER_LIB)/wine_unicode.a: $(PATH_TO_TOP)/lib/unicode/wine_unicode.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D wine_unicode.dll\
		-d $(PATH_TO_TOP)/lib/unicode/wine_unicode.def

# --- CONTRIB ---

$(TARGET_FOLDER_LIB)/perl58.a: $(PATH_TO_TOP)/../contrib/perl-5.8.0/win32/perldll.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D perl58.dll\
		-d $(PATH_TO_TOP)/../contrib/perl-5.8.0/win32/perldll.def

# --- Service rules ---

clean:
	- $(RM) $(TARGET_FOLDER_LIB)/$(MODULES)

#	- $(RM) $(TARGET_FOLDER_LIB)/*.a
#	- $(RM) $(TARGET_FOLDER_INC)/*.h

install:

#EOF

