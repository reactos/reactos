obj\$(TARGET_DIRECTORY)\smbtrsup.def: smbtrsup.src
    $(TARGET_CPP) /EP $(CDEFINES) smbtrsup.src > obj\$(TARGET_DIRECTORY)\smbtrsup.def
