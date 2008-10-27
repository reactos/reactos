<module name="ntdsapi" type="win32dll" baseaddress="${BASEADDRESS_NTDSAPI}" installbase="system32" installname="ntdsapi.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="ntdsapi.spec" />
        <include base="ntdsapi">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <library>wine</library>
        <library>user32</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>ntdsapi.c</file>
        <file>ntdsapi.spec</file>
</module>
