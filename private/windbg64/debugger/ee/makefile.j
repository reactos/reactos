!message NOTE: Must be running on NT-J for correct compilation of Japanese resources !

!IFNDEF DEBUG
DEBUG=1
!ENDIF

!IFNDEF JDATE
JDATE=1
!ENDIF

!IFNDEF CODEVIEW
CODEVIEW=1
!ENDIF

!IFNDEF TRANSLATE
TRANSLATE=1
!ENDIF

!IF "$(TRANSLATE)" == "1"
# if build under NT-US, use 	"rt -h -cCOMMAND ...."
# if build under NT-J, use 		"rt -cCOMMAND ...."
J_RC=$(LOCALIZEDIR)\rt $(LOCALIZEDIR)\japan.lng debmsg.cr debmsg.rc
!ELSE
J_RC=copy debmsgj.rc debmsg.rc
!ENDIF

all:
	attrib -r debmsg.rc
	copy debmsg.rc debmsg.cr
	$(J_RC)
	copy debmsg.rc rc.out

	nmake DEBUG=$(DEBUG) CODEVIEW=$(CODEVIEW)
	copy debmsg.cr debmsg.rc
