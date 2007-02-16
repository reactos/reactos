<module name="cabinet" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_CABINET}" installbase="system32" installname="cabinet.dll" allowwarnings="true">
	<importlibrary definition="cabinet.spec.def" />
	<include base="cabinet">.</include>
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
	<file>cabinet_main.c</file>
	<file>fci.c</file>
	<file>fdi.c</file>
	<file>cabinet.rc</file>
	<file>cabinet.spec</file>
</module>
