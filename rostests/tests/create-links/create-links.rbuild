<module name="create-links" type="win32cui" installbase="bin" installname="create-links.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<file>create-links.c</file>
</module>
