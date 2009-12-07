/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Empty exception thrown when the parameter processor detects an invalid parameter
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * Constructs an empty CInvalidParameterException object, which is catched in wmain as an exception.
 */
CInvalidParameterException::CInvalidParameterException()
{
}
