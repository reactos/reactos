<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="vmwinst" type="win32gui" installbase="system32" installname="vmwinst.exe">
	<include base="vmwinst">.</include>
	<define name="UNICODE" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>newdev</library>
	<library>user32</library>
	<library>setupapi</library>
	<library>shell32</library>
	<library>ntdll</library>
	<file>vmwinst.c</file>
	<file>vmwinst.rc</file>
</module>
