/*
 * Copyright (C) 2004, 2005, 2007, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: loc_29.c,v 1.45.332.4 2009/02/17 05:54:12 marka Exp $ */

/* Reviewed: Wed Mar 15 18:13:09 PST 2000 by explorer */

/* RFC1876 */

#ifndef RDATA_GENERIC_LOC_29_C
#define RDATA_GENERIC_LOC_29_C

#define RRTYPE_LOC_ATTRIBUTES (0)

static inline isc_result_t
fromtext_loc(ARGS_FROMTEXT) {
	isc_token_t token;
	int d1, m1, s1;
	int d2, m2, s2;
	unsigned char size;
	unsigned char hp;
	unsigned char vp;
	unsigned char version;
	isc_boolean_t east = ISC_FALSE;
	isc_boolean_t north = ISC_FALSE;
	long tmp;
	long m;
	long cm;
	long poweroften[8] = { 1, 10, 100, 1000,
			       10000, 100000, 1000000, 10000000 };
	int man;
	int exp;
	char *e;
	int i;
	unsigned long latitude;
	unsigned long longitude;
	unsigned long altitude;

	REQUIRE(type == 29);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);

	/*
	 * Defaults.
	 */
	m1 = s1 = 0;
	m2 = s2 = 0;
	size = 0x12;	/* 1.00m */
	hp = 0x16;	/* 10000.00 m */
	vp = 0x13;	/* 10.00 m */
	version = 0;

	/*
	 * Degrees.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 90U)
		RETTOK(ISC_R_RANGE);
	d1 = (int)token.value.as_ulong;
	/*
	 * Minutes.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	if (strcasecmp(DNS_AS_STR(token), "N") == 0)
		north = ISC_TRUE;
	if (north || strcasecmp(DNS_AS_STR(token), "S") == 0)
		goto getlong;
	m1 = strtol(DNS_AS_STR(token), &e, 10);
	if (*e != 0)
		RETTOK(DNS_R_SYNTAX);
	if (m1 < 0 || m1 > 59)
		RETTOK(ISC_R_RANGE);
	if (d1 == 90 && m1 != 0)
		RETTOK(ISC_R_RANGE);

	/*
	 * Seconds.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	if (strcasecmp(DNS_AS_STR(token), "N") == 0)
		north = ISC_TRUE;
	if (north || strcasecmp(DNS_AS_STR(token), "S") == 0)
		goto getlong;
	s1 = strtol(DNS_AS_STR(token), &e, 10);
	if (*e != 0 && *e != '.')
		RETTOK(DNS_R_SYNTAX);
	if (s1 < 0 || s1 > 59)
		RETTOK(ISC_R_RANGE);
	if (*e == '.') {
		const char *l;
		e++;
		for (i = 0; i < 3; i++) {
			if (*e == 0)
				break;
			if ((tmp = decvalue(*e++)) < 0)
				RETTOK(DNS_R_SYNTAX);
			s1 *= 10;
			s1 += tmp;
		}
		for (; i < 3; i++)
			s1 *= 10;
		l = e;
		while (*e != 0) {
			if (decvalue(*e++) < 0)
				RETTOK(DNS_R_SYNTAX);
		}
		if (*l != '\0' && callbacks != NULL) {
			const char *file = isc_lex_getsourcename(lexer);
			unsigned long line = isc_lex_getsourceline(lexer);

			if (file == NULL)
				file = "UNKNOWN";
			(*callbacks->warn)(callbacks, "%s: %s:%u: '%s' extra "
					   "precision digits ignored",
					   "dns_rdata_fromtext", file, line,
					   DNS_AS_STR(token));
		}
	} else
		s1 *= 1000;
	if (d1 == 90 && s1 != 0)
		RETTOK(ISC_R_RANGE);

	/*
	 * Direction.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	if (strcasecmp(DNS_AS_STR(token), "N") == 0)
		north = ISC_TRUE;
	if (!north && strcasecmp(DNS_AS_STR(token), "S") != 0)
		RETTOK(DNS_R_SYNTAX);

 getlong:
	/*
	 * Degrees.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 180U)
		RETTOK(ISC_R_RANGE);
	d2 = (int)token.value.as_ulong;

	/*
	 * Minutes.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	if (strcasecmp(DNS_AS_STR(token), "E") == 0)
		east = ISC_TRUE;
	if (east || strcasecmp(DNS_AS_STR(token), "W") == 0)
		goto getalt;
	m2 = strtol(DNS_AS_STR(token), &e, 10);
	if (*e != 0)
		RETTOK(DNS_R_SYNTAX);
	if (m2 < 0 || m2 > 59)
		RETTOK(ISC_R_RANGE);
	if (d2 == 180 && m2 != 0)
		RETTOK(ISC_R_RANGE);

	/*
	 * Seconds.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	if (strcasecmp(DNS_AS_STR(token), "E") == 0)
		east = ISC_TRUE;
	if (east || strcasecmp(DNS_AS_STR(token), "W") == 0)
		goto getalt;
	s2 = strtol(DNS_AS_STR(token), &e, 10);
	if (*e != 0 && *e != '.')
		RETTOK(DNS_R_SYNTAX);
	if (s2 < 0 || s2 > 59)
		RETTOK(ISC_R_RANGE);
	if (*e == '.') {
		const char *l;
		e++;
		for (i = 0; i < 3; i++) {
			if (*e == 0)
				break;
			if ((tmp = decvalue(*e++)) < 0)
				RETTOK(DNS_R_SYNTAX);
			s2 *= 10;
			s2 += tmp;
		}
		for (; i < 3; i++)
			s2 *= 10;
		l = e;
		while (*e != 0) {
			if (decvalue(*e++) < 0)
				RETTOK(DNS_R_SYNTAX);
		}
		if (*l != '\0' && callbacks != NULL) {
			const char *file = isc_lex_getsourcename(lexer);
			unsigned long line = isc_lex_getsourceline(lexer);

			if (file == NULL)
				file = "UNKNOWN";
			(*callbacks->warn)(callbacks, "%s: %s:%u: '%s' extra "
					   "precision digits ignored",
					   "dns_rdata_fromtext",
					   file, line, DNS_AS_STR(token));
		}
	} else
		s2 *= 1000;
	if (d2 == 180 && s2 != 0)
		RETTOK(ISC_R_RANGE);

	/*
	 * Direction.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	if (strcasecmp(DNS_AS_STR(token), "E") == 0)
		east = ISC_TRUE;
	if (!east && strcasecmp(DNS_AS_STR(token), "W") != 0)
		RETTOK(DNS_R_SYNTAX);

 getalt:
	/*
	 * Altitude.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	m = strtol(DNS_AS_STR(token), &e, 10);
	if (*e != 0 && *e != '.' && *e != 'm')
		RETTOK(DNS_R_SYNTAX);
	if (m < -100000 || m > 42849672)
		RETTOK(ISC_R_RANGE);
	cm = 0;
	if (*e == '.') {
		e++;
		for (i = 0; i < 2; i++) {
			if (*e == 0 || *e == 'm')
				break;
			if ((tmp = decvalue(*e++)) < 0)
				return (DNS_R_SYNTAX);
			cm *= 10;
			if (m < 0)
				cm -= tmp;
			else
				cm += tmp;
		}
		for (; i < 2; i++)
			cm *= 10;
	}
	if (*e == 'm')
		e++;
	if (*e != 0)
		RETTOK(DNS_R_SYNTAX);
	if (m == -100000 && cm != 0)
		RETTOK(ISC_R_RANGE);
	if (m == 42849672 && cm > 95)
		RETTOK(ISC_R_RANGE);
	/*
	 * Adjust base.
	 */
	altitude = m + 100000;
	altitude *= 100;
	altitude += cm;

	/*
	 * Size: optional.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_TRUE));
	if (token.type == isc_tokentype_eol ||
	    token.type == isc_tokentype_eof) {
		isc_lex_ungettoken(lexer, &token);
		goto encode;
	}
	m = strtol(DNS_AS_STR(token), &e, 10);
	if (*e != 0 && *e != '.' && *e != 'm')
		RETTOK(DNS_R_SYNTAX);
	if (m < 0 || m > 90000000)
		RETTOK(ISC_R_RANGE);
	cm = 0;
	if (*e == '.') {
		e++;
		for (i = 0; i < 2; i++) {
			if (*e == 0 || *e == 'm')
				break;
			if ((tmp = decvalue(*e++)) < 0)
				RETTOK(DNS_R_SYNTAX);
			cm *= 10;
			cm += tmp;
		}
		for (; i < 2; i++)
			cm *= 10;
	}
	if (*e == 'm')
		e++;
	if (*e != 0)
		RETTOK(DNS_R_SYNTAX);
	/*
	 * We don't just multiply out as we will overflow.
	 */
	if (m > 0) {
		for (exp = 0; exp < 7; exp++)
			if (m < poweroften[exp+1])
				break;
		man = m / poweroften[exp];
		exp += 2;
	} else {
		if (cm >= 10) {
			man = cm / 10;
			exp = 1;
		} else {
			man = cm;
			exp = 0;
		}
	}
	size = (man << 4) + exp;

	/*
	 * Horizontal precision: optional.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_TRUE));
	if (token.type == isc_tokentype_eol ||
	    token.type == isc_tokentype_eof) {
		isc_lex_ungettoken(lexer, &token);
		goto encode;
	}
	m = strtol(DNS_AS_STR(token), &e, 10);
	if (*e != 0 && *e != '.' && *e != 'm')
		RETTOK(DNS_R_SYNTAX);
	if (m < 0 || m > 90000000)
		RETTOK(ISC_R_RANGE);
	cm = 0;
	if (*e == '.') {
		e++;
		for (i = 0; i < 2; i++) {
			if (*e == 0 || *e == 'm')
				break;
			if ((tmp = decvalue(*e++)) < 0)
				RETTOK(DNS_R_SYNTAX);
			cm *= 10;
			cm += tmp;
		}
		for (; i < 2; i++)
			cm *= 10;
	}
	if (*e == 'm')
		e++;
	if (*e != 0)
		RETTOK(DNS_R_SYNTAX);
	/*
	 * We don't just multiply out as we will overflow.
	 */
	if (m > 0) {
		for (exp = 0; exp < 7; exp++)
			if (m < poweroften[exp+1])
				break;
		man = m / poweroften[exp];
		exp += 2;
	} else if (cm >= 10) {
		man = cm / 10;
		exp = 1;
	} else  {
		man = cm;
		exp = 0;
	}
	hp = (man << 4) + exp;

	/*
	 * Vertical precision: optional.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_TRUE));
	if (token.type == isc_tokentype_eol ||
	    token.type == isc_tokentype_eof) {
		isc_lex_ungettoken(lexer, &token);
		goto encode;
	}
	m = strtol(DNS_AS_STR(token), &e, 10);
	if (*e != 0 && *e != '.' && *e != 'm')
		RETTOK(DNS_R_SYNTAX);
	if (m < 0 || m > 90000000)
		RETTOK(ISC_R_RANGE);
	cm = 0;
	if (*e == '.') {
		e++;
		for (i = 0; i < 2; i++) {
			if (*e == 0 || *e == 'm')
				break;
			if ((tmp = decvalue(*e++)) < 0)
				RETTOK(DNS_R_SYNTAX);
			cm *= 10;
			cm += tmp;
		}
		for (; i < 2; i++)
			cm *= 10;
	}
	if (*e == 'm')
		e++;
	if (*e != 0)
		RETTOK(DNS_R_SYNTAX);
	/*
	 * We don't just multiply out as we will overflow.
	 */
	if (m > 0) {
		for (exp = 0; exp < 7; exp++)
			if (m < poweroften[exp+1])
				break;
		man = m / poweroften[exp];
		exp += 2;
	} else if (cm >= 10) {
		man = cm / 10;
		exp = 1;
	} else {
		man = cm;
		exp = 0;
	}
	vp = (man << 4) + exp;

 encode:
	RETERR(mem_tobuffer(target, &version, 1));
	RETERR(mem_tobuffer(target, &size, 1));
	RETERR(mem_tobuffer(target, &hp, 1));
	RETERR(mem_tobuffer(target, &vp, 1));
	if (north)
		latitude = 0x80000000 + ( d1 * 3600 + m1 * 60 ) * 1000 + s1;
	else
		latitude = 0x80000000 - ( d1 * 3600 + m1 * 60 ) * 1000 - s1;
	RETERR(uint32_tobuffer(latitude, target));

	if (east)
		longitude = 0x80000000 + ( d2 * 3600 + m2 * 60 ) * 1000 + s2;
	else
		longitude = 0x80000000 - ( d2 * 3600 + m2 * 60 ) * 1000 - s2;
	RETERR(uint32_tobuffer(longitude, target));

	return (uint32_tobuffer(altitude, target));
}

