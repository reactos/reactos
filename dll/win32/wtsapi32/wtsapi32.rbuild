<module name="wtsapi32" type="win32dll" baseaddress="${BASEADDRESS_WTSAPI32}" installbase="system32" installname="wtsapi32.dll" allowwarnings="true">
	<importlibrary definition="wtsapi32.spec.def" />
	<include base="wtsapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>wtsapi32.c</file>
	<file>wtsapi32.spec</file>
</module>
