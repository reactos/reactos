<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wininet_winetest" type="win32cui" installbase="bin" installname="wininet_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="wininet_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>wininet</library>
	<library>ws2_32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>ftp.c</file>
	<file>generated.c</file>
	<file>http.c</file>
	<file>internet.c</file>
	<file>url.c</file>
	<file>testlist.c</file>
</module>
