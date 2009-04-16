<module name="gethostbyname" type="win32gui" installbase="bin" installname="gethostbyname.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>gethostbyname.c</file>
</module>
