<module name="lantest" type="win32gui" installbase="bin" installname="lantest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>lantest.cpp</file>
</module>
