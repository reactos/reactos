#////////////////////////////////////////////////////////////////////
#// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
#// All rights reserved
#////////////////////////////////////////////////////////////////////
# Include make file

#!MESSAGE Including UDF_INFO...

!IF "$(UDF_INFO_CFG)" == "SRC"

#!MESSAGE SRC...

!IF "$(UDF_INFO_PATH)" == ""
!MESSAGE Defaulting UDF_INFO path
UDF_INFO_PATH=.
!ENDIF

UDF_INFO_OBJS= \
    "$(INTDIR)\alloc.obj" \
    "$(INTDIR)\extent.obj" \
    "$(INTDIR)\dirtree.obj" \
    "$(INTDIR)\mount.obj" \
    "$(INTDIR)\phys_eject.obj" \
    "$(INTDIR)\physical.obj" \
    "$(INTDIR)\remap.obj" \
    "$(INTDIR)\udf_info.obj"

!ELSEIF "$(UDF_INFO_CFG)" == "MAKE"

#!MESSAGE MAKE...
# Explain how to make UDF_INFO sources
!IF "$(xCFG)" == "UDF - NT4 Release" || "$(xCFG)" == "UDF - NT4 Debug"

.IGNORE:

REL_PATH=$(UDF_INFO_PATH)
SRC_EXT=cpp

#!MESSAGE :: $(REL_PATH)\alloc.$(SRC_EXT) ::

SRC=alloc
!INCLUDE $(UDF_INFO_PATH)\build.mak

SRC=dirtree
!INCLUDE $(UDF_INFO_PATH)\build.mak

SRC=extent
!INCLUDE $(UDF_INFO_PATH)\build.mak

SRC=mount
!INCLUDE $(UDF_INFO_PATH)\build.mak

SRC=phys_eject
!INCLUDE $(UDF_INFO_PATH)\build.mak

SRC=physical
!INCLUDE $(UDF_INFO_PATH)\build.mak

SRC=remap
!INCLUDE $(UDF_INFO_PATH)\build.mak

SRC=udf_info
!INCLUDE $(UDF_INFO_PATH)\build.mak


!ENDIF


!ENDIF
