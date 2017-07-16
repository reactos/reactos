/*
    Copyright 1991-2017 Amebis

    This file is part of atlex.

    atlex is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    atlex is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with atlex. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <eaptypes.h>


namespace ATL
{
    namespace EAP
    {
        ///
        /// \defgroup ATLEAPAPI Extensible Authentication Protocol API
        /// Integrates ATL classes with Microsoft EAP API
        ///
        /// @{

        ///
        /// EAP_ATTRIBUTE wrapper class
        ///
        class CEAPAttribute : public EAP_ATTRIBUTE
        {
        public:
            ///
            /// Initializes a new EAP attribute set to eatReserved.
            ///
            CEAPAttribute()
            {
                eaType   = eatReserved;
                dwLength = 0;
                pValue   = NULL;
            }

            ///
            /// Destroys the EAP attribute.
            ///
            ~CEAPAttribute()
            {
                if (pValue)
                    delete pValue;
            }
        };

        /// @}
    }
}
