!if exist(..\$(TARGET_DIRECTORY).inc)
!include ..\$(TARGET_DIRECTORY).inc
!endif

..\res.rc: ..\dialogs.dlg ..\strings.rc ..\win31mig.dlg wlevents.rc

precomp.h: wlevents.h
