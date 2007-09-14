<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="welcome" type="win32gui" installbase="system32" installname="welcome.exe">
	<bootstrap base="$(CDOUTPUT)" />
	<include base="welcome">.</include>
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<file>welcome.c</file>
	<file>welcome.rc</file>
</module>
