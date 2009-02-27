<module name="pdh_winetest" type="win32cui" installbase="bin" installname="pdh_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="pdh_winetest">.</include>
	<file>pdh.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>pdh</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
