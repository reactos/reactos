<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="shlwapi_winetest" type="win32cui" installbase="bin" installname="shlwapi_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="shlwapi_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>shlwapi</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>clist.c</file>
	<file>clsid.c</file>
	<file>generated.c</file>
	<file>ordinal.c</file>
	<file>path.c</file>
	<file>shreg.c</file>
	<file>string.c</file>
	<file>url.c</file>
	<file>testlist.c</file>
</module>
