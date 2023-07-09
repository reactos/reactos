
!ifndef HTML_TARGET
HTML_TARGET     = privatebeta
!endif

HTMPARSE_CMD    = ..\parse\$(O)\parsehtm
HTMPARSE_FLAGS  = /D $(HTML_TARGET)

ECHO_PARSE_MSG  =$(HTMPARSE_CMD) $(HTMPARSE_FLAGS) $< $(O)\$(@F)

!ifndef SRC
SRC = .
!endif

.SUFFIXES: .htt .htx .css .gif .inf .thm .h

!ifdef BINPLACE_ORIGINAL_SRC
{$(SRC)\}.htt{$(O)}.htt:
    copy $< $@

{$(SRC)\}.css{$(O)}.css:
    copy $< $@

{$(SRC)\}.htx{$(O)}.htx:
    copy $< $@

{$(SRC)\bitmaps\}.gif{$(O)}.gif:
    copy $< $@

{$(SRC)\}.thm{$(O)}.thm:
    copy $< $@

{$(SRC)\}.inf{$(O)}.inf:
    copy $< $@

{$(SRC)\}.h{$(O)}.h:
    copy $< $@

!else

{$(SRC)\}.htt{$(O)}.htt:
    @type <<$(ECHO_RSP)
$(ECHO_PARSE_MSG)
<<NOKEEP
    @cd $(SRC)
    @$(HTMPARSE_CMD) $(HTMPARSE_FLAGS) $(MAKEDIR)\$(SRC)\$(<F) $(MAKEDIR)\$(O)\$(@F)
    @cd $(MAKEDIR)

{$(SRC)\}.css{$(O)}.css:
    @type <<$(ECHO_RSP)
$(ECHO_PARSE_MSG)
<<NOKEEP
    @cd $(SRC)
    @$(HTMPARSE_CMD) $(HTMPARSE_FLAGS) $(MAKEDIR)\$(SRC)\$(<F) $(MAKEDIR)\$(O)\$(@F)
    @cd $(MAKEDIR)

{$(SRC)\}.htx{$(O)}.htx:
    @type <<$(ECHO_RSP)
$(ECHO_PARSE_MSG)
<<NOKEEP
    @cd $(SRC)
    @$(HTMPARSE_CMD) $(HTMPARSE_FLAGS) $(MAKEDIR)\$(SRC)\$(<F) $(MAKEDIR)\$(O)\$(@F)
    @cd $(MAKEDIR)

{$(SRC)\}.htx{$(O)}.htm:
    @type <<$(ECHO_RSP)
$(ECHO_PARSE_MSG)
<<NOKEEP
    @cd $(SRC)
    @$(HTMPARSE_CMD) $(HTMPARSE_FLAGS) $(MAKEDIR)\$(SRC)\$(<F) $(MAKEDIR)\$(O)\$(@F)
    @cd $(MAKEDIR)

{$(SRC)\bitmaps\}.gif{$(O)}.gif:
    copy $(MAKEDIR)\$(SRC)\bitmaps\$(<F) $(MAKEDIR)\$(O)\$(@F)

{$(SRC)\}.thm{$(O)}.thm:
    copy $(MAKEDIR)\$(SRC)\$(<F) $(MAKEDIR)\$(O)\$(@F)

{$(SRC)\}.inf{$(O)}.inf:
    copy $(MAKEDIR)\$(SRC)\$(<F) $(MAKEDIR)\$(O)\$(@F)

!endif
