<module name="enhmetafile" type="win32gui" installbase="bin" installname="enhmetafile.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>enhmetafile.c</file>
</module>
