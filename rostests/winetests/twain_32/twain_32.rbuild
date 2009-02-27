<module name="twain_32_winetest" type="win32cui" installbase="bin" installname="twain_32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="twain_32_winetest">.</include>
	<file>dsm.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
