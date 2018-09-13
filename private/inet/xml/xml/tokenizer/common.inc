##########################################################################
#
#   Microsoft Confidential
#   Copyright (c) 1991 - 1999 Microsoft Corporation.
#   All Rights Reserved.
#
##########################################################################

!include $(ROOT)\common.inc

STRUCTURED_EXCEPTION=0

SYNCHRONIZE_BLOCK=1

XML_TOKENIZER_INCLUDES = \
    $(TOK_ROOT); \
    $(MSXML)\include; \
    $(MSXML); \
    $(MSXML)\core\base;\
    $(WINDOWS_INC_PATH)


#
# Override normal MSXML optimization flags - no structured exceptions.
# 9/17 - not any more...
#
# !if !$(FREEBUILD)
# USE_RTTI=0
# _DEBUG=1
# MSC_OPTIMIZATION=/Odi
# !else
# MSC_OPTIMIZATION=/Oxs
# !ENDIF

!ifdef WIN16
PRECOMPILED_OPTION=/fhr /fh=$(MSXML)\include\$(O)core.pch
!else
PRECOMPILED_OPTION=/Yucore.hxx /Fp$(MSXML)\include\$(O)\core.pch
!endif
PRECOMPILED_CXX=1

CONDITIONAL_INCLUDES=\
	macwin32.h\
	macocidl.h\
	winwlm.h\
	rpcerr.h\
	macname1.h\
	macname2.h\
	macpub.h\
	macapi.h\
	shlwapip.h\
        w95wraps.h\
	rpcmac.h\
	pure.h\
	codepage.h
