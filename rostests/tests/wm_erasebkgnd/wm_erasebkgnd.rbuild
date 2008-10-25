<module name="wm_erasebkgnd" type="win32gui" installbase="bin" installname="wm_erasebkgnd.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>wm_erasebkgnd.cpp</file>
</module>
