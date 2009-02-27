<module name="localui_winetest" type="win32cui" installbase="bin" installname="localui_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="localui_winetest">.</include>
	<file>localui.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>winspool</library>
	<library>ntdll</library>
</module>
