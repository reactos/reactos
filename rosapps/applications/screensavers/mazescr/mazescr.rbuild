<module name="mazescr" type="win32scr" installbase="system32">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>

	<define name="UNICODE" />
	<define name="_UNICODE" />

	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>

	<file>scrnsave.c</file>
	<file>maze.c</file>
	<file>scrnsave.rc</file>
</module>
