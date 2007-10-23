<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="matrix" type="win32scr" installbase="system32" installname="matrix2.scr" allowwarnings="true">
	<include base="matrix">.</include>

	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="UNICODE" />
	<define name="_UNICODE" />

	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>winspool</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>uuid</library>

	<file>config.c</file>
	<file>matrix.c</file>
	<file>message.c</file>
	<file>password.c</file>
	<file>screensave.c</file>
	<file>settings.c</file>
	<file>rsrc.rc</file>
</module>
