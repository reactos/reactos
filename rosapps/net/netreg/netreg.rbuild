<module name="netreg" type="win32cui" installbase="system32" installname="netreg.exe"  stdlib="host">
	<include base="netreg">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>ws2_32</library> 

	<file>netreg.cpp</file>
	<file>netreg.rc</file>
</module>