static inline isc_result_t
totext_loc(ARGS_TOTEXT) {
	int d1, m1, s1, fs1;
	int d2, m2, s2, fs2;
	unsigned long latitude;
	unsigned long longitude;
	unsigned long altitude;
	isc_boolean_t north;
	isc_boolean_t east;
	isc_boolean_t below;
	isc_region_t sr;
	char buf[sizeof("89 59 59.999 N 179 59 59.999 E "
			"42849672.95m 90000000m 90000000m 90000000m")];
	char sbuf[sizeof("90000000m")];
	char hbuf[sizeof("90000000m")];
	char vbuf[sizeof("90000000m")];
	unsigned char size, hp, vp;
	unsigned long poweroften[8] = { 1, 10, 100, 1000,
					10000, 100000, 1000000, 10000000 };

	UNUSED(tctx);

	REQUIRE(rdata->type == 29);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);

	/* version = sr.base[0]; */
	size = sr.base[1];
	INSIST((size&0x0f) < 10 && (size>>4) < 10);
	if ((size&0x0f)> 1)
		sprintf(sbuf, "%lum", (size>>4) * poweroften[(size&0x0f)-2]);
	else
		sprintf(sbuf, "0.%02lum", (size>>4) * poweroften[(size&0x0f)]);
	hp = sr.base[2];
	INSIST((hp&0x0f) < 10 && (hp>>4) < 10);
	if ((hp&0x0f)> 1)
		sprintf(hbuf, "%lum", (hp>>4) * poweroften[(hp&0x0f)-2]);
	else
		sprintf(hbuf, "0.%02lum", (hp>>4) * poweroften[(hp&0x0f)]);
	vp = sr.base[3];
	INSIST((vp&0x0f) < 10 && (vp>>4) < 10);
	if ((vp&0x0f)> 1)
		sprintf(vbuf, "%lum", (vp>>4) * poweroften[(vp&0x0f)-2]);
	else
		sprintf(vbuf, "0.%02lum", (vp>>4) * poweroften[(vp&0x0f)]);
	isc_region_consume(&sr, 4);

	latitude = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	if (latitude >= 0x80000000) {
		north = ISC_TRUE;
		latitude -= 0x80000000;
	} else {
		north = ISC_FALSE;
		latitude = 0x80000000 - latitude;
	}
	fs1 = (int)(latitude % 1000);
	latitude /= 1000;
	s1 = (int)(latitude % 60);
	latitude /= 60;
	m1 = (int)(latitude % 60);
	latitude /= 60;
	d1 = (int)latitude;
	INSIST(latitude <= 90U);

	longitude = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	if (longitude >= 0x80000000) {
		east = ISC_TRUE;
		longitude -= 0x80000000;
	} else {
		east = ISC_FALSE;
		longitude = 0x80000000 - longitude;
	}
	fs2 = (int)(longitude % 1000);
	longitude /= 1000;
	s2 = (int)(longitude % 60);
	longitude /= 60;
	m2 = (int)(longitude % 60);
	longitude /= 60;
	d2 = (int)longitude;
	INSIST(longitude <= 180U);

	altitude = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	if (altitude < 10000000U) {
		below = ISC_TRUE;
		altitude = 10000000 - altitude;
	} else {
		below =ISC_FALSE;
		altitude -= 10000000;
	}

	sprintf(buf, "%d %d %d.%03d %s %d %d %d.%03d %s %s%ld.%02ldm %s %s %s",
		d1, m1, s1, fs1, north ? "N" : "S",
		d2, m2, s2, fs2, east ? "E" : "W",
		below ? "-" : "", altitude/100, altitude % 100,
		sbuf, hbuf, vbuf);

	return (str_totext(buf, target));
}

