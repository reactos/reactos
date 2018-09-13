#
#
# Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
# EXEMPT: copyright change only, no build required
# 
#
#
# copied this from \nt\private\shell\shdocvw\dll\makefile.inc
#

!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)

selfreg_msxml.inf : selfreg.inx
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
        $(CLEANINF) $*.cln $@
        del $*.cln

msxml.rc : $(SDK_LIB_PATH)\msxml.tlb messages.rc selfreg_msxml.inf mimedefault.xsl
