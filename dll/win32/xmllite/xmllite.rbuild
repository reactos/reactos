<module name="xmllite" type="win32dll" installbase="system32" installname="xmllite.dll" allowwarnings="true">
	<importlibrary definition="xmllite.spec" />
	<include base="xmllite">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<file>reader.c</file>
	<file>xmllite_main.c</file>
</module>
