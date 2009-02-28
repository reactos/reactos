<module name="crtdll" type="win32dll" baseaddress="${BASEADDRESS_CRTDLL}" mangledsymbols="true" installbase="system32" installname="crtdll.dll" iscrt="true">
	<importlibrary definition="crtdll.def" />
	<include base="crtdll">.</include>
	<include base="crt">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="USE_MSVCRT_PREFIX" />
	<define name="_MSVCRT_LIB_" />
	<define name="_MSVCRT_" />
	<define name="__NO_CTYPE_INLINES" />
	<define name="_CTYPE_DISABLE_MACROS" />
	<define name="_NO_INLINING" />

	<!--	__MINGW_IMPORT needs to be defined differently because it's defined
		as dllimport by default, which is invalid from GCC 4.1.0 on!	-->
	<define name="__MINGW_IMPORT">"extern __attribute__ ((dllexport))"</define>

	<library>crt</library>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<pch>precomp.h</pch>
	<file>dllmain.c</file>
	<file>crtdll.rc</file>
</module>
