#*************************************************************#
#**                                                         **#
#**                 Microsoft RPC Examples                  **#
#**		      hello Application			    **#
#**	       Copyright(c) Microsoft Corp. 1992	    **#
#**                                                         **#
#*************************************************************#

!INCLUDE $(NTMAKEENV)\makefile.plt

CPP = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) $(C_DEFINES) $(NET_C_DEFINES)
IDL_NAME = hello

# Stubs, auxiliary and header file from the IDL file
hello.h hello_c.c hello_x.c hello_s.c hello_y.c : hello.idl hello.acf
    midl -Oi -error allocation -error ref -ms_ext -c_ext $(CPP) $(CLIENT_FLAGS) .\$(IDL_NAME).idl $(INCS)
    copy hello_c.c client
    copy hello_s.c server



clean :
    -del hello_c.c
    -del hello_x.c
    -del hello_s.c
    -del hello_y.c
    -del hello.h
