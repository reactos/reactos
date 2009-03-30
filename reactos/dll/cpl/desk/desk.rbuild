<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="desk" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_DESK}" installbase="system32" installname="desk.cpl" unicode="yes" crt="msvcrt">
	<importlibrary definition="desk.spec" />
	<include base="desk">.</include>
	<define name="_WIN32" />
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>ole32</library>
	<library>setupapi</library>
	<library>shell32</library>
	<library>ntdll</library>
	<library>msimg32</library>
	<library>uuid</library>
	<file>advmon.c</file>
	<file>appearance.c</file>
	<file>background.c</file>
	<file>classinst.c</file>
	<file>desk.c</file>
	<file>devsett.c</file>
	<file>dibitmap.c</file>
	<file>misc.c</file>
	<file>preview.c</file>
	<file>screensaver.c</file>
	<file>advappdlg.c</file>
	<file>effappdlg.c</file>
	<file>settings.c</file>
	<file>monslctl.c</file>
	<file>general.c</file>
	<file>desk.rc</file>
</module>
