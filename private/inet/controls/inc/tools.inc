#=------------------------------------------------------------------------=
# Tools.Inc
#=------------------------------------------------------------------------=
# Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
# PARTICULAR PURPOSE.
#=--------------------------------------------------------------------------=
#
# sets up the various tools required to build a control.
#


CC=cl
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
ML=asaxp
!elseif "$(PROCESSOR_ARCHITECTURE)"=="MIPS"
ML=cl
!elseif "$(PROCESSOR_ARCHITECTURE)"=="PPC"
ML=pas
!else
ML=ml
!endif
RC=rc
LINK=link
MKTYPLIB=mktyplib
MKDEP=mkdep
