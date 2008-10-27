<module name="netshell" type="win32dll" baseaddress="${BASEADDRESS_NETSHELL}" installbase="system32" installname="netshell.dll">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="netshell.spec" />
	<include base="netshell">.</include>
	<define name="_NETSHELL_" />
	<library>shlwapi</library>
	<library>shell32</library>
	<library>version</library>
	<library>iphlpapi</library>
	<library>kernel32</library>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>uuid</library>
	<library>advapi32</library>
	<library>setupapi</library>
	<library>ws2_32</library>
	<library>comctl32</library>
	<pch>precomp.h</pch>
	<file>netshell.c</file>
	<file>shfldr_netconnect.c</file>
	<file>enumlist.c</file>
	<file>netshell.rc</file>
	<file>classfactory.c</file>
	<file>connectmanager.c</file>
	<file>netshell.spec</file>
	<file>lanconnectui.c</file>
	<file>lanstatusui.c</file>
</module>
