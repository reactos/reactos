!include $(CCSHELL_DIR)\comdlg32\comdlg32.inc

NO_BROWSER_FILE = 1

SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\comdlg32\comdlg32.inc

TARGETNAME = srccpp
TARGETPATH = obj
TARGETTYPE = LIBRARY

SOURCES    = ..\filemru.cpp  \
             ..\filenew.cpp   \
             ..\printnew.cpp  \
             ..\tlog.cpp      \
             ..\util.cpp

# PRECOMPILED_CXX = 1
# PRECOMPILED_INCLUDE = ..\pch.h

