<module name="shm" type="win32gui" installbase="bin" installname="shm.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>shm.c</file>
</module>