static inline isc_result_t
fromwire_loc(ARGS_FROMWIRE) {
	isc_region_t sr;
	unsigned char c;
	unsigned long latitude;
	unsigned long longitude;

	REQUIRE(type == 29);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 1)
		return (ISC_R_UNEXPECTEDEND);
	if (sr.base[0] != 0)
		return (ISC_R_NOTIMPLEMENTED);
	if (sr.length < 16)
		return (ISC_R_UNEXPECTEDEND);

	/*
	 * Size.
	 */
	c = sr.base[1];
	if (c != 0)
		if ((c&0xf) > 9 || ((c>>4)&0xf) > 9 || ((c>>4)&0xf) == 0)
			return (ISC_R_RANGE);

	/*
	 * Horizontal precision.
	 */
	c = sr.base[2];
	if (c != 0)
		if ((c&0xf) > 9 || ((c>>4)&0xf) > 9 || ((c>>4)&0xf) == 0)
			return (ISC_R_RANGE);

	/*
	 * Vertical precision.
	 */
	c = sr.base[3];
	if (c != 0)
		if ((c&0xf) > 9 || ((c>>4)&0xf) > 9 || ((c>>4)&0xf) == 0)
			return (ISC_R_RANGE);
	isc_region_consume(&sr, 4);

	/*
	 * Latitude.
	 */
	latitude = uint32_fromregion(&sr);
	if (latitude < (0x80000000UL - 90 * 3600000) ||
	    latitude > (0x80000000UL + 90 * 3600000))
		return (ISC_R_RANGE);
	isc_region_consume(&sr, 4);

	/*
	 * Longitude.
	 */
	longitude = uint32_fromregion(&sr);
	if (longitude < (0x80000000UL - 180 * 3600000) ||
	    longitude > (0x80000000UL + 180 * 3600000))
		return (ISC_R_RANGE);

	/*
	 * Altitude.
	 * All values possible.
	 */

	isc_buffer_activeregion(source, &sr);
	isc_buffer_forward(source, 16);
	return (mem_tobuffer(target, sr.base, 16));
}

