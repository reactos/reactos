<module name="wavemap" type="win32dll" entrypoint="0" extension=".drv" baseaddress="${BASEADDRESS_WAVEMAP}" installbase="system32" installname="msacm32.drv" allowwarnings="true">
	<importlibrary definition="msacm.spec.def" />
	<include base="wavemap">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>msacm32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>wavemap.c</file>
	<file>wavemap.rc</file>
	<file>msacm.spec</file>
</module>
