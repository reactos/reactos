
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

#include <unistd.h>
#include <stdlib.h>

using namespace std;

string to_upper(string s)
{
    string temp = s;
    transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
    return temp;
}

string realpath(const char* path)
{
    char* temp = realpath(path, NULL);
    if (temp == NULL)
        throw runtime_error("realpath failed");
    string result(temp);
    free(temp);
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
