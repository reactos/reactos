# $Id: Makefile,v 1.1 2001/12/15 11:00:53 ea Exp $
#
# PROJECT    : ReactOS Operating System
# FILE       : reactos/dk/w32/Makefile
# DESCRIPTION: Build the development kit for user mode application
# DESCRIPTION: to be run in the subsystems 2 (WINDOWS_GUI)
# DESCRIPTION: and 3 (WINDOWS_CUI).
# LICENSE    : GNU GPL V2.0
#
PATH_TO_TOP=../..

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
	$(TARGET_FOLDER_LIB)/secur32.a

MODULES_SHELL=\
	$(TARGET_FOLDER_LIB)/shell32.a\

MODULES_NET=\
	$(TARGET_FOLDER_LIB)/msafd.a\
	$(TARGET_FOLDER_LIB)/ws2_32.a

MODULES_COM=\
	$(TARGET_FOLDER_LIB)/ole32.a\
	$(TARGET_FOLDER_LIB)/oleaut32.a
#	$(TARGET_FOLDER_LIB)/rpcrt4.a

MODULES_CRT=\
	$(TARGET_FOLDER_LIB)/crtdll.a\
	$(TARGET_FOLDER_LIB)/msvcrt.a

MODULES_MISC=\
	$(TARGET_FOLDER_LIB)/version.a

MODULES =\
	$(MODULES_CORE)\
	$(MODULES_ADVANCED)\
	$(MODULES_SHELL)\
	$(MODULES_NET)\
	$(MODULES_COM)\
	$(MODULES_CRT)\
	$(MODULES_MISC)

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

# --- SHELL ---

$(TARGET_FOLDER_LIB)/shell32.a: $(PATH_TO_TOP)/lib/shell32/shell32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D shell32.dll\
		-d $(PATH_TO_TOP)/lib/shell32/shell32.def

# --- NET ---

$(TARGET_FOLDER_LIB)/msafd.a: $(PATH_TO_TOP)/lib/msafd/msafd.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D msafd.dll\
		-d $(PATH_TO_TOP)/lib/msafd/msafd.def


$(TARGET_FOLDER_LIB)/ws2_32.a: $(PATH_TO_TOP)/lib/ws2_32/ws2_32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D ws2_32.dll\
		-d $(PATH_TO_TOP)/lib/ws2_32/ws2_32.def

# --- COM ---

$(TARGET_FOLDER_LIB)/ole32.a: $(PATH_TO_TOP)/lib/ole32/ole32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D ole32.dll\
		-d $(PATH_TO_TOP)/lib/ole32/ole32.def

$(TARGET_FOLDER_LIB)/oleaut32.a: $(PATH_TO_TOP)/lib/oleaut32/oleaut32.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D oleaut32.dll\
		-d $(PATH_TO_TOP)/lib/oleaut32/oleaut32.def

$(TARGET_FOLDER_LIB)/rpcrt4.a: $(PATH_TO_TOP)/lib/rpcrt4/rpcrt4.def
	$(DLLTOOL) \
		$(DTFLAGS)\
		-D rpcrt4.dll\
		-d $(PATH_TO_TOP)/lib/rpcrt4/rpcrt4.def

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

# --- Service rules ---

clean:
	- $(RM) $(TARGET_FOLDER_LIB)/*.a
	- $(RM) $(TARGET_FOLDER_INC)/*.h

install:

dist:

#EOF

