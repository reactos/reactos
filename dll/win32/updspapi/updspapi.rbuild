<module name="updspapi" type="win32dll" installbase="system32" installname="updspapi.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="updspapi.spec" />
	<include base="updspapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<library>wine</library>
	<library>setupapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
