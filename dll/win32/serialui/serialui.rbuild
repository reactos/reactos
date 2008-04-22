<module name="serialui" type="win32dll" baseaddress="${BASEADDRESS_SERIALUI}" installbase="system32" installname="serialui.dll">
	<importlibrary definition="serialui.def" />
	<include base="serialui">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>kernel32</library>
	<library>user32</library>
	<library>shlwapi</library>
	<file>serialui.c</file>
	<file>serialui.rc</file>
</module>
