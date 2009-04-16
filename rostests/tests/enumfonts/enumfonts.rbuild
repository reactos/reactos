<module name="enumfonts" type="win32gui" installbase="bin" installname="enumfonts.exe" stdlib="host">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<compilerflag compiler="cpp">-Wno-non-virtual-dtor</compilerflag>
	<file>enumfonts.cpp</file>
</module>
