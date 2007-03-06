<module name="global_mem" type="win32gui" installbase="bin" installname="global_mem.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>global_mem.c</file>
</module>
