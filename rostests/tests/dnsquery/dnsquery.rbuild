<module name="dnsquery" type="win32gui" installbase="bin" installname="dnsquery.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>dnsapi</library>
	<library>ws2_32</library>
	<file>dnsquery.c</file>
</module>
