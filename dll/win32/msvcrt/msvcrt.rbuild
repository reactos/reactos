<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="msvcrt" type="win32dll" baseaddress="${BASEADDRESS_MSVCRT}" mangledsymbols="true" installbase="system32" installname="msvcrt.dll">
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>--enable-stdcall-fixup</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<linkerflag>-lgcc</linkerflag>
	<importlibrary definition="msvcrt.def" />
	<include base="msvcrt">.</include>
	<include base="crt">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="__REACTOS__" />
	<define name="USE_MSVCRT_PREFIX" />
	<define name="_MSVCRT_LIB_" />
	<define name="_MT" />
	<library>crt</library>
	<library>string</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>wine</library>
	<pch>precomp.h</pch>
	<file>dllmain.c</file>
	<file>msvcrt.rc</file>
</module>
</rbuild>
