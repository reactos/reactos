<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="umpnpmgr" type="win32cui" installbase="system32" installname="umpnpmgr.exe">
	<include base="umpnpmgr">.</include>
	<include base="pnp_server">.</include>
	<include base="pnp_client">.</include>
	<define name="__USE_W32API" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>pnp_server</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<library>wdmguid</library>
	<family>services</family>
	<file>umpnpmgr.c</file>
	<file>umpnpmgr.rc</file>
</module>
