<module name="imagehlp" type="win32dll" baseaddress="${BASEADDRESS_IMAGEHLP}" installbase="system32" installname="imagehlp.dll" allowwarnings="true">
	<importlibrary definition="imagehlp.def" />
	<include base="imagehlp">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x600</define>
	<define name="WINVER">0x0600</define>
	<define name="_IMAGEHLP_SOURCE_"></define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>access.c</file>
	<file>imagehlp_main.c</file>
	<file>integrity.c</file>
	<file>modify.c</file>
	<file>imagehlp.rc</file>
	<pch>precomp.h</pch>
	<linkerflag>-enable-stdcall-fixup</linkerflag>
</module>
