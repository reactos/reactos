<module name="spoolss_winetest" type="win32cui" installbase="bin" installname="spoolss_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="spoolss_winetest">.</include>
	<file>spoolss.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
