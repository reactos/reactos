<module name="gethostbyname" type="win32gui" installbase="bin" installname="gethostbyname.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>gethostbyname.c</file>
</module>
