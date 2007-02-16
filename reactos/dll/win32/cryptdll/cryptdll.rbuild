<module name="cryptdll" type="win32dll" baseaddress="${BASEADDRESS_CRYPTDLL}" installbase="system32" installname="cryptdll.dll" allowwarnings="true">
	<importlibrary definition="cryptdll.spec.def" />
	<include base="cryptdll">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>cryptdll.c</file>
	<file>cryptdll.spec</file>
</module>
