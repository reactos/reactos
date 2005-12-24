<module name="crypt32" type="win32dll" baseaddress="${BASEADDRESS_CRYPT32}" installbase="system32" installname="crypt32.dll">
	<importlibrary definition="crypt32.spec.def" />
	<include base="crypt32">.</include>
	<include base="ReactOS">include/wine</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>wine</library>
	<library>advapi32</library>
	<file>main.c</file>
	<file>encode.c</file>
	<file>cert.c</file>
	<file>oid.c</file>
	<file>protectdata.c</file>
	<file>crypt32.rc</file>
	<file>crypt32.spec</file>
</module>
