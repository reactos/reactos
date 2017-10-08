/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Empty exception thrown when the parameter processor detects an invalid parameter
 * COPYRIGHT:   Copyright 2009 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

/**
 * Constructs an empty CInvalidParameterException object, which is caught in wmain as an exception.
 */
CInvalidParameterException::CInvalidParameterException()
{
}
