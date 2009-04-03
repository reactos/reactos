<module name="console" type="win32dll" baseaddress="${BASEADDRESS_CONSOLE}" installbase="system32" installname="console.dll" unicode="yes">
	<importlibrary definition="console.spec" />
	<include base="console">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>gdi32</library>
	<file>console.c</file>
	<file>options.c</file>
	<file>font.c</file>
	<file>layout.c</file>
	<file>colors.c</file>
	<file>console.rc</file>
	<pch>console.h</pch>
</module>
