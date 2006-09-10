<module name="gdb2" type="win32cui" installbase="system32" installname="gdb2.exe"  stdlib="host">
	<include base="gdb2">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<file>gdb2.cpp</file>
</module>