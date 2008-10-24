<module name="serialui" type="win32dll" baseaddress="${BASEADDRESS_SERIALUI}" installbase="system32" installname="serialui.dll" unicode="yes">
	<importlibrary definition="serialui.spec" />
	<include base="serialui">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>shlwapi</library>
	<file>serialui.c</file>
	<file>serialui.rc</file>
	<file>serialui.spec</file>
</module>
