$(SDK_INC_PATH)\std.h : std.h std.sed
    sed -f std.sed < std.h > $@

