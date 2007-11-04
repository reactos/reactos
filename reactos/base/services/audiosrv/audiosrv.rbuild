<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="audiosrv" type="win32cui" installbase="system32" installname="audiosrv.exe" unicode="yes" allowwarnings="true">
	<include base="audiosrv">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="WINVER">0x0501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>setupapi</library>
	<library>ksguid</library>
	<file>main.c</file>
	<file>pnp_list_manager.c</file>
	<file>pnp_list_lock.c</file>
	<file>pnp.c</file>
	<file>debug.c</file>
	<file>audiosrv.rc</file>
</module>
