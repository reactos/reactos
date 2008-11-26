<module name="rasman" type="win32dll" baseaddress="${BASEADDRESS_RASMAN}" installbase="system32" installname="rasman.dll">
	<importlibrary definition="rasman.spec" />
	<include base="rasman">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>uuid</library>
	<file>rasman.c</file>
	<file>rasman.rc</file>
</module>
