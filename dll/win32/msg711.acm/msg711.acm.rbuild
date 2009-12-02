<module name="msg711.acm" type="win32dll" baseaddress="${BASEADDRESS_MSG711ACM}" installbase="system32" installname="msg711.acm" allowwarnings="true" entrypoint="0">
	<importlibrary definition="msg711.acm.spec" />
	<include base="msg711.acm">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msg711.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
