<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
  <module name="winfile" type="win32gui" installbase="system32" installname="winfile.exe" allowwarnings="true">
    <include base="winfile">.</include>
    <define name="UNICODE" />
    <library>shell32</library>
    <library>comdlg32</library>
    <library>comctl32</library>
    <library>ole32</library>
    <library>mpr</library>
    <library>version</library>
    <library>user32</library>
    <library>gdi32</library>
    <library>advapi32</library>
    <library>kernel32</library>
    <library>uuid</library>
    <file>winefile.c</file>
    <file>rsrc.rc</file>
  </module>
</rbuild>
