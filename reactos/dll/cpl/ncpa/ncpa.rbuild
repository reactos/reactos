<module name="ncpa" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_NCPA}" installbase="system32" installname="ncpa.cpl">
	<importlibrary definition="ncpa.def" />
	<include base="ncpa">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>iphlpapi</library>
	<library>ws2_32</library>
	<library>dhcpcsvc</library>
	<library>ntdll</library>
	<library>msvcrt</library>
	<file>ncpa.c</file>
	<file>tcpip_properties.c</file>
	<file>ncpa.rc</file>
</module>
