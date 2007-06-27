<module name="lsasrv" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_LSASRV}" installbase="system32" installname="lsasrv.dll">
	<importlibrary definition="lsasrv.def" />
	<include base="lsasrv">.</include>
	<include base="lsa_server">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<library>lsa_server</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>rpcrt4</library>
	<file>lsarpc.c</file>
	<file>lsasrv.c</file>
	<file>lsasrv.rc</file>
</module>
