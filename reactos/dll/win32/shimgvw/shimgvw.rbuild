<module name="shimgvw" type="win32dll" baseaddress="${BASEADDRESS_SHIMGVW}" installbase="system32" installname="shimgvw.dll">
	<importlibrary definition="shimgvw.spec" />
	<include base="shimgvw">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>kernel32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>gdiplus</library>
	<file>shimgvw.c</file>
	<file>shimgvw.rc</file>
	<file>shimgvw.spec</file>
</module>
