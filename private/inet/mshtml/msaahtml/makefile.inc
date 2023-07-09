$(O)\guid.obj : guid.cpp
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo$(MAKEDIR)\$@ $(USECXX_FLAG) $(MAKEDIR)\guid.cpp
$(NP_COMPILER_FLAGS: =
)
<<NOKEEP
