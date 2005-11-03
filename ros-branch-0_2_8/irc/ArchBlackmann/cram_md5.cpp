// cram_md5.cpp
// This file is (C) 2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#include "MD5.h"
#include "cram_md5.h"
#include "base64.h"

using std::string;

string cram_md5 ( const string& username, const string& password, const string& greeting )
{
	string challenge = base64_decode ( greeting );

	string hmac = HMAC_MD5 ( password, challenge );
	//printf ( "(cram_md5): hmac = %s\n", hmac.c_str() );
	string raw_response = username;
	raw_response += " ";
	raw_response += hmac;
	//printf ( "(cram_md5): raw_response = %s\n", raw_response.c_str() );

	return base64_encode ( raw_response );
}
