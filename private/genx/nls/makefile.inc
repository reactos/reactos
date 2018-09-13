NTSTATUS=    $(SDK_INC_PATH)\ntstatus.h \
             $(SDK_INC_PATH)\ntstatus.dbg \
             $(O)\msg00001.bin \
             $(O)\ntstatus.rc

clean:
    -del /Q /F $(NTSTATUS)

$(SDK_INC_PATH)\ntstatus.h : $(O)\ntstatus.h
    copy $** $@
