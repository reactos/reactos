
!ifndef HTML_TARGET
HTML_TARGET     = privatebeta
!endif

HTMPARSE_CMD    = ..\parse\$(O)\parsehtm
HTMPARSE_FLAGS  = /D $(HTML_TARGET)

ECHO_PARSE_MSG  =$(HTMPARSE_CMD) $(HTMPARSE_FLAGS) $< $(O)\$(@F)

!ifndef SRC
SRC = .
!endif

.SUFFIXES: .scf

{$(SRC)\}.scf{$(O)}.scf:
    copy $< $@

