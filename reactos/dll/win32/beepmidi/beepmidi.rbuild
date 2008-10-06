<module name="beepmidi" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_BEEPMIDI}" installbase="system32" installname="beepmidi.dll" unicode="yes">
	<importlibrary definition="beepmidi.def" />
	<include base="beepmidi">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>beepmidi.c</file>
</module>
