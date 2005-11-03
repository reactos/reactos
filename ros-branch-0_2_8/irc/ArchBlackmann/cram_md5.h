// cram_md5.h
// This file is (C) 2004 Royce Mitchell III
// and released under the BSD & LGPL licenses

#ifndef CRAM_MD5_H
#define CRAM_MD5_H

#include <string>

std::string cram_md5 (
	const std::string& username,
	const std::string& password,
	const std::string& greeting );

#endif//CRAM_MD5_H
