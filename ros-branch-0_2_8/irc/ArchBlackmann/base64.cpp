// base64.cpp

#include "base64.h"

using std::string;

static const char* alfabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

string base64_encode ( const string& sInput )
{
	unsigned char x=0, topbit=0;
	int v=0;
	string sOutput;
	do
	{
		if ( topbit < 6 )
		{
			x++;
			v <<= 8;
			if ( x <= sInput.length() ) v += sInput[x-1];
			topbit += 8;
		}
		topbit -= 6;
		if ( x > sInput.length() && !v )
			break;
		sOutput += alfabet[(v >> topbit) & 63];
		v &= (1 << topbit) - 1;
	} while ( x < sInput.length() || v );
	int eq = (8 - (sOutput.length() % 4)) % 4;
	while ( eq-- )
		sOutput += '=';
	return sOutput;
}

string base64_decode ( const string& sInput )
{
	unsigned char x=0, topbit=0;
	int v=0, inlen = sInput.length();
	string sOutput;
	while ( inlen && sInput[inlen-1] == '=' )
		inlen--;
	do
	{
		while ( topbit < 8 )
		{
			x++;
			v <<= 6;
			if ( x <= inlen ) v += (strchr(alfabet, sInput[x-1]) - alfabet);
			topbit += 6;
		}
		topbit -= 8;
		if ( x > inlen && !v )
			break;
		sOutput += (char)((v >> topbit) & 255);
		v &= ((1 << topbit) - 1);
	} while ( x <= inlen || v );
	return sOutput;
}
