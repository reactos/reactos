<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="ps" type="win32cui" installbase="bin" installname="ps.exe">
    <include base="ps">.</include>
    <define name="__USE_W32API" />
    <library>user32</library>
    <library>kernel32</library>
    <library>ntdll</library>
    <file>ps.c</file>
</module>
</rbuild>
