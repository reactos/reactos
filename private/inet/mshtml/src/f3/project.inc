# project.inc - project specific settings

# Version resource stuff
!if "$(_RELEASE)" != "2"

# USERNAME and COMPUTERNAME aren't defined in Win95
!if "$(USERNAME)" == ""
USERNAME=Unknown
!endif

!if "$(COMPUTERNAME)" == ""
COMPUTERNAME=Win95 machine
!endif

RFLAGS=$(RFLAGS) /DVER_PRIVATE_BUILD_STR="\"$(USERNAME) on \\\\$(COMPUTERNAME)\""
!endif  //$(_RELEASE)
