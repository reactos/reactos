<module name="authz" type="win32dll" baseaddress="${BASEADDRESS_AUTHZ}" installbase="system32" installname="authz.dll">
	<importlibrary definition="authz.def" />
	<include base="authz">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>authz.c</file>
	<file>clictx.c</file>
	<file>resman.c</file>
	<file>authz.rc</file>
	<pch>precomp.h</pch>
</module>
