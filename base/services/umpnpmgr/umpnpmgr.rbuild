<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="umpnpmgr" type="win32cui" installbase="system32" installname="umpnpmgr.exe" unicode="yes">
	<include base="umpnpmgr">.</include>
	<include base="pnp_server">.</include>
	<define name="_WIN32_WINNT">0x600</define>
	<library>pnp_server</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<library>wdmguid</library>
	<file>umpnpmgr.c</file>
	<file>umpnpmgr.rc</file>
</module>
