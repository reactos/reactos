/*
 * Copyright (c) 2026 Terascale Functionalists
 * SPDX-License-Identifier: MIT
 *
 * Clang C++ exception base support for the ReactOS C++ runtime.
 */

#include <exception>
#include <stdlib.h>
#include <string.h>

static char*
ExceptionDuplicateString(const char *Name) throw()
{
    size_t Length;
    char *NameCopy;

    if (Name == NULL)
        return NULL;

    Length = strlen(Name) + 1;
    NameCopy = static_cast<char *>(malloc(Length));
    if (NameCopy == NULL)
        return NULL;

    memcpy(NameCopy, Name, Length);
    return NameCopy;
}

exception::exception() throw()
    : _name(NULL),
      _do_free(0)
{
}

exception::exception(const char * const &Name) throw()
    : _name(ExceptionDuplicateString(Name)),
      _do_free(_name != NULL)
{
}

exception::exception(const char * const &Name, int) throw()
    : _name(Name),
      _do_free(0)
{
}

exception::~exception() throw()
{
    if (_do_free != 0)
        free(const_cast<char *>(_name));
}

const char*
exception::what() const throw()
{
    if (_name != NULL)
        return _name;

    return "Unknown exception";
}
