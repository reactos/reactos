/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Return FALSE if no default mail client is installed.
 */
static BOOL HaveDefaultMailClient(void)
{
    HKEY Key;
    DWORD Type, Size;
    BYTE Buffer[64];
    BOOL HasHKCUKey;

    /* We check the default value of both HKCU\Software\Clients\Mail and
     * HKLM\Software\Clients\Mail, if one of them is present there is a default
     * mail client. If neither of these keys is present, we might be running
     * on an old Windows version (W95, NT4) and we assume a default mail client
     * might be available. Only if one of the keys is present, but there is
     * no default value do we assume there is no default client. */
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Clients\\Mail", 0, KEY_QUERY_VALUE, &Key) == ERROR_SUCCESS)
    {
        Size = sizeof(Buffer);
        /* Any return value besides ERROR_FILE_NOT_FOUND (including success,
           ERROR_MORE_DATA) indicates the value is present */
        if (RegQueryValueExA(Key, NULL, NULL, &Type, Buffer, &Size) != ERROR_FILE_NOT_FOUND)
        {
            RegCloseKey(Key);
            return TRUE;
        }
        RegCloseKey(Key);
        HasHKCUKey = TRUE;
    }
    else
        HasHKCUKey = FALSE;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Clients\\Mail", 0, KEY_QUERY_VALUE, &Key) == ERROR_SUCCESS)
    {
        Size = sizeof(Buffer);
        if (RegQueryValueExA(Key, NULL, NULL, &Type, Buffer, &Size) != ERROR_FILE_NOT_FOUND)
        {
            RegCloseKey(Key);
            return TRUE;
        }
        RegCloseKey(Key);
        return FALSE;
    }

    return ! HasHKCUKey;
}
