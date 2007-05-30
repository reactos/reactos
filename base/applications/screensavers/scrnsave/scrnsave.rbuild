<module name="scrnsave" type="win32scr" installbase="system32" installname="scrnsave.scr">
	<define name="__USE_W32API" />
	<define name="__REACTOS__" />
	<define name="UNICODE" />
	<define name="_UNICODE" />

	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>opengl32</library>
	<library>glu32</library>
	<library>winmm</library>

	<file>scrnsave.c</file>
	<file>scrnsave.rc</file>
</module>
