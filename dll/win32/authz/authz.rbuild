<module name="authz" type="win32dll" baseaddress="${BASEADDRESS_AUTHZ}" installbase="system32" installname="authz.dll" unicode="yes">
	<importlibrary definition="authz.spec" />
	<include base="authz">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>authz.c</file>
	<file>clictx.c</file>
	<file>resman.c</file>
	<file>authz.rc</file>
	<pch>precomp.h</pch>
</module>
