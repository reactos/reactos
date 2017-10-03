
// This file is part of hhpcomp, a free HTML Help Project (*.hhp) compiler.
// Copyright (C) 2015  Benedikt Freisen
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


#include <string>
#include <algorithm>
#include <stdexcept>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>  // for GetFullPathNameA 
#else
    #include <unistd.h>
#endif

#include <stdlib.h>

using namespace std;

string to_upper(string s)
{
    string temp = s;
    transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
    return temp;
}

string real_path(const char* path)
{
    char* temp = NULL;
    #if defined(_WIN32)
        char temp2[MAX_PATH];
        if (GetFullPathNameA(path, MAX_PATH, temp2, NULL)) {
            temp = temp2;
        }
    #else
        temp = realpath(path, NULL);
    #endif
    if (temp == NULL)
        throw runtime_error("realpath failed");
    string result(temp);
    #if !defined(_WIN32)
        free(temp);
    #endif
    return result;
}

string replace_backslashes(string s)
{
    string temp = s;
    for (string::iterator it = temp.begin(); it != temp.end(); ++it)
        if (*it == '\\')
            *it = '/';
    return temp;
}
