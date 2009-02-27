<module name="serialui_winetest" type="win32cui" installbase="bin" installname="serialui_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="serialui_winetest">.</include>
	<file>confdlg.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
