<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="rpcss" type="win32cui" installbase="system32" installname="rpcss.exe">
	<include base="rpcss">.</include>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>advapi32</library>
	<file>rpcss.c</file>
	<file>endpoint.c</file>
	<file>rpcss.rc</file>
</module>
</rbuild>
