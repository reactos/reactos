<module name="acledit" type="win32dll" baseaddress="${BASEADDRESS_ACLEDIT}" installbase="system32" installname="acledit.dll">
	<importlibrary definition="acledit.def" />
	<include base="acledit">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />

	<metadata description = "Access Control List Editor" />

	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>acledit.c</file>
	<file>stubs.c</file>
	<file>acledit.rc</file>
</module>
