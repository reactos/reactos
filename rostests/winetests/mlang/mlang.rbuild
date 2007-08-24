<module name="mlang_winetest" type="win32cui" installbase="bin" installname="mlang_winetest.exe" allowwarnings="true">
	<include base="mlang_winetest">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ole32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>mlang.c</file>
	<file>testlist.c</file>
</module>
