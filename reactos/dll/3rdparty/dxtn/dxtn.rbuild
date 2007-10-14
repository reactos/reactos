<module name="dxtn" type="win32dll" entrypoint="0" installbase="system32" installname="dxnt.dll" allowwarnings="true">
	<importlibrary definition="dxtn.def" />
	<include base="dxtn">.</include>
	<define name="__USE_W32API" />

	<library>msvcrt</library>

	<file>fxt1.c</file>
	<file>dxtn.c</file>
	<file>wrapper.c</file>
	<file>texstore.c</file>
</module>