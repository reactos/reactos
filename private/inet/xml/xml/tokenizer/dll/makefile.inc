/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)

xmlpsr.rc : messages.rc selfreg.inf

selfreg.inf : selfreg.inx
        $(CLEANINF) $** $@
