<module name="msadp32.acm" type="win32dll" baseaddress="${BASEADDRESS_MSADP32ACM}" installbase="system32" installname="msadp32.acm" allowwarnings="true" entrypoint="0">
	<importlibrary definition="msadp32.acm.spec" />
	<include base="msadp32.acm">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msadp32.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
