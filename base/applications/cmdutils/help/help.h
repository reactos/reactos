/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS help utility
 * FILE:            base/applications/cmdutils/help/help.h
 * PURPOSE:         Provide help for command-line utilities
 * PROGRAMMERS:     Lee Schroeder (spaceseel at gmail dot com)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#define CMDLINE_LENGTH  1024

/*
 * Internal commands sorted by alphabetical order.
 * WARNING: Keep this list in sync with base\shell\cmd\cmdtable.c
 */
LPCWSTR InternalCommands[] =
{
    L"activate",
    L"alias",
    L"assoc",
    L"attrib",
    L"beep",
    L"call",
    L"cd",
    L"chcp",
    L"chdir",
    L"choice",
    L"cls",
    L"cmd",
    L"color",
    L"copy",
    L"date",
    L"del",
    L"delay",
    L"delete",
    L"dir",
    L"dirs",
    L"dirstack",
    L"echo",
    L"echoerr",
    L"echos",
    L"echoserr",
    L"endlocal",
    L"erase",
    L"exit",
    L"for",
    L"free",
    L"goto",
    L"help",
    L"history",
    L"if",
    L"label",
    L"md",
    L"memory",
    L"mkdir",
    L"mklink",
    L"move",
    L"msgbox",
    L"path",
    L"pause",
    L"popd",
    L"prompt",
    L"pushd",
    L"rd",
    L"rem",
    L"ren",
    L"rename",
    L"replace",
    L"rmdir",
    L"screen",
    L"set",
    L"setlocal",
    L"shift",
    L"start",
    L"time",
    L"timer",
    L"title",
    L"type",
    L"ver",
    L"verify",
    L"vol",
    L"window",
};

/* EOF */
