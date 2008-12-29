/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_base64(void);
extern void func_cert(void);
extern void func_chain(void);
extern void func_crl(void);
extern void func_ctl(void);
extern void func_encode(void);
extern void func_main(void);
extern void func_message(void);
extern void func_msg(void);
extern void func_object(void);
extern void func_oid(void);
extern void func_protectdata(void);
extern void func_sip(void);
extern void func_store(void);
extern void func_str(void);

const struct test winetest_testlist[] =
{
    { "asn", func_base64 },
    { "cert", func_cert },
    { "chain", func_chain },
    { "crl", func_crl },
    { "ctl", func_ctl },
	{ "encode", func_encode },
	{ "main", func_main },
	{ "message", func_message },
	{ "msg", func_msg },
	{ "object", func_object },
	{ "oid", func_oid },
	{ "protectdata", func_protectdata },
	{ "sip", func_sip },
	{ "store", func_store },
	{ "str", func_str },
	{ 0, 0 }
};
