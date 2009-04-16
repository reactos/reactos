<module name="carets" type="win32gui" installbase="bin" installname="carets.exe">
	<define name="__USE_W32API" />
	<include base="carets">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ntdll</library>
	<file>carets.c</file>
	<file>carets.rc</file>
</module>
