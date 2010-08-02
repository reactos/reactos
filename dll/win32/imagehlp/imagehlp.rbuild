<module name="imagehlp" type="win32dll" baseaddress="${BASEADDRESS_IMAGEHLP}" installbase="system32" allowwarnings="true" installname="imagehlp.dll" crt="msvcrt">
	<importlibrary definition="imagehlp.spec" />
	<include base="imagehlp">.</include>
	<define name="_IMAGEHLP_SOURCE_"></define>
	<library>wine</library>
	<library>dbghelp</library>
	<library>ntdll</library>
	<file>access.c</file>
	<file>imagehlp_main.c</file>
	<file>integrity.c</file>
	<file>modify.c</file>
	<file>imagehlp.rc</file>
	<pch>precomp.h</pch>
</module>
