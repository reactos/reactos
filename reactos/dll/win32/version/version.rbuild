<module name="version" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_VERSION}" installbase="system32" installname="version.dll" allowwarnings="true">
	<importlibrary definition="version.spec.def" />
	<include base="version">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>lz32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>info.c</file>
	<file>install.c</file>
	<file>resource.c</file>
	<file>version.rc</file>
	<file>version.spec</file>
</module>
