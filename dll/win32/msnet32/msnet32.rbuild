<module name="msnet32" type="win32dll" installbase="system32" installname="msnet32.dll" allowwarnings="true">
	<importlibrary definition="msnet32.spec" />
	<include base="msnet32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msnet_main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
