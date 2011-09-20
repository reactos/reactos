<module name="lsasrv" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_LSASRV}" installbase="system32" installname="lsasrv.dll" unicode="yes">
	<importlibrary definition="lsasrv.spec" />
	<include base="lsasrv">.</include>
	<include base="lsa_server">.</include>
	<include base="ReactOS">include/reactos/subsys/lsass</include>
	<library>lsa_server</library>
	<library>wine</library>
	<library>rpcrt4</library>
	<library>ntdll</library>
	<library>pseh</library>
	<file>authport.c</file>
	<file>database.c</file>
	<file>lsarpc.c</file>
	<file>lsasrv.c</file>
	<file>policy.c</file>
	<file>privileges.c</file>
	<file>sids.c</file>
	<file>lsasrv.rc</file>
	<pch>lsasrv.h</pch>
</module>
