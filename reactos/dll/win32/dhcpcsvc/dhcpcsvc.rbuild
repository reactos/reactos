<module name="dhcpcsvc" type="win32dll" baseaddress="${BASEADDRESS_DHCPCSVC}" installbase="system32" installname="dhcpcsvc.dll">
	<importlibrary definition="dhcpcsvc.spec" />
	<include base="dhcpcsvc">include</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>iphlpapi</library>
	<file>dhcpcsvc.c</file>
	<file>dhcpcsvc.rc</file>
	<file>dhcpcsvc.spec</file>
</module>
