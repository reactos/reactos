<module name="enumfonts" type="win32gui" installbase="bin" installname="enumfonts.exe" stdlib="host">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>enumfonts.cpp</file>
</module>
