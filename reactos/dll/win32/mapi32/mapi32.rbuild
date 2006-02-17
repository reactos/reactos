<module name="mapi32" type="win32dll" baseaddress="${BASEADDRESS_MAPI32}" installbase="system32" installname="mapi32.dll">
	<importlibrary definition="mapi32.spec.def" />
	<include base="mapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x501</define>
	<define name="__WINESRC__" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>shlwapi</library>
	<library>wine</library>
	<library>uuid</library>
	<library>advapi32</library>
	<file>mapi32_main.c</file>
	<file>imalloc.c</file>
	<file>prop.c</file>
	<file>util.c</file>
	<file>mapi32.spec</file>
</module>
