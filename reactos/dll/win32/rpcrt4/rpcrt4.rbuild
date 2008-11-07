<module name="rpcrt4" type="win32dll" baseaddress="${BASEADDRESS_RPCRT4}" installbase="system32" installname="rpcrt4.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="rpcrt4.spec" />
	<include base="rpcrt4">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_STDDEF_H" />
	<define name="_RPCRT4_" />
	<define name="COM_NO_WINDOWS_H" />
	<define name="MSWMSG" />
	<library>wine</library>
	<library>uuid</library>
	<library>rpcrt4_epm_client</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>secur32</library>
	<library>iphlpapi</library>
	<library>ws2_32</library>
	<library>ntdll</library>
	<file>cproxy.c</file>
	<file>cpsf.c</file>
	<file>cstub.c</file>
	<file>ndr_contexthandle.c</file>
	<file>ndr_clientserver.c</file>
	<file>ndr_fullpointer.c</file>
	<file>ndr_marshall.c</file>
	<file>ndr_ole.c</file>
	<file>ndr_stubless.c</file>
	<file>rpc_assoc.c</file>
	<file>rpc_async.c</file>
	<file>rpc_binding.c</file>
	<file>rpc_epmap.c</file>
	<file>rpc_message.c</file>
	<file>rpc_server.c</file>
	<file>rpc_transport.c</file>
	<file>rpcrt4_main.c</file>
	<file>unix_func.c</file>
	<file>ndr_es.c</file>
	<file>rpcrt4.rc</file>
	<file>epm.idl</file>
	<include base="rpcrt4" root="intermediate">.</include>
	<file>rpcrt4.spec</file>
</module>
<module name="rpcrt4_epm_client" type="rpcclient">
	<file>epm.idl</file>
</module>
