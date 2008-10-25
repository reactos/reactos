<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="oleaut32_winetest" type="win32cui" installbase="bin" installname="oleaut32_winetest.exe" allowwarnings="true">
	<include base="oleaut32_winetest">.</include>
	<library>wine</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>shlwapi</library>
	<library>rpcrt4</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>olefont.c</file>
	<file>olepicture.c</file>
	<file>safearray.c</file>
	<file>tmarshal.c</file>
	<file>typelib.c</file>
	<file>usrmarshal.c</file>
	<file>varformat.c</file>
	<file>vartest.c</file>
	<file>vartype.c</file>
	<file>tmarshal.rc</file>
	<file>testlist.c</file>
</module>
</group>
