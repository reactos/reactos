
# This is the list of html and related files packaged in the resource

HTM_FILES = \
            ..\html\obj\arp.css         \
            ..\html\obj\default.hta     \
            ..\html\obj\accel.htc       \
            ..\html\obj\listbox.htc     \
            ..\html\obj\places.htc      \
            ..\html\obj\def_freq.htm    \
            ..\html\obj\def_size.htm    \
            ..\html\obj\support.htm     \
            ..\html\obj\add.js          \
            ..\html\obj\bplate.js       \
            ..\html\obj\config.js       \
            ..\html\obj\dlg.js          \
            ..\html\obj\preload.js      \
            ..\html\obj\remove.js       \
            ..\html\obj\util.js         \

OTHER_FILES = \
            ..\selfreg_appwiz.inf   \
            ..\adcctl.rgs           \
            ..\carpctl.rgs          \
            $(SDK_LIB_PATH)\shappmgr.tlb

..\appwiz.rc : $(OTHER_FILES) $(HTM_FILES)


# 
# Inference rules
#

.SUFFIXES:.js .htc .htm .hta .css

{..\html}.hta{..\html\obj}.hta:
    @if not exist ..\html\obj\nul md ..\html\obj
    cleaninf -w $** $@

{..\html}.htm{..\html\obj}.htm:
    @if not exist ..\html\obj\nul md ..\html\obj
    cleaninf -w $** $@

{..\html}.htc{..\html\obj}.htc:
    @if not exist ..\html\obj\nul md ..\html\obj
    cleaninf -w $** $@

{..\html}.css{..\html\obj}.css:
    @if not exist ..\html\obj\nul md ..\html\obj
    cleaninf -w -htc $** $@

{..\html}.js{..\html\obj}.js:
    @if not exist ..\html\obj\nul md ..\html\obj
    cleaninf -w $** $@


!include $(CCSHELL_DIR)\makefile.inc
