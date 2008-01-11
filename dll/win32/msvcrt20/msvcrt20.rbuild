<module name="msvcrt20" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_MSVCRT20}" mangledsymbols="yes" installbase="system32" installname="msvcrt20.dll">
	<importlibrary definition="msvcrt20.def" />
	<include base="msvcrt20">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__REACTOS__" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="USE_MSVCRT_PREFIX" />
	<define name="_MT" />
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>msvcrt</library>
	<file>msvcrt20.c</file>
	<file>msvcrt20.rc</file>
</module>
