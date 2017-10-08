/*

  Copyright (C) 2010 Alex Andreotti <alex.andreotti@gmail.com>

  This file is part of chmc.

  chmc is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  chmc is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with chmc.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "err.h"

#include <stdarg.h>
#include <assert.h>

struct chmcErr
{
	int code;
	char msg[CHMC_ERRMAXLEN+1];
};

static struct chmcErr chmc_err = {
	CHMC_NOERR,
	'\0',
};

void chmcerr_clean(void) {
	chmc_err.code = CHMC_NOERR;
	chmc_err.msg[0] = '\0';
}

int chmcerr_code(void) {
	return chmc_err.code;
}

const char *chmcerr_message( void ) {
	return chmc_err.msg;
}

void chmcerr_set(int code, const char *fmt, ...)
{
	int len;
	va_list ap;

	chmc_err.code = code;

	va_start(ap, fmt);

	len = vsnprintf(chmc_err.msg, CHMC_ERRMAXLEN, fmt, ap);
	if (len == CHMC_ERRMAXLEN)
		chmc_err.msg[CHMC_ERRMAXLEN] = '\0';

	assert(len <= CHMC_ERRMAXLEN);

	va_end(ap);
}
