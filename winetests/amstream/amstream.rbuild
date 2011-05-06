<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="amstream_winetest" type="win32cui" installbase="bin" installname="amstream_winetest.exe" allowwarnings="true">
	<include base="amstream_winetest">.</include>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>ddraw</library>
	<file>amstream.c</file>
	<file>testlist.c</file>
</module>
</group>
