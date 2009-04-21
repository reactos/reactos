<module name="uext2" type="win32dll" baseaddress="${BASEADDRESS_UEXT2}" installbase="system32" installname="uext2.dll">
	<importlibrary definition="uext2.def" />
	<include base="uext2">.</include>
	<library>ext2lib</library>
	<library>ntdll</library>
	<file>uext2.c</file>
	<file>uext2.rc</file>
</module>
