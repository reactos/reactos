<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="freeldr_fdebug" type="win32gui" installbase="system32" installname="fdebug.exe">
	<include base="freeldr_fdebug">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comdlg32</library>
	<library>gdi32</library>	
	<file>fdebug.c</file>
	<file>rs232.c</file>
	<file>fdebug.rc</file>
</module>
