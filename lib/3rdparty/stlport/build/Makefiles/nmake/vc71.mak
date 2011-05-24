#!ifndef MSVC_DIR
#MSVC_DIR = c:\Program Files\Microsoft Visual Studio .NET 2003\VC7
#!endif

CFLAGS_COMMON = /nologo /W4 /GX
CXXFLAGS_COMMON = /nologo /W4 /GX

OPT_REL = $(OPT_REL) /GL
LDFLAGS_REL = $(LDFLAGS_REL) /LTCG


!include vc-common.mak

