<module name="msgsm32.acm" type="win32dll" baseaddress="${BASEADDRESS_MSGSM32ACM}" installbase="system32" installname="msgsm32.acm" allowwarnings="true" entrypoint="0">
	<importlibrary definition="msgsm32.acm.spec" />
	<include base="msgsm32.acm">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msgsm32.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
</module>
