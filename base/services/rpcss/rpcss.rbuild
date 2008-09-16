<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rpcss" type="win32cui" installbase="system32" installname="rpcss.exe">
	<include base="rpcss">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>rpcss.c</file>
	<file>endpoint.c</file>
	<file>rpcss.rc</file>
</module>
