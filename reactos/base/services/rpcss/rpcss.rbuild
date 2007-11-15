<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rpcss" type="win32cui" installbase="system32" installname="rpcss.exe">
	<include base="rpcss">.</include>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>advapi32</library>
	<family>services</family>
	<file>rpcss.c</file>
	<file>endpoint.c</file>
	<file>rpcss.rc</file>
</module>
