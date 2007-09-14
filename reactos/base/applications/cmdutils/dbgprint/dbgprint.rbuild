<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="dbgprint" type="win32cui" installbase="system32" installname="dbgprint.exe">
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>dbgprint.c</file>
</module>
