<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="eventlog" type="win32cui" installbase="system32" installname="eventlog.exe">
	<include base="eventlog">.</include>
	<include base="eventlog_server">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>eventlog_server</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<file>eventlog.c</file>
	<file>logport.c</file>
	<file>eventlog.rc</file>
	<file>rpc.c</file>
	<file>file.c</file>
	<pch>eventlog.h</pch>
</module>
