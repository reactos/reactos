<module name="nddeapi" type="win32dll" baseaddress="${BASEADDRESS_NDDEAPI}" installbase="system32" installname="nddeapi.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="nddeapi.spec" />
        <include base="nddeapi">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <library>wine</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>nddeapi.c</file>
</module>
