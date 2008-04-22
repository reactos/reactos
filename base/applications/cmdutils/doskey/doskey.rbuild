<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="doskey" type="win32cui" installbase="system32" installname="doskey.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>kernel32</library>
	<file>doskey.c</file>
	<file>doskey.rc</file>
</module>
