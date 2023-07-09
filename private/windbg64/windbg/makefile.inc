obj\$(TARGET_DIRECTORY)\res_str.i: include\res_str.h
    $(TARGET_CPP) -EP -DRESOURCES -Tc include\res_str.h > $@

obj\$(TARGET_DIRECTORY)\windbg.res: obj\$(TARGET_DIRECTORY)\res_str.i dialogs.dlg

