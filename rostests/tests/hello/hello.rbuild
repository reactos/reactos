<module name="hello" type="win32cui" installbase="bin" installname="hello.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>hello.c</file>
</module>
