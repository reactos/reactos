<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rasadhlp" type="win32dll" installbase="system32" installname="rasadhlp.dll">
	<importlibrary definition="rasadhlp.spec" />
	<include base="ReactOS">include/reactos/winsock</include>
	<include base="rasadhlp">.</include>
	<library>ntdll</library>
	<library>ws2_32</library>
	<file>autodial.c</file>
	<file>init.c</file>
	<file>winsock.c</file>
	<pch>precomp.h</pch>
</module>
