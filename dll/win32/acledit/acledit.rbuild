<module name="acledit" type="win32dll" baseaddress="${BASEADDRESS_ACLEDIT}" installbase="system32" installname="acledit.dll" unicode="yes">
	<importlibrary definition="acledit.spec" />
	<include base="acledit">.</include>

	<metadata description = "Access Control List Editor" />

	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>acledit.c</file>
	<file>stubs.c</file>
	<file>acledit.rc</file>
</module>
