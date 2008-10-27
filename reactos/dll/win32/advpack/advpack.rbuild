<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="advpack" type="win32dll" baseaddress="${BASEADDRESS_ADVPACK}" installbase="system32" installname="advpack.dll" allowwarnings="true">
	<importlibrary definition="advpack.spec" />
	<include base="advpack">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>advpack.c</file>
	<file>files.c</file>
	<file>install.c</file>
	<file>reg.c</file>
	<file>advpack.spec</file>
	<library>wine</library>
	<library>ole32</library>
	<library>setupapi</library>
	<library>version</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
