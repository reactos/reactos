<module name="wintab32" type="win32dll" installbase="system32" installname="wintab32.dll" allowwarnings="true">
	<importlibrary definition="wintab32.spec" />
	<include base="wintab32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>context.c</file>
	<file>manager.c</file>
	<file>wintab32.c</file>
	<library>wine</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
