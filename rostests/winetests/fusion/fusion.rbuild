<module name="fusion_winetest" type="win32cui" installbase="bin" installname="fusion_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="fusion_winetest">.</include>
	<file>asmcache.c</file>
	<file>asmenum.c</file>
	<file>asmname.c</file>
	<file>fusion.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
