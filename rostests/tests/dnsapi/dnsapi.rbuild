<module name="dnsapitst" type="win32cui" installbase="bin" installname="dnsapitst.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>dnsapi</library>
	<library>ws2_32</library>
	<file>dnsapi.c</file>
</module>
