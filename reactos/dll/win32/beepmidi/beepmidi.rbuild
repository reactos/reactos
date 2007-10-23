<module name="beepmidi" type="win32dll" installbase="system32" installname="beepmidi.dll">
	<importlibrary definition="beepmidi.def" />
	<include base="beepmidi">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>beepmidi.c</file>
</module>
