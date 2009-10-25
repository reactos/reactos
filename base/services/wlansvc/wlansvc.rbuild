<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wlansvc" type="win32cui" installbase="system32" installname="wlansvc.exe" unicode="yes">
	<include base="wlansvc">.</include>
	<include base="wlansvc_server">.</include>
	<library>wlansvc_server</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<file>wlansvc.c</file>
	<file>rpcserver.c</file>
</module>
