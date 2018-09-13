!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)

msxql.rc : $(SDK_LIB_PATH)\msxml.tlb messages.rc selfreg.inf

selfreg.inf : selfreg.inx
        $(CLEANINF) $** $@
