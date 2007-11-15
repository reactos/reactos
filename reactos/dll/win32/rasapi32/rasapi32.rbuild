<module name="rasapi32" type="win32dll" baseaddress="${BASEADDRESS_RASAPI32}" installbase="system32" installname="rasapi32.dll">
	<importlibrary definition="rasapi32.spec.def" />
	<include base="rasapi32">.</include>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>wine</library>
	<library>kernel32</library>
	<library>uuid</library>
	<file>rasapi.c</file>
	<file>rasapi32.spec</file>
	<file>rasapi32.rc</file>
	<file>rsrc.rc</file>
</module>
