<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="winlogon" type="win32gui" installbase="system32" installname="winlogon.exe">
	<include base="winlogon">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>userenv</library>
	<library>secur32</library>
	<file>sas.c</file>
	<file>screensaver.c</file>
	<file>setup.c</file>
	<file>winlogon.c</file>
	<file>wlx.c</file>
	<file>winlogon.rc</file>
	<pch>winlogon.h</pch>
</module>
