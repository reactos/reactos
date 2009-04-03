<module name="mciseq" type="win32dll" baseaddress="${BASEADDRESS_MCISEQ}" installbase="system32" installname="mciseq.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="mciseq.spec" />
	<include base="mciseq">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>mcimidi.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
