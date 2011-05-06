<module name="secur32" type="win32dll" baseaddress="${BASEADDRESS_SECUR32}" installbase="system32" installname="secur32.dll" unicode="true">
	<importlibrary definition="secur32.spec" />
	<include base="secur32">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<define name="__SECUR32__" />
	<library>lsalib</library>
	<library>ntdll</library>
	<library>advapi32</library>
	<library>wine</library>
	<file>sspap.c</file>
	<file>sspi.c</file>
	<file>stubs.c</file>
	<file>dllmain.c</file>
	<file>secext.c</file>
	<file>secur32.c</file>
	<file>thunks.c</file>
	<file>wrapper.c</file>
	<file>secur32.rc</file>
	<pch>precomp.h</pch>
</module>