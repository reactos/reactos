/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Simple exception containing just a message
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

/**
 * Constructs a CSimpleException object, which is catched in wmain as an exception.
 * You should always use the EXCEPTION or SSEXCEPTION macro for throwing this exception.
 *
 * @param Message
 * String containing a short message about the exception
 */
CSimpleException::CSimpleException(const string& Message)
    : m_Message(Message)
{
}
