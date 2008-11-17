<module name="stretchblt" type="win32gui" installbase="bin" installname="stretchblt.exe" stdlib="host">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>stretchblt.cpp</file>
</module>
