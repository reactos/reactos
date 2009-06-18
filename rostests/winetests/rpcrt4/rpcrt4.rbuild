<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="rpcrt4_winetest_server" type="rpcserver" allowwarnings="true">
		<file>server.idl</file>
	</module>
	<module name="rpcrt4_winetest_client" type="rpcclient">
		<file>server.idl</file>
	</module>
	<module name="rpcrt4_winetest" type="win32cui" installbase="bin" installname="rpcrt4_winetest.exe" allowwarnings="true">
		<include base="rpcrt4_winetest">.</include>
		<include root="intermediate" base="rpcrt4_winetest">.</include>
		<define name="__ROS_LONG64__" />
		<library>wine</library>
		<library>pseh</library>
		<library>ole32</library>
		<library>uuid</library>
		<library>rpcrt4_winetest_server</library>
		<library>rpcrt4_winetest_client</library>
		<library>rpcrt4</library>
		<library>kernel32</library>
		<library>ntdll</library>
		<file>cstub.c</file>
		<file>generated.c</file>
		<file>ndr_marshall.c</file>
		<file>rpc.c</file>
		<file>rpc_async.c</file>
		<file>server.c</file>
		<file>testlist.c</file>
	</module>
</group>
