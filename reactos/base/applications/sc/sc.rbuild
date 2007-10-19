<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sc" type="win32cui" installbase="system32" installname="sc.exe">
	<define name="__USE_W32API" />
	<define name="DEFINE_GUID" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>kernel32</library>
	<library>advapi32</library>
	<file>control.c</file>
	<file>create.c</file>
	<file>delete.c</file>
	<file>print.c</file>
	<file>query.c</file>
	<file>sc.c</file>
	<file>start.c</file>
	<file>usage.c</file>
	<file>sc.rc</file>
	<pch>sc.h</pch>
</module>
