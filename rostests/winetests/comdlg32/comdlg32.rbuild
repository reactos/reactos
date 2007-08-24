<module name="comdlg32_winetest" type="win32cui" installbase="bin" installname="comdlg32_winetest.exe" allowwarnings="true">
	<include base="comdlg32_winetest">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>comdlg32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>filedlg.c</file>
	<file>printdlg.c</file>
	<file>testlist.c</file>
</module>
