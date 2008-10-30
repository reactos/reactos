<module name="netcfgx" type="win32dll" baseaddress="${BASEADDRESS_NETCFGX}" installbase="system32" installname="netcfgx.dll">
	<importlibrary definition="netcfgx.spec" />
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<library>ntdll</library>
	<library>rpcrt4</library>
	<library>setupapi</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>uuid</library>
	<library>iphlpapi</library>
	<library>iphlpapi</library>
	<library>kernel32</library>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>ws2_32</library>
	<file>netcfgx.c</file>
	<file>netcfgx.spec</file>
	<file>classfactory.c</file>
	<file>netcfg_iface.c</file>
	<file>inetcfgcomp_iface.c</file>
	<file>tcpipconf_notify.c</file>
	<file>netcfgx.rc</file>
</module>
