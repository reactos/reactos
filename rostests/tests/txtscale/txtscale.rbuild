<module name="txtscale" type="win32gui" installbase="bin" installname="txtscale.exe" allowwarnings ="true" stdlib="host">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<compilerflag compiler="cpp">-Wno-non-virtual-dtor</compilerflag>
	<file>txtscale.cpp</file>
	<file>mk_font.cpp</file>
</module>