static inline isc_result_t
towire_loc(ARGS_TOWIRE) {
	UNUSED(cctx);

	REQUIRE(rdata->type == 29);
	REQUIRE(rdata->length != 0);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_loc(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 29);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_loc(ARGS_FROMSTRUCT) {
	dns_rdata_loc_t *loc = source;
	isc_uint8_t c;

	REQUIRE(type == 29);
	REQUIRE(source != NULL);
	REQUIRE(loc->common.rdtype == type);
	REQUIRE(loc->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	if (loc->v.v0.version != 0)
		return (ISC_R_NOTIMPLEMENTED);
	RETERR(uint8_tobuffer(loc->v.v0.version, target));

	c = loc->v.v0.size;
	if ((c&0xf) > 9 || ((c>>4)&0xf) > 9 || ((c>>4)&0xf) == 0)
		return (ISC_R_RANGE);
	RETERR(uint8_tobuffer(loc->v.v0.size, target));

	c = loc->v.v0.horizontal;
	if ((c&0xf) > 9 || ((c>>4)&0xf) > 9 || ((c>>4)&0xf) == 0)
		return (ISC_R_RANGE);
	RETERR(uint8_tobuffer(loc->v.v0.horizontal, target));

	c = loc->v.v0.vertical;
	if ((c&0xf) > 9 || ((c>>4)&0xf) > 9 || ((c>>4)&0xf) == 0)
		return (ISC_R_RANGE);
	RETERR(uint8_tobuffer(loc->v.v0.vertical, target));

	if (loc->v.v0.latitude < (0x80000000UL - 90 * 3600000) ||
	    loc->v.v0.latitude > (0x80000000UL + 90 * 3600000))
		return (ISC_R_RANGE);
	RETERR(uint32_tobuffer(loc->v.v0.latitude, target));

	if (loc->v.v0.longitude < (0x80000000UL - 180 * 3600000) ||
	    loc->v.v0.longitude > (0x80000000UL + 180 * 3600000))
		return (ISC_R_RANGE);
	RETERR(uint32_tobuffer(loc->v.v0.longitude, target));
	return (uint32_tobuffer(loc->v.v0.altitude, target));
}

static inline isc_result_t
tostruct_loc(ARGS_TOSTRUCT) {
	dns_rdata_loc_t *loc = target;
	isc_region_t r;
	isc_uint8_t version;

	REQUIRE(rdata->type == 29);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	UNUSED(mctx);

	dns_rdata_toregion(rdata, &r);
	version = uint8_fromregion(&r);
	if (version != 0)
		return (ISC_R_NOTIMPLEMENTED);

	loc->common.rdclass = rdata->rdclass;
	loc->common.rdtype = rdata->type;
	ISC_LINK_INIT(&loc->common, link);

	loc->v.v0.version = version;
	isc_region_consume(&r, 1);
	loc->v.v0.size = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	loc->v.v0.horizontal = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	loc->v.v0.vertical = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	loc->v.v0.latitude = uint32_fromregion(&r);
	isc_region_consume(&r, 4);
	loc->v.v0.longitude = uint32_fromregion(&r);
	isc_region_consume(&r, 4);
	loc->v.v0.altitude = uint32_fromregion(&r);
	isc_region_consume(&r, 4);
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_loc(ARGS_FREESTRUCT) {
	dns_rdata_loc_t *loc = source;

	REQUIRE(source != NULL);
	REQUIRE(loc->common.rdtype == 29);

	UNUSED(source);
	UNUSED(loc);
}

static inline isc_result_t
additionaldata_loc(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 29);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_loc(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 29);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_loc(ARGS_CHECKOWNER) {

	REQUIRE(type == 29);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_loc(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 29);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_LOC_29_C */
