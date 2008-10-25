<module name="thread_msg" type="win32gui" installbase="bin" installname="thread_msg.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>thread_msg.c</file>
</module>
