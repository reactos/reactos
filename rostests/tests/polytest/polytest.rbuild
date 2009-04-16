<module name="polytest" type="win32gui" installbase="bin" installname="polytest.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>polytest.cpp</file>
</module>
