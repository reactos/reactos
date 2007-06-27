<module name="version" type="win32dll" baseaddress="${BASEADDRESS_VERSION}" installbase="system32" installname="version.dll" allowwarnings="true">
	<importlibrary definition="version.def" />
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<directory name="misc">
		<file>libmain.c</file>
		<file>stubs.c</file>
	</directory>
	<file>info.c</file>
	<file>install.c</file>
	<file>resource.c</file>
	<file>version.rc</file>
</module>
