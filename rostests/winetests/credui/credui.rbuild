<module name="credui_winetest" type="win32cui" installbase="bin" installname="credui_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="credui_winetest">.</include>
	<file>credui.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>credui</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
