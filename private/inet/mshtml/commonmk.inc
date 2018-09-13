.SUFFIXES: .hdl .pdl

.pdl{$O}.hdl:
    $(C_PREPROCESSOR) $< | pdlparse - $(MAKEDIR)\$O\$(@B) $< $(MAKEDIR)\$O

!if 0
#
# .pdl -> .hdl
#
.pdl.hdl:
        $(CC) $(NOLOGO) @<< | $(PDLPARSE) <<
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHCXXFLAGS: =
)
/c
/E
$<
<<$(KEEPFLAGS)
- $(TYPEDIR)\$(@B) $(@B).pdl $(TYPEDIR)
<<$(KEEPFLAGS)
!endif
