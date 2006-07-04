<module name="msimg32" type="win32dll" baseaddress="${BASEADDRESS_MSIMG32}" installbase="system32" installname="msimg32.dll" allowwarnings="true">
	<importlibrary definition="msimg32.spec.def" />
	<include base="msimg32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>msimg32_main.c</file>
	<file>msimg32.spec</file>
</module>
