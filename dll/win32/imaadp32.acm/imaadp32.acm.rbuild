<module name="imaadp32.acm" type="win32dll" baseaddress="${BASEADDRESS_IMAADP32ACM}" installbase="system32" installname="imaadp32.acm" allowwarnings="true" entrypoint="0">
	<importlibrary definition="imaadp32.acm.spec" />
	<include base="imaadp32.acm">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>imaadp32.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
