<module name="sysicon" type="win32gui" installbase="system32" installname="sysicon.exe">
	<include base="capicon">.</include>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>sysicon.c</file>
</module>
