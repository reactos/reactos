<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="ws2_32_apitest" type="win32cui" installbase="bin" installname="ws2_32_apitest.exe">
	<include base="ws2_32_apitest">.</include>
	<library>wine</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>pseh</library>
	<library>ws2_32</library>
	<file>testlist.c</file>
	<file>helpers.c</file>

	<file>ioctlsocket.c</file>
	<file>recv.c</file>

</module>
</group>
