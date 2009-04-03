<module name="mcicda" type="win32dll" baseaddress="${BASEADDRESS_MCICDA}" installbase="system32" installname="mcicda.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="mcicda.spec" />
	<include base="mcicda">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>mcicda.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
