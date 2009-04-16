<module name="tcpsvr" type="win32cui" installbase="bin" installname="tcpsvr.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>tcpsvr.c</file>
</module>
