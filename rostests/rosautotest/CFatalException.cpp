/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Fatal program exception with automatically added information
 * COPYRIGHT:   Copyright 2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * Constructs a CFatalException object, which is catched in wmain as an exception.
 * You should always use the FATAL macro for throwing this exception.
 *
 * @param File
 * Constant pointer to a char array with the source file where the exception occured (__FILE__)
 *
 * @param Line
 * Constant pointer to a char array with the appropriate source line (#__LINE__)
 *
 * @param Message
 * Constant pointer to a char array containing a short message about the exception
 */
CFatalException::CFatalException(const char* File, const char* Line, const char* Message)
    : m_File(File), m_Line(Line), m_Message(Message)
{
}
