<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="odbc32" type="win32dll" baseaddress="${BASEADDRESS_ODBC32}" installbase="system32" installname="odbc32.dll" allowwarnings="true" >
	<importlibrary definition="odbc32.spec" />
	<include base="odbc32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>kernel32</library>
	<file>proxyodbc.c</file>
</module>
</group>
