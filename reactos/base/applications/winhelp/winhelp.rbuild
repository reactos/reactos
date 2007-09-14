<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="winhelp" type="win32gui" installbase="system32" installname="winhelp.exe" unicode="no">
	<include base="winhelp">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ntdll</library>
	<library>comdlg32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<file>callback.c</file>
	<file>hlpfile.c</file>
	<file>macro.c</file>
	<file>string.c</file>
	<file>winhelp.c</file>
	<file>lex.yy.c</file>
	<file>rsrc.rc</file>
</module>
