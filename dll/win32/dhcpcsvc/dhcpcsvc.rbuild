<module name="dhcpcsvc" type="win32dll" baseaddress="${BASEADDRESS_DHCPCSVC}" installbase="system32" installname="dhcpcsvc.dll">
	<importlibrary definition="dhcpcsvc.spec.def" />
	<include base="dhcpcsvc">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT=0x0500" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>msvcrt</library>
	<library>ws2_32</library>
	<library>iphlpapi</library>
	<file>dhcpcsvc.c</file>
	<file>dhcpcsvc.rc</file>
	<file>dhcpcsvc.spec</file>
</module>
