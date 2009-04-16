<module name="nameserverlist" type="win32cui" installbase="bin" installname="nameserverlist.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>iphlpapi</library>
	<file>nameserverlist.c</file>
</module>
