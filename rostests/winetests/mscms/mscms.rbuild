<module name="mscms_winetest" type="win32cui" installbase="bin" installname="mscms_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="mscms_winetest">.</include>
	<file>profile.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
