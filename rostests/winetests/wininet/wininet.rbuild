<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="wininet_winetest" type="win32cui" installbase="bin" installname="wininet_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="wininet_winetest">.</include>
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
	<file>urlcache.c</file>
	<file>testlist.c</file>
</module>
</group>
