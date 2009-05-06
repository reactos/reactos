<module name="xinput9_1_0" type="win32dll" installbase="system32" installname="xinput9_1_0.dll">
	<importlibrary definition="xinput9_1_0.spec" />
	<include base="xinput9_1_0">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>kernel32</library>
	<file>xinput9_1_0_main.c</file>
	<file>version.rc</file>
</module>
