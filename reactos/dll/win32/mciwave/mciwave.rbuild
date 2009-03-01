<module name="mciwave" type="win32dll" baseaddress="${BASEADDRESS_MCIWAVE}" installbase="system32" installname="mciwave.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="mciwave.spec" />
	<include base="mciwave">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>mciwave.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
