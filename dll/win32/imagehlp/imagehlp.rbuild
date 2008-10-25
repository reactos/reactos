<module name="imagehlp" type="win32dll" baseaddress="${BASEADDRESS_IMAGEHLP}" installbase="system32" installname="imagehlp.dll" allowwarnings="true">
	<importlibrary definition="imagehlp.spec" />
	<include base="imagehlp">.</include>
	<define name="_IMAGEHLP_SOURCE_"></define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>access.c</file>
	<file>imagehlp_main.c</file>
	<file>integrity.c</file>
	<file>modify.c</file>
	<file>imagehlp.rc</file>
	<file>imagehlp.spec</file>
	<pch>precomp.h</pch>
</module>
