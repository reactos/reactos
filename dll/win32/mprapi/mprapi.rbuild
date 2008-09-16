<module name="mprapi" type="win32dll" baseaddress="${BASEADDRESS_MPRAPI}" installbase="system32" installname="mprapi.dll">
	<importlibrary definition="mprapi.spec.def" />
	<include base="mprapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x601</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>kernel32</library>
	<file>mprapi.c</file>
	<file>mprapi.spec</file>
</module>
