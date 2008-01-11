<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="notepad" type="win32gui" installbase="system32" installname="notepad.exe" unicode="yes">
	<include base="notepad">.</include>
	<define name="_WIN32_IE">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comdlg32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<file>dialog.c</file>
	<file>main.c</file>
	<file>text.c</file>
	<file>settings.c</file>
	<file>rsrc.rc</file>
	<pch>notepad.h</pch>
</module>
