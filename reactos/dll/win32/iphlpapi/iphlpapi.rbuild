<module name="iphlpapi" type="win32dll" baseaddress="${BASEADDRESS_IPHLPAPI}" installbase="system32" installname="iphlpapi.dll" allowwarnings="true">
	<importlibrary definition="iphlpapi.spec.def" />
	<include base="iphlpapi">include</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ws2_32</library>
	<library>dhcpcsvc</library>
	<file>dhcp_reactos.c</file>
	<file>ifenum_reactos.c</file>
	<file>ipstats_reactos.c</file>
	<file>iphlpapi_main.c</file>
	<file>media.c</file>
	<file>registry.c</file>
	<file>resinfo_reactos.c</file>
	<file>route_reactos.c</file>
	<file>iphlpapi.rc</file>
	<file>iphlpapi.spec</file>
</module>
