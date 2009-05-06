<module name="xinput1_3" type="win32dll" installbase="system32" installname="xinput1_3.dll">
	<importlibrary definition="xinput1_3.spec" />
	<include base="xinput1_3">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>kernel32</library>
	<library>wine</library>
	<file>xinput1_3_main.c</file>
	<file>version.rc</file>
</module>
