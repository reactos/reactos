<module name="sensapi" type="win32dll" baseaddress="${BASEADDRESS_SENSAPI}" installbase="system32" installname="sensapi.dll" allowwarnings="true">
	<importlibrary definition="sensapi.spec.def" />
	<include base="sensapi">.</include>
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
	<file>sensapi.c</file>
	<file>sensapi.spec</file>
</module>
