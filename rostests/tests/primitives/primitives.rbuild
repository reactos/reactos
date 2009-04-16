<module name="primitives" type="win32gui" installbase="bin" installname="primitives.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<compilerflag compiler="cpp">-Wno-non-virtual-dtor</compilerflag>
	<file>primitives.cpp</file>
	<file>mk_font.cpp</file>
</module>
