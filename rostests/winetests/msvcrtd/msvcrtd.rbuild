<module name="msvcrtd_winetest" type="win32cui" installbase="bin" installname="msvcrtd_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="msvcrtd_winetest">.</include>
	<file>debug.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
