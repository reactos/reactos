/*
 * Copyright (C) 2009 KJK::Hyperion
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if defined(WIN32)

#define UNICODE
#define _UNICODE

#include "../../pch.h"

#include "mingw.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(X_) (sizeof(X_) / sizeof((X_)[0]))
#endif

#include <ole2.h>

#include <assert.h>
#include <stdlib.h>
#include <tchar.h>

#include <string>
#include <istream>
#include <streambuf>
#include <ios>
#include <iostream>
#include <iterator>
#include <functional>
#include <locale>
#include <vector>
#include <algorithm>
#include <limits>
#include <sstream>

#if defined(_CPPLIB_VER)
#include <fstream>
typedef std::filebuf stdio_filebuf;
#elif defined(__GLIBCXX__)
#include <ext/stdio_sync_filebuf.h>
typedef __gnu_cxx::stdio_sync_filebuf<char> stdio_filebuf;
#else
#error Unknown or unsupported C++ standard library
#endif

namespace
{

#if 1
HRESULT dispPropGet(IDispatch * object, DISPID dispid, VARIANTARG * args, UINT ccArgs, VARTYPE vt, VARIANT& result)
{
	HRESULT hr;

	DISPPARAMS params = {};
	params.rgvarg = args;
	params.cArgs = ccArgs;

	hr = object->Invoke(dispid, IID_NULL, 0, DISPATCH_PROPERTYGET | (ccArgs ? DISPATCH_METHOD : 0), &params, &result, NULL, NULL);

	if(SUCCEEDED(hr))
		hr = VariantChangeType(&result, &result, 0, vt);

	return hr;
}

HRESULT dispPropGet(IDispatch * object, const OLECHAR * name, VARIANTARG * args, UINT ccArgs, VARTYPE vt, VARIANT& result)
{
	HRESULT hr;

	DISPID dispid;
	hr = object->GetIDsOfNames(IID_NULL, const_cast<OLECHAR **>(&name), 1, 0, &dispid);

	if(SUCCEEDED(hr))
		hr = dispPropGet(object, dispid, args, ccArgs, vt, result);

	return hr;
}

VARIANT dispPropGet(HRESULT& hr, IDispatch * object, DISPID dispid, VARTYPE vt)
{
	VARIANT ret;
	VariantInit(&ret);

	if(SUCCEEDED(hr))
		hr = dispPropGet(object, dispid, 0, 0, vt, ret);

	return ret;
}

VARIANT dispPropGet(HRESULT& hr, IDispatch * object, const OLECHAR * name, VARTYPE vt)
{
	VARIANT ret;
	VariantInit(&ret);

	if(SUCCEEDED(hr))
		hr = dispPropGet(object, name, 0, 0, vt, ret);

	return ret;
}

HRESULT dispInvoke(IDispatch * object, DISPID dispid, VARIANTARG * args, UINT ccArgs, VARTYPE vt, VARIANT& result)
{
	HRESULT hr;

	DISPPARAMS params = {};
	params.rgvarg = args;
	params.cArgs = ccArgs;

	hr = object->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, &params, &result, NULL, NULL);

	if(SUCCEEDED(hr))
		hr = VariantChangeType(&result, &result, 0, vt);

	return hr;
}

HRESULT dispInvoke(IDispatch * object, DISPID dispid, const VARIANTARG& arg, VARTYPE vt, VARIANT& result)
{
	return dispInvoke(object, dispid, const_cast<VARIANTARG *>(&arg), 1, vt, result);
}

HRESULT dispInvoke(IDispatch * object, DISPID dispid, VARTYPE vt, VARIANT& result)
{
	return dispInvoke(object, dispid, NULL, 0, vt, result);
}

HRESULT dispInvoke(IDispatch * object, VARIANTARG * args, UINT ccArgs, VARTYPE vt, VARIANT& result)
{
	return dispInvoke(object, DISPID_VALUE, args, ccArgs, vt, result);
}

HRESULT dispInvoke(IDispatch * object, VARTYPE vt, VARIANT& result)
{
	return dispInvoke(object, DISPID_VALUE, NULL, 0, vt, result);
}

HRESULT dispInvoke(IDispatch * object, const VARIANTARG& arg, VARTYPE vt, VARIANT& result)
{
	return dispInvoke(object, DISPID_VALUE, const_cast<VARIANTARG *>(&arg), 1, vt, result);
}

HRESULT dispInvoke(IDispatch * object, const OLECHAR * name, VARIANTARG * args, UINT ccArgs, VARTYPE vt, VARIANT& result)
{
	HRESULT hr;

	DISPID dispid;
	hr = object->GetIDsOfNames(IID_NULL, const_cast<OLECHAR **>(&name), 1, 0, &dispid);

	if(SUCCEEDED(hr))
		hr = dispInvoke(object, dispid, args, ccArgs, vt, result);

	return hr;
}

HRESULT dispInvoke(IDispatch * object, const OLECHAR * name, const VARIANTARG& arg, VARTYPE vt, VARIANT& result)
{
	return dispInvoke(object, name, const_cast<VARIANTARG *>(&arg), 1, vt, result);
}

HRESULT dispInvoke(IDispatch * object, const OLECHAR * name, VARTYPE vt, VARIANT& result)
{
	return dispInvoke(object, name, NULL, 0, vt, result);
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, DISPID dispid, VARIANTARG * args, UINT ccArgs, VARTYPE vt)
{
	VARIANT ret;
	VariantInit(&ret);

	if(SUCCEEDED(hr))
		hr = dispInvoke(object, dispid, args, ccArgs, vt, ret);

	return ret;
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, const OLECHAR * name, VARIANTARG * args, UINT ccArgs, VARTYPE vt)
{
	VARIANT ret;
	VariantInit(&ret);

	if(SUCCEEDED(hr))
		hr = dispInvoke(object, name, args, ccArgs, vt, ret);

	return ret;
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, DISPID dispid, const VARIANTARG& arg, VARTYPE vt)
{
	return dispInvoke(hr, object, dispid, const_cast<VARIANTARG *>(&arg), 1, vt);
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, DISPID dispid, VARTYPE vt)
{
	return dispInvoke(hr, object, dispid, NULL, 0, vt);
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, VARIANTARG * args, UINT ccArgs, VARTYPE vt)
{
	return dispInvoke(hr, object, DISPID(DISPID_VALUE), args, ccArgs, vt);
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, VARTYPE vt)
{
	return dispInvoke(hr, object, DISPID(DISPID_VALUE), NULL, 0, vt);
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, const VARIANTARG& arg, VARTYPE vt)
{
	return dispInvoke(hr, object, DISPID(DISPID_VALUE), const_cast<VARIANTARG *>(&arg), 1, vt);
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, const OLECHAR * name, const VARIANTARG& arg, VARTYPE vt)
{
	return dispInvoke(hr, object, name, const_cast<VARIANTARG *>(&arg), 1, vt);
}

VARIANT dispInvoke(HRESULT& hr, IDispatch * object, const OLECHAR * name, VARTYPE vt)
{
	return dispInvoke(hr, object, name, NULL, 0, vt);
}

VARIANT oleString(HRESULT& hr, const OLECHAR * sz)
{
	VARIANT v;
	VariantInit(&v);

	if(SUCCEEDED(hr))
	{
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = SysAllocString(sz);

		if(!V_BSTR(&v))
			hr = E_OUTOFMEMORY;
	}

	if(FAILED(hr))
	{
		V_VT(&v) = VT_ERROR;
		V_ERROR(&v) = hr;
	}

	return v;
}

#endif

#if 1
namespace knuth_morris_pratt
{

namespace details
{

template<class Iter>
typename std::iterator_traits<Iter>::iterator_category iterator_category(const Iter&)
{
	return typename std::iterator_traits<Iter>::iterator_category();
}

template<typename TypeX, typename TypeY>
struct type_equals
{
	enum { value = 0 };
};

template<typename TypeX>
struct type_equals<TypeX, TypeX>
{
	enum { value = 1 };
};

template<class Iter, class diff_type>
void advance_substr(Iter& pos, Iter, diff_type, diff_type rel, const std::bidirectional_iterator_tag&)
{
	std::advance(pos, rel);
}

template<class Iter, class diff_type>
void advance_substr(Iter& pos, Iter begin, diff_type absol, diff_type rel, const std::forward_iterator_tag&)
{
	if(rel > 0)
		std::advance(pos, rel);
	else
	{
		pos = begin;
		std::advance(pos, absol);
	}
}

template<class Iter, class diff_type>
void advance_substr(Iter& pos, Iter begin, diff_type absol, diff_type rel)
{
	advance_substr(pos, begin, absol, rel, iterator_category(pos));
}

template<class T, size_t N>
struct fixed_table
{
public:
	typedef typename std::iterator_traits<T *>::difference_type value_type;
	typedef value_type * pointer;
	typedef value_type& reference;
	typedef const value_type * const_pointer;
	typedef const value_type& const_reference;
	typedef typename std::allocator<value_type>::difference_type difference_type;
	typedef typename std::allocator<value_type>::size_type size_type;

private:
	value_type m_table[(N)];

public:
	static pointer address(reference r) { return &r; }
	static const_pointer address(const_reference r) { return &r; }
	static size_type max_size() { return (N); }
	static void construct(pointer p, const T& v) { p->T(v); }
	static void destroy(pointer p) { p->~T(); }
	template<class Other> struct rebind { typedef fixed_table<Other, N> other; };

	template<class Other> pointer allocate(size_type n, const Other * hint)
	{
		assert(n <= (N));
		assert(hint == 0);
		return m_table;
	}

	pointer allocate(size_type n)
	{
		assert(n <= (N));
		return m_table;
	}

	void deallocate(pointer p, size_type n) const
	{
		assert(p == m_table);
		assert(n <= (N));
	}
};

void validate_range_iterator_category(const std::input_iterator_tag&) { }
void validate_substr_iterator_category(const std::forward_iterator_tag&) { }

template<class RangeIter, class SubstrIter>
RangeIter search_cornercase_neg(RangeIter, RangeIter end, SubstrIter)
{
	return end;
}

template<class RangeIter, class SubstrIter>
RangeIter search_cornercase0(RangeIter begin, RangeIter, SubstrIter)
{
	return begin;
}

template<class RangeIter, class SubstrIter>
RangeIter search_cornercase1(RangeIter begin, RangeIter end, SubstrIter substrBegin)
{
	RangeIter pos = std::find(begin, end, *substrBegin);

	if(type_equals<typename std::iterator_traits<RangeIter>::iterator_category, std::input_iterator_tag>::value && pos != end)
		++ pos;

	return pos;
}

template<class SubstrIter, class TableType, class TableSize>
void search_createtable_dynamic
(
	SubstrIter beginSubstr,
	typename std::iterator_traits<SubstrIter>::difference_type substrSize,
	TableType table,
	TableSize
)
{
	typedef typename std::iterator_traits<SubstrIter>::difference_type substr_diff_type;
	typedef typename TableType::pointer table_type;
	typedef TableSize table_size_type;

	substr_diff_type i(2);
	SubstrIter pi = beginSubstr;
	std::advance(pi, substr_diff_type(1));

	substr_diff_type j(0);
	SubstrIter pj = beginSubstr;
	std::advance(pj, substr_diff_type(0));

	while(i < substrSize)
	{
		if(*pi == *pj)
		{
			substr_diff_type new_j = j;
			++ new_j;

			table[static_cast<table_size_type>(i)] = new_j;

			++ i; ++ pi;
			++ j; ++ pj;
		}
		else if(j > 0)
		{
			substr_diff_type new_j = table[i];
			details::advance_substr(pj, beginSubstr, new_j, new_j - j);
			j = new_j;
		}
		else
		{
			table[i] = 0;
			++ i; ++ pi;
		}
	}
}

template<class D, class S, size_t N>
void search_createtable_static
(
	const S (& substr)[N],
	D (& table)[N]
)
{
	typedef std::ptrdiff_t substr_diff_type;

	table[0] = substr_diff_type(-1);
	table[1] = substr_diff_type(0);

	substr_diff_type pos = 2;
	substr_diff_type cnd = 0;

	while(pos < static_cast<substr_diff_type>(N))
	{
		if(substr[pos - 1] == substr[cnd])
		{
			table[pos] = cnd + 1;
			++ pos;
			++ cnd;
		}
		else if(cnd > 0)
			cnd = table[cnd];
		else
		{
			table[pos] = 0;
			++ pos;
		}
	}
}

template<class RangeIter, class SubstrIter, class Table, class TableSize>
RangeIter search
(
	const RangeIter& begin,
	const RangeIter& end,
	const SubstrIter beginSubstr,
	typename std::iterator_traits<SubstrIter>::difference_type substrSize,
	Table table,
	TableSize
)
{
	details::validate_range_iterator_category(details::iterator_category(begin));
	details::validate_substr_iterator_category(details::iterator_category(beginSubstr));

	typedef typename std::iterator_traits<SubstrIter>::difference_type substr_diff_type;
	typedef Table table_type;
	typedef TableSize table_size_type;

	// Find the next match
	substr_diff_type iSubstr(0); // i
	RangeIter pCurMatch = begin; // m
	RangeIter pCurElem = begin; // m + i
	SubstrIter pCurSubstrElem = beginSubstr; // i

	while(pCurElem != end)
	{
		if(*pCurSubstrElem == *pCurElem)
		{
			++ iSubstr; // ++ i

			if(iSubstr == substrSize)
			{
				// For input iterators, we return the end of the match
				if(type_equals<typename std::iterator_traits<RangeIter>::iterator_category, std::input_iterator_tag>::value)
					++ pCurElem; // return m + i + 1
				// For all other iterators, we return the beginning of the match
				else
					pCurElem = pCurMatch; // return m

				break;
			}

			++ pCurElem; // ++ i
			++ pCurSubstrElem; // ++ i
		}
		else
		{
			substr_diff_type table_iSubstr = table[static_cast<table_size_type>(iSubstr)]; // T[i]
			substr_diff_type iSubstr_backtracked = iSubstr - table_iSubstr; // i - T[i]
			assert(iSubstr_backtracked >= substr_diff_type(0));

			if(!type_equals<typename std::iterator_traits<RangeIter>::iterator_category, std::input_iterator_tag>::value)
				std::advance(pCurMatch, iSubstr_backtracked); // m = m + i - T[i]

			if(iSubstr > substr_diff_type(0)) // i > 0
			{
				iSubstr = table_iSubstr; // i = T[i]
				pCurSubstrElem = beginSubstr;
				std::advance(pCurSubstrElem, iSubstr); // i = T[i]
			}
			else
				std::advance(pCurElem, iSubstr_backtracked);
		}
	}

	return pCurElem;
}

};

template<class RangeIter, class SubstrIter, class AllocTable>
RangeIter search(RangeIter begin, RangeIter end, SubstrIter beginSubstr, SubstrIter endSubstr, AllocTable& allocTable)
{
	typename std::iterator_traits<SubstrIter>::difference_type substrSize = std::distance(beginSubstr, endSubstr);

	if(substrSize < 0)
		return knuth_morris_pratt::details::search_cornercase_neg(begin, end, beginSubstr);

	if(substrSize == 0)
		return knuth_morris_pratt::details::search_cornercase0(begin, end, beginSubstr);

	if(substrSize == 1)
		return knuth_morris_pratt::details::search_cornercase1(begin, end, beginSubstr);

	typedef typename AllocTable::size_type table_size;
	typedef typename AllocTable::pointer table_type;

	table_size tableSize = static_cast<table_size>(substrSize);
	table_type table = allocTable.allocate(tableSize);

	RangeIter pos = knuth_morris_pratt::details::search(begin, end, beginSubstr, substrSize, table, tableSize);

	allocTable.deallocate(table, tableSize);

	return pos;
}

template<class RangeIter, class SubstrIter>
RangeIter search(RangeIter begin, RangeIter end, SubstrIter beginSubstr, SubstrIter endSubstr)
{
	std::allocator<typename std::iterator_traits<SubstrIter>::difference_type> allocTable;
	return knuth_morris_pratt::search(begin, end, beginSubstr, endSubstr, allocTable);
}

template<class RangeIter, class SubstrElem, size_t N>
RangeIter search(RangeIter begin, RangeIter end, const SubstrElem (& substr)[(N)])
{
	if((N) == 0)
		return knuth_morris_pratt::details::search_cornercase0(begin, end, substr);

	if((N) == 1)
		return knuth_morris_pratt::details::search_cornercase1(begin, end, substr);

	if((N) * sizeof(SubstrElem) <= 4096)
	{
		ptrdiff_t table[N];
		knuth_morris_pratt::details::search_createtable_static(substr, table);
		return knuth_morris_pratt::details::search(begin, end, substr, (N), table, sizeof(table));
	}
	else
		return knuth_morris_pratt::search(begin, end, substr, substr + (N));
}

}
#endif

#if 1
template<class CharT, class Traits, class Alloc, size_t N, class ReplIter>
std::basic_string<CharT, Traits, Alloc> replace(const std::basic_string<CharT, Traits, Alloc>& str, const CharT (& substr)[(N)], ReplIter replBegin, ReplIter replEnd)
{
	if((N) == 0)
		return str;

	std::basic_string<CharT, Traits, Alloc> newStr;
	typename std::basic_string<CharT, Traits, Alloc>::const_iterator cur = str.begin();
	typename std::basic_string<CharT, Traits, Alloc>::const_iterator end = str.end();

	while(cur != end)
	{
		typename std::basic_string<CharT, Traits, Alloc>::const_iterator found = knuth_morris_pratt::search(cur, end, *reinterpret_cast<const CharT (*)[(N) - 1]>(&substr));

		newStr.append(cur, found);
		cur = found;

		if(found != end)
		{
			newStr.append(replBegin, replEnd);
			advance(cur, (N) - 1);
		}
	}

	return newStr;
}

template<class CharT, class Traits, class Alloc, size_t N, size_t M>
std::basic_string<CharT, Traits, Alloc> replace(const std::basic_string<CharT, Traits, Alloc>& str, const CharT (& substr)[(N)], const CharT (& repl)[(M)])
{
	return replace(str, substr, repl, repl + ((M) - 1));
}

template<class CharT, class Traits, class Alloc, size_t N, class Traits2, class Alloc2>
std::basic_string<CharT, Traits, Alloc> replace(const std::basic_string<CharT, Traits, Alloc>& str, const CharT (& substr)[(N)], const std::basic_string<CharT, Traits2, Alloc2>& repl)
{
	return replace(str, substr, repl.begin(), repl.end());
}

template<class CharT, class Traits, class Alloc, size_t M, class Traits2, class Alloc2>
std::basic_string<CharT, Traits, Alloc> replace(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string<CharT, Traits2, Alloc2>& substr, const CharT (& repl)[(M)])
{
	return replace(str, substr.begin(), substr.end(), repl, repl + ((M) - 1));
}

template<class CharT, class Traits, class Alloc, class Traits2, class Alloc2, class Traits3, class Alloc3>
std::basic_string<CharT, Traits, Alloc> replace(const std::basic_string<CharT, Traits, Alloc>& str, const std::basic_string<CharT, Traits2, Alloc2>& substr, const std::basic_string<CharT, Traits3, Alloc3>& repl)
{
	return replace(str, substr.begin(), substr.end(), repl.begin(), repl.end());
}

template<typename TypeX, typename TypeY>
struct type_equals
{
	enum { value = 0 };
};

template<typename TypeX>
struct type_equals<TypeX, TypeX>
{
	enum { value = 1 };
};

template<class InIter, class Dist>
bool advance_between_impl(InIter& cur, const InIter& begin, const InIter& end, Dist off, const std::input_iterator_tag&)
{
	if(Dist(0) < off)
		for(; off != Dist(0) && cur != end; -- off)
			++ cur;

	return off == Dist(0);
}

template<class InIter, class Dist>
bool advance_between_impl(InIter& cur, const InIter& begin, const InIter& end, Dist off, const std::bidirectional_iterator_tag&)
{
	if(Dist(0) < off)
		for(; off != Dist(0) && cur != end; -- off)
			++ cur;
	else
		for(; off != Dist(0) && cur != begin; ++ off)
			++ cur;

	return off == Dist(0);
}

template<class InIter, class Dist>
bool advance_between_impl(InIter& cur, const InIter& begin, const InIter& end, Dist off, const std::random_access_iterator_tag&)
{
	bool ret;

	if(Dist(0) < off)
	{
		Dist max = distance(cur, end);
		ret = off == max || off < max;
	}
	else
	{
		Dist max = distance(cur, end);
		ret = abs(off) == max || abs(off) < max;
	}

	if(ret)
		advance(cur, off);

	return ret;
}

template<class InIter, class Dist>
bool advance_between(InIter& cur, const InIter& begin, const InIter& end, Dist off)
{
	return advance_between_impl(cur, begin, end, off, typename std::iterator_traits<InIter>::iterator_category());
}

template<class InIter, class Elem, class Traits, class InIterTag>
class range_streambuf_base: public std::basic_streambuf<Elem, Traits>
{
public:
	typedef typename std::basic_streambuf<Elem, Traits>::int_type int_type;
	typedef typename std::basic_streambuf<Elem, Traits>::pos_type pos_type;
	typedef typename std::basic_streambuf<Elem, Traits>::off_type off_type;

protected:
	InIter m_cur;
	InIter m_end;
	int_type m_ungetc;
	int_type m_putbackc;

	int_type get_char()
	{
		int_type c = m_ungetc;

		if(c == Traits::eof())
		{
			if(m_cur != m_end)
			{
				c = *m_cur;
				++ m_cur;
			}
		}
		else
			m_ungetc = Traits::eof();

		return c;
	}

	int_type unget_char(int_type c)
	{
		if(c == Traits::eof())
			return c;

		if(m_ungetc == Traits::eof())
			m_ungetc = c;
		else
			return Traits::eof();

		return c;
	}

protected:
	range_streambuf_base(const InIter& begin, const InIter& end):
		m_cur(begin),
		m_end(end),
		m_ungetc(Traits::eof()),
		m_putbackc(Traits::eof())
	{
	}

public:
	virtual int_type pbackfail(int_type c)
	{
		int_type ret;

		if(Traits::eq_int_type(c, Traits::eof()))
		{
			if(!Traits::eq_int_type(m_putbackc, Traits::eof()))
				ret = unget_char(m_putbackc);
			else
				ret = Traits::eof();
		}
		else
			ret = unget_char(c);

		m_putbackc = Traits::eof();
		return ret;
	}

	virtual std::streamsize showmanyc() const
	{
		return std::streamsize(-1);
	}

	virtual int_type underflow()
	{
		return unget_char(get_char());
	}

	virtual int_type uflow()
	{
		m_putbackc = get_char();
		return m_putbackc;
	}

	virtual std::streamsize xsgetn(Elem * p, std::streamsize n)
	{
		return _Xsgetn_s(p, std::numeric_limits<size_t>::max(), n);
	}

	virtual std::streamsize _Xsgetn_s(Elem * p, size_t cb, std::streamsize n)
	{
		std::streamsize i;
		size_t ib;

		for(i = 0, ib = 0; i < n && ib < cb; ++ i, ++ ib, ++ p)
		{
			int_type c = get_char();

			if(c == Traits::eof())
				break;

			*p = Traits::to_char_type(c);
		}

		if(i > 0)
			m_putbackc = p[-1];
		else
			m_putbackc = Traits::eof();

		return i;
	}

	virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode)
	{
		if(((off == 0 && dir == std::ios_base::end) || (off >= 0 && dir == std::ios_base::cur)) && !(mode & std::ios_base::out))
		{
			m_ungetc = Traits::eof();

			if(dir == std::ios_base::end)
				m_cur = m_end;

			advance_between(m_cur, m_cur, m_end, off);
		}

		return pos_type(-1);
	}
};

template<class InIter, class Elem, class Traits>
class range_streambuf_base<InIter, Elem, Traits, std::forward_iterator_tag>:
	public range_streambuf_base<InIter, Elem, Traits, std::input_iterator_tag>
{
public:
	typedef typename range_streambuf_base<InIter, Elem, Traits, std::input_iterator_tag>::int_type int_type;
	typedef typename range_streambuf_base<InIter, Elem, Traits, std::input_iterator_tag>::pos_type pos_type;
	typedef typename range_streambuf_base<InIter, Elem, Traits, std::input_iterator_tag>::off_type off_type;

private:
	typedef range_streambuf_base<InIter, Elem, Traits, std::input_iterator_tag> super;
	InIter m_begin;

protected:
	range_streambuf_base(const InIter& begin, const InIter& end):
		m_begin(begin),
		range_streambuf_base<InIter, Elem, Traits, std::input_iterator_tag>(begin, end)
	{
	}

public:
	virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode)
	{
		if(!(mode & std::ios_base::out) && off >= 0)
		{
			if(dir == std::ios_base::beg)
				super::m_cur = m_begin;
			else if(dir != std::ios_base::cur)
				super::m_cur = super::m_end;

			advance_between(super::m_cur, m_begin, super::m_end, off);
		}

		return pos_type(-1);
	}
};

template<class InIter, class Elem, class Traits>
class range_streambuf_base<InIter, Elem, Traits, std::bidirectional_iterator_tag>: public std::basic_streambuf<Elem, Traits>
{
public:
	typedef typename std::basic_streambuf<Elem, Traits>::int_type int_type;
	typedef typename std::basic_streambuf<Elem, Traits>::pos_type pos_type;
	typedef typename std::basic_streambuf<Elem, Traits>::off_type off_type;

protected:
	InIter m_begin;
	InIter m_cur;
	InIter m_end;

protected:
	range_streambuf_base(const InIter& begin, const InIter& end):
		m_begin(begin),
		m_cur(begin),
		m_end(end)
	{
	}

public:
	virtual int_type pbackfail(int_type c)
	{
		int_type ret = Traits::eof();

		if(m_cur != m_begin)
		{
			InIter prev = m_cur;
			-- prev;

			if(Traits::eq_int_type(c, Traits::eof()) || Traits::eq_int_type(c, Traits::to_int_type(*prev)))
			{
				-- m_cur;
				ret = Traits::to_int_type(*m_cur);
			}
		}

		return ret;
	}

	virtual std::streamsize showmanyc() const
	{
		return std::streamsize(-1);
	}

	virtual int_type underflow()
	{
		if(m_cur != m_end)
			return *m_cur;
		else
			return Traits::eof();
	}

	virtual int_type uflow()
	{
		int_type c = Traits::eof();

		if(m_cur != m_end)
		{
			c = *m_cur;
			++ m_cur;
		}

		return c;
	}

	virtual std::streamsize xsgetn(Elem * p, std::streamsize n)
	{
		return _Xsgetn_s(p, std::numeric_limits<size_t>::max(), n);
	}

	virtual std::streamsize _Xsgetn_s(Elem * p, size_t cb, std::streamsize n)
	{
		std::streamsize i;
		size_t ib;

		for(i = 0, ib = 0; i < n && ib < cb && m_cur != m_end; ++ i, ++ ib, ++ p, ++ m_cur)
			*p = Traits::to_char_type(*m_cur);

		return i;
	}

	virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode)
	{
		if(mode & std::ios_base::out)
			return pos_type(-1);

		if(dir == std::ios_base::beg)
			m_cur = m_begin;
		else if(dir != std::ios_base::cur)
			m_cur = m_end;

		if(!advance_between(m_cur, m_begin, m_end, off))
			return pos_type(-1);

		return m_cur - m_begin;
	}
};

template<class InIter, class Elem, class Traits>
class range_streambuf_base<InIter, Elem, Traits, std::random_access_iterator_tag>:
	public range_streambuf_base<InIter, Elem, Traits, std::bidirectional_iterator_tag>
{
private:
	typedef range_streambuf_base<InIter, Elem, Traits, std::bidirectional_iterator_tag> super;

protected:
	range_streambuf_base(const InIter& begin, const InIter& end):
		range_streambuf_base<InIter, Elem, Traits, std::bidirectional_iterator_tag>(begin, end)
	{
	}

public:
	virtual std::streamsize showmanyc() const
	{
		return distance(super::m_cur, super::m_end);
	}

	virtual std::streamsize _Xsgetn_s(Elem * p, size_t cb, std::streamsize n)
	{
		std::streamsize maxn = showmanyc();

		if(n > maxn)
			n = maxn;

		if(cb > static_cast<size_t>(n))
			cb = static_cast<size_t>(n);

		std::copy(super::m_cur, super::m_cur + cb, p);
		return cb;
	}
};

template<class InIter, class Elem = typename std::iterator_traits<InIter>::value_type, class Traits = typename std::char_traits<Elem> >
class range_streambuf:
	public range_streambuf_base<InIter, Elem, Traits, typename std::iterator_traits<InIter>::iterator_category>
{
public:
	range_streambuf(const InIter& begin, const InIter& end):
		range_streambuf_base<InIter, Elem, Traits, typename std::iterator_traits<InIter>::iterator_category>(begin, end) { }
};

#endif

struct version_tuple
{
private:
	unsigned short m_fields[4];

public:
	version_tuple(): m_fields() {}

	version_tuple(unsigned short major, unsigned short minor, unsigned short build_major = 0, unsigned short build_minor = 0)
	{
		m_fields[0] = major;
		m_fields[1] = minor;
		m_fields[2] = build_major;
		m_fields[3] = build_minor;
	}

	version_tuple(const version_tuple& That)
	{
		m_fields[0] = That.m_fields[0];
		m_fields[1] = That.m_fields[1];
		m_fields[2] = That.m_fields[2];
		m_fields[3] = That.m_fields[3];
	}

	const version_tuple& operator=(const version_tuple& That)
	{
		m_fields[0] = That.m_fields[0];
		m_fields[1] = That.m_fields[1];
		m_fields[2] = That.m_fields[2];
		m_fields[3] = That.m_fields[3];
		return *this;
	}

	unsigned short get_major() const
	{
		return m_fields[0];
	}

	unsigned short get_minor() const
	{
		return m_fields[1];
	}

	unsigned short get_build_major() const
	{
		return m_fields[2];
	}

	unsigned short get_build_minor() const
	{
		return m_fields[3];
	}

	bool operator==(const version_tuple& That) const
	{
		return memcmp(this->m_fields, That.m_fields, sizeof(this->m_fields)) == 0;
	}

	bool operator<(const version_tuple& That) const
	{
		return std::lexicographical_compare
		(
			&(this->m_fields[0]) + 0,
			&(this->m_fields[0]) + ARRAYSIZE(this->m_fields),
			&(That.m_fields[0]) + 0,
			&(That.m_fields[0]) + ARRAYSIZE(That.m_fields)
		);
	}

	bool operator<=(const version_tuple& That) const
	{
		return (*this == That) || (*this < That);
	}

	bool operator>(const version_tuple& That) const
	{
		return !(*this <= That);
	}

	bool operator>=(const version_tuple& That) const
	{
		return !(*this < That);
	}
};

template<class Elem, class Tr> std::basic_ostream<Elem, Tr>& operator<<(std::basic_ostream<Elem, Tr>& ostr, const version_tuple& v)
{
	std::ios_base::fmtflags flags = ostr.flags();
	ostr.flags(std::ios_base::dec);
	ostr << v.get_major();
	ostr << Tr::to_char_type('.');
	ostr << v.get_minor();
	ostr << Tr::to_char_type('.');
	ostr << v.get_build_major();
	ostr << Tr::to_char_type('.');
	ostr << v.get_build_minor();
	ostr.flags(flags);
	return ostr;
}

template<class Elem, class Tr> std::basic_istream<Elem, Tr>& operator>>(std::basic_istream<Elem, Tr>& istr, version_tuple& v)
{
	const std::ctype<Elem>& ct = std::use_facet<std::ctype<Elem> >(istr.getloc());
	unsigned short a = 0, b = 0, c = 0, d = 0;
	Elem ch;

	std::ios_base::fmtflags flags = istr.setf(std::ios_base::dec);

	if(istr >> a)
	{
		istr >> ch;

		if(ch == ct.widen('.'))
		{
			if(istr >> b)
			{
				istr >> ch;

				if(ch == ct.widen('.'))
				{
					if(istr >> c)
					{
						istr >> ch;

						if(ch == ct.widen('.'))
							istr >> d;
						else
							istr.putback(ch);
					}
				}
				else
					istr.putback(ch);

				if(istr)
				{
					v = version_tuple(a, b, c, d);
					istr.flags(flags);
					return istr;
				}
			}
		}
		else
			istr.putback(ch);
	}

	istr.flags(flags);
	istr.setstate(std::ios_base::failbit);
	return istr;
}

struct cl_version: public version_tuple
{
private:
	bool m_optimizing;

public:
	cl_version(): m_optimizing() {}

	cl_version(const version_tuple& version_number, bool optimizing = false):
		version_tuple(version_number), m_optimizing(optimizing) {}

	cl_version
	(
		unsigned short major,
		unsigned short minor,
		unsigned short build_major = 0,
		unsigned short build_minor = 0,
		bool optimizing = false,
		bool analyze = false
	):
		version_tuple(major, minor, build_major, build_minor), m_optimizing(optimizing) {}

	cl_version(const cl_version& That): version_tuple(That), m_optimizing(That.m_optimizing) {}

	const cl_version& operator=(const cl_version& That)
	{
		version_tuple::operator=(That);
		m_optimizing = That.m_optimizing;
		return *this;
	}

	bool is_optimizing() const
	{
		return m_optimizing;
	}

	bool operator==(const cl_version& That) const
	{
		return this->m_optimizing == That.m_optimizing && this->version_tuple::operator==(That);
	}

	bool operator<(const cl_version& That) const
	{
		return (!this->m_optimizing && That.m_optimizing) || this->version_tuple::operator<(That);
	}

	bool operator<=(const cl_version& That) const
	{
		return (*this == That) || (*this < That);
	}

	bool operator>(const cl_version& That) const
	{
		return !(*this <= That);
	}

	bool operator>=(const cl_version& That) const
	{
		return !(*this < That);
	}

	operator bool() const
	{
		return get_major() || get_minor() || get_build_major() || get_build_minor();
	}
};

struct link_version: public version_tuple
{
public:
	link_version() {}

	link_version(const version_tuple& version_number): version_tuple(version_number) {}

	link_version
	(
		unsigned short major,
		unsigned short minor,
		unsigned short build_major = 0,
		unsigned short build_minor = 0
	):
		version_tuple(major, minor, build_major, build_minor) {}

	link_version(const link_version& That): version_tuple(That) {}

	const link_version& operator=(const link_version& That)
	{
		version_tuple::operator=(That);
		return *this;
	}

	bool operator==(const cl_version& That) const
	{
		return this->version_tuple::operator==(That);
	}

	bool operator<(const cl_version& That) const
	{
		return this->version_tuple::operator<(That);
	}

	bool operator<=(const cl_version& That) const
	{
		return (*this == That) || (*this < That);
	}

	bool operator>(const cl_version& That) const
	{
		return !(*this <= That);
	}

	bool operator>=(const cl_version& That) const
	{
		return !(*this < That);
	}

	operator bool() const
	{
		return get_major() || get_minor() || get_build_major() || get_build_minor();
	}
};

template<typename T>
struct type_holder
{
	typedef T type;
};

template<class Elem, class Traits, class Ax>
std::basic_string<Elem, Traits, Ax> get_env(const Elem * name, const type_holder<Traits>&, const type_holder<Ax>&)
{
	DWORD cchValue = 0;
	std::vector<Elem, Ax> value;

	for(;;)
	{
		value.resize(cchValue);

		if(type_equals<Elem, CHAR>::value)
			cchValue = GetEnvironmentVariableA(reinterpret_cast<const CHAR *>(name), reinterpret_cast<CHAR *>(cchValue ? &(value[0]) : NULL), cchValue);
		else if(type_equals<Elem, WCHAR>::value)
			cchValue = GetEnvironmentVariableW(reinterpret_cast<const WCHAR *>(name), reinterpret_cast<WCHAR *>(cchValue ? &(value[0]) : NULL), cchValue);

		if(cchValue == 0)
			return std::basic_string<Elem, Traits, Ax>();

		if(cchValue <= value.size())
			break;
	}

	value.resize(cchValue);
	return std::basic_string<Elem, Traits, Ax>(value.begin(), value.end());
}

template<class Traits, class Ax, class Elem>
std::basic_string<Elem, Traits, Ax> get_env(const Elem * name)
{
	return get_env(name, type_holder<Traits>(), type_holder<Ax>());
}

template<class Traits, class Elem>
std::basic_string<Elem, Traits> get_env(const Elem * name)
{
	return get_env(name, type_holder<Traits>(), type_holder<typename std::basic_string<Elem>::allocator_type>());
}

template<class Elem>
std::basic_string<Elem> get_env(const Elem * name)
{
	return get_env(name, type_holder<typename std::basic_string<Elem>::traits_type>(), type_holder<typename std::basic_string<Elem>::allocator_type>());
}

template<class Elem>
void set_env(const Elem * name, const Elem * value)
{
	if(type_equals<Elem, CHAR>::value)
		SetEnvironmentVariableA(reinterpret_cast<const CHAR *>(name), reinterpret_cast<const CHAR *>(value));
	else if(type_equals<Elem, WCHAR>::value)
		SetEnvironmentVariableW(reinterpret_cast<const WCHAR *>(name), reinterpret_cast<const WCHAR *>(value));
}

template<class CharT>
FILE * pipe_open(const CharT * commandLine, const CharT * mode)
{
	if(type_equals<CharT, char>::value)
		return _popen(reinterpret_cast<const char *>(commandLine), reinterpret_cast<const char *>(mode));
	else if(type_equals<CharT, wchar_t>::value)
		return _wpopen(reinterpret_cast<const wchar_t *>(commandLine), reinterpret_cast<const wchar_t *>(mode));
	else
		return NULL;
}

template<class CharT, class CharT2>
FILE * pipe_open_override_path(const CharT * commandLine, const CharT * mode, const CharT2 * newPath)
{
	FILE * pipe = NULL;
	const CharT2 path[] = { 'P', 'A', 'T', 'H', 0 };

	std::basic_string<CharT2> oldPath;

	if(newPath)
	{
		oldPath = get_env(path);
		set_env(path, newPath);
	}

	try
	{
		pipe = pipe_open(commandLine, mode);
	}
	catch(...)
	{
		if(pipe != NULL)
			fclose(pipe);

		if(newPath)
			set_env(path, oldPath.c_str());

		throw;
	}

	if(newPath)
		set_env(path, oldPath.c_str());

	return pipe;
}

template<class InIter, class IsSep>
std::pair<InIter, InIter> tokenize(InIter begin, InIter end, IsSep isSeparator)
{
	InIter beginToken = std::find_if(begin, end, std::not1(isSeparator));
	InIter endToken = std::find_if(beginToken, end, isSeparator);
	return std::make_pair(beginToken, endToken);
}

std::locale clocale("C");
const std::ctype<char>& cctype = std::use_facet<std::ctype<char> >(clocale);

template<const std::ctype_base::mask Mask>
struct is_ctype_l: public std::binary_function<std::locale, char, bool>
{
public:
	result_type operator()(const first_argument_type& locale, second_argument_type c) const
	{
		return std::use_facet<std::ctype<char> >(locale).is(Mask, c);
	}
};

typedef is_ctype_l<std::ctype_base::space> is_space_l;

template<const std::ctype_base::mask Mask>
struct is_ctype: public std::unary_function<char, bool>
{
private:
	const std::locale& m_locale;
	is_ctype_l<Mask> m_is_ctype_l;

public:
	is_ctype(const std::locale& locale): m_locale(locale) {}

	result_type operator()(argument_type c) const
	{
		return m_is_ctype_l(m_locale, c);
	}
};

typedef is_ctype<std::ctype_base::space> is_space;

template<class Elem, size_t N>
std::pair<const Elem *, const Elem *> string_literal_token(const Elem (& lit)[N])
{
	return std::make_pair(lit, lit + ((N) - 1));
}

template<class Iter, class Iter2>
bool token_equals(const std::pair<Iter, Iter>& tokenX, const std::pair<Iter2, Iter2>& tokenY)
{
	return std::distance(tokenX.first, tokenX.second) == std::distance(tokenY.first, tokenY.second) && std::equal(tokenX.first, tokenX.second, tokenY.first);
}

std::pair<const char *, const char *> GetClArchToken(const std::string& arch)
{
	if(arch == "i386")
		return string_literal_token("80x86");
	else if(arch == "amd64")
		return string_literal_token("x64");
	else if(arch == "arm")
		return string_literal_token("ARM");
	else
		return string_literal_token("");
}

template<class Elem>
cl_version CheckClVersion(const std::string& arch, const Elem * pathOverride)
{
	stdio_filebuf clVersionInfoBuf(pipe_open_override_path(_T("cl /nologo- 2>&1 >nul <nul"), _T("rt"), pathOverride));
	std::istream clVersionInfo(&clVersionInfoBuf);

	std::pair<const char *, const char *> archToken = GetClArchToken(arch);

	version_tuple clVersionNumber;
	bool clOptimizing = false;
	bool clTargetArch = false;

	std::string clVersionLine;
	getline(clVersionInfo, clVersionLine);

	for
	(
		std::pair<std::string::iterator, std::string::iterator> token = tokenize(clVersionLine.begin(), clVersionLine.end(), is_space(clocale));
		token.first != token.second;
		token = tokenize(token.second, clVersionLine.end(), is_space(clocale))
	)
	{
		// Is the compiler optimizing?
		if(token_equals(token, string_literal_token("Optimizing")))
		{
			clOptimizing = true;
			continue;
		}

		// Is this the version number?
		range_streambuf<std::string::const_iterator> tokenBuf(token.first, token.second);
		std::istream tokenStream(&tokenBuf);

		if(tokenStream >> clVersionNumber)
			continue;

		// Does the compiler support the target architecture?
		if(token_equals(token, archToken))
		{
			clTargetArch = true;
			continue;
		}
	}

	if(clTargetArch)
		return cl_version(clVersionNumber, clOptimizing);
	else
		return cl_version();
}

cl_version CheckClVersion(const std::string& arch)
{
	return CheckClVersion(arch, (const char *)0);
}

std::pair<const _TCHAR *, const _TCHAR *> GetLinkArchToken(const std::string& arch)
{
	if(arch == "i386")
		return string_literal_token(_T("X86"));
	else if(arch == "amd64")
		return string_literal_token(_T("X64"));
	else if(arch == "arm")
		return string_literal_token(_T("ARM"));
	else
		return string_literal_token(_T(""));
}

template<class Elem>
link_version CheckLinkVersion(const std::string& arch, const Elem * pathOverride)
{
	std::pair<const _TCHAR *, const _TCHAR *> archToken = GetLinkArchToken(arch);

	std::basic_string<_TCHAR> linkCmdLine;
	linkCmdLine.append(_T("link /nologo- /machine:\""));
	linkCmdLine.append(archToken.first, archToken.second);
	linkCmdLine.append(_T("\" <nul 2>nul"));

	stdio_filebuf linkOutputBuf(pipe_open_override_path(linkCmdLine.c_str(), _T("rt"), pathOverride));
	std::istream linkOutput(&linkOutputBuf);

	link_version linkVersion;

	std::string linkOutputLine;

	if(getline(linkOutput, linkOutputLine))
	{
		for
		(
			std::pair<std::string::iterator, std::string::iterator> token = tokenize(linkOutputLine.begin(), linkOutputLine.end(), is_space(clocale));
			token.first != token.second;
			token = tokenize(token.second, linkOutputLine.end(), is_space(clocale))
		)
		{
			range_streambuf<std::string::const_iterator> tokenBuf(token.first, token.second);
			std::istream tokenStream(&tokenBuf);

			version_tuple linkVersionNumber;

			if(tokenStream >> linkVersionNumber)
			{
				linkVersion = linkVersionNumber;
				break;
			}
		}
	}

	if(linkVersion)
	{
		bool linkArchCheckFail = false;

		while(!linkArchCheckFail && getline(linkOutput, linkOutputLine))
		{
			linkArchCheckFail =
				knuth_morris_pratt::search(linkOutputLine.begin(), linkOutputLine.end(), "LNK4012") != linkOutputLine.end() ||
				knuth_morris_pratt::search(linkOutputLine.begin(), linkOutputLine.end(), "LNK1146") != linkOutputLine.end();
		}

		if(linkArchCheckFail)
			linkVersion = link_version();
	}

	return linkVersion;
}

link_version CheckLinkVersion(const std::string& arch)
{
	return CheckLinkVersion(arch, (const char *)0);
}

template<class Elem, class Traits, class Ax>
void CharsToString(HRESULT& hr, std::basic_string<Elem, Traits, Ax>& str, const Elem * beginChars, const Elem * endChars)
{
	if(SUCCEEDED(hr))
		str = std::basic_string<Elem, Traits, Ax>(beginChars, endChars);
}

template<class Traits, class Ax>
void CharsToString(HRESULT& hr, std::basic_string<CHAR, Traits, Ax>& str, const WCHAR * beginChars, const WCHAR * endChars)
{
	if(!SUCCEEDED(hr))
		return;

	UINT cchChars = endChars - beginChars;
	int cch = WideCharToMultiByte(CP_ACP, 0, beginChars, cchChars, NULL, 0, NULL, NULL);

	if(cch <= 0)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return;
	}

	CHAR * psz = new CHAR[cch];

	if(psz == NULL)
	{
		hr = E_OUTOFMEMORY;
		return;
	}

	cch = WideCharToMultiByte(CP_ACP, 0, beginChars, cchChars, psz, cch, NULL, NULL);

	if(cch <= 0)
		hr = HRESULT_FROM_WIN32(GetLastError());
	else
		str = std::basic_string<CHAR, Traits, Ax>(psz, psz + cch);

	delete[] psz;
}

template<class Traits, class Ax>
void CharsToString(HRESULT& hr, std::basic_string<WCHAR, Traits, Ax>& str, const CHAR * beginChars, const CHAR * endChars)
{
	if(!SUCCEEDED(hr))
		return;

	UINT cchChars = endChars - beginChars;
	int cch = MultiByteToWideChar(CP_ACP, 0, beginChars, cchChars, NULL, 0);

	if(cch <= 0)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		return;
	}

	WCHAR * psz = new WCHAR[cch];

	if(psz == NULL)
	{
		hr = E_OUTOFMEMORY;
		return;
	}

	cch = MultiByteToWideChar(CP_ACP, 0, beginChars, cchChars, psz, cch);

	if(cch <= 0)
		hr = HRESULT_FROM_WIN32(GetLastError());
	else
		str = std::basic_string<WCHAR, Traits, Ax>(psz, psz + cch);

	delete[] psz;
}

template<class Elem, class Traits, class Ax>
void BSTRToString(HRESULT& hr, std::basic_string<Elem, Traits, Ax>& str, BSTR bstr)
{
	CharsToString(hr, str, bstr, bstr + SysStringLen(bstr));
}

std::vector<std::string> SplitPath(const std::string& strPath)
{
	std::vector<std::string> path;
	std::string::const_iterator cur = strPath.begin();
	std::string::const_iterator end = strPath.end();

	while(!(cur == end))
	{
		std::string::const_iterator itemBegin = cur;
		std::string::const_iterator itemEnd = std::find(cur, end, ';');

		if(!(itemBegin == itemEnd))
			path.push_back(std::string(itemBegin, itemEnd));

		cur = itemEnd;

		if(!(cur == end))
			++ cur;
	}

	return path;
}

std::vector<std::string> ParsePlatformPath(HRESULT& hr, IDispatch * pPlatform, BSTR bstrPath)
{
	std::vector<std::string> path;

	if(FAILED(hr))
		return path;

	VARIANT varPath;
	VariantInit(&varPath);
	V_VT(&varPath) = VT_BSTR;
	V_BSTR(&varPath) = bstrPath;

	VARIANT varRet = dispInvoke(hr, pPlatform, OLESTR("Evaluate"), varPath, VT_BSTR);

	std::string strPath;
	BSTRToString(hr, strPath, V_BSTR(&varRet));

	VariantClear(&varRet);

	path = SplitPath(strPath);
	return path;
}

VARIANT GetDTEProjectEngine(HRESULT& hr, IDispatch * pDTE)
{
	VARIANT varVCProjects = oleString(hr, OLESTR("VCProjects"));
	VARIANT varVCProjectEngine = oleString(hr, OLESTR("VCProjectEngine"));
	VARIANT varProjects = dispInvoke(hr, pDTE, OLESTR("GetObject"), varVCProjects, VT_DISPATCH);
	VARIANT varProjectsProperties = dispPropGet(hr, V_DISPATCH(&varProjects), OLESTR("Properties"), VT_DISPATCH);
	VARIANT varProjectEngineProperty = dispInvoke(hr, V_DISPATCH(&varProjectsProperties), varVCProjectEngine, VT_DISPATCH);
	VARIANT varRet = dispPropGet(hr, V_DISPATCH(&varProjectEngineProperty), OLESTR("Object"), VT_DISPATCH);
	VariantClear(&varProjectEngineProperty);
	VariantClear(&varProjectsProperties);
	VariantClear(&varProjects);
	VariantClear(&varVCProjectEngine);
	VariantClear(&varVCProjects);
	return varRet;
}

VARIANT GetDTEPlatformPath(HRESULT& hr, IDispatch * pDTE, BSTR bstrPlatformName, const OLECHAR * pszPathName)
{
	VARIANTARG argsProperties[2] = { oleString(hr, OLESTR("VCDirectories")), oleString(hr, OLESTR("Projects")) };

	VARIANT varProperties;
	VariantInit(&varProperties);

	if(SUCCEEDED(hr))
		hr = dispPropGet(pDTE, OLESTR("Properties"), argsProperties, ARRAYSIZE(argsProperties), VT_DISPATCH, varProperties);

	VARIANT varPathName = oleString(hr, pszPathName);
	VARIANT varPath = dispInvoke(hr, V_DISPATCH(&varProperties), varPathName, VT_BSTR);

	VARIANT varRet;
	VariantInit(&varRet);

	if(SUCCEEDED(hr))
	{
		const OLECHAR * pCur = V_BSTR(&varPath);
		const OLECHAR * pEnd = V_BSTR(&varPath) + SysStringLen(V_BSTR(&varPath));

		while(!(pCur == pEnd))
		{
			const OLECHAR * pBeginName = pCur;
			const OLECHAR * pEndName = std::find(pBeginName, pEnd, OLESTR('|'));
			const OLECHAR * pBeginValue = pEndName == pEnd ? pEndName : pEndName + 1;
			const OLECHAR * pEndValue = std::find(pBeginValue, pEnd, OLESTR('|'));

			pCur = pEndValue;

			if(!(pCur == pEnd))
				++ pCur;

			BSTR bstrName = SysAllocStringLen(pBeginName, pEndName - pBeginName);

			if(bstrName)
			{
				hr = VarBstrCmp(bstrName, bstrPlatformName, 0, NORM_IGNORECASE);

				if(hr == VARCMP_EQ)
				{
					BSTR bstrRet = SysAllocStringLen(pBeginValue, pEndValue - pBeginValue);

					if(bstrRet)
					{
						V_VT(&varRet) = VT_BSTR;
						V_BSTR(&varRet) = bstrRet;
					}
					else
						hr = E_OUTOFMEMORY;
				}

				SysFreeString(bstrName);

				if(FAILED(hr) || hr == VARCMP_EQ)
					break;
			}
			else
			{
				hr = E_OUTOFMEMORY;
				break;
			}
		}
	}

	VariantClear(&varPath);
	VariantClear(&varPathName);
	VariantClear(&varProperties);
	VariantClear(&argsProperties[1]);
	VariantClear(&argsProperties[0]);

	if(SUCCEEDED(hr) && V_VT(&varRet) != VT_BSTR)
	{
		VariantClear(&varRet);
		varRet = oleString(hr, OLESTR(""));
	}

	return varRet;
}

cl_version msCompilerVersion;
std::string msCompilerPath;
std::vector<std::string> msCompilerIncludeDirs; // TODO: use this
std::string msCompilerSource; // TODO: use this

link_version msLinkerVersion;
std::string msLinkerPath;
std::vector<std::string> msLinkerLibDirs; // TODO: use this
std::string msLinkerSource; // TODO: use this

void ProcessVCPlatform(HRESULT& hr, const std::string& arch, bool wantCompiler, bool wantLinker, IDispatch * pVCProjectEngine, IDispatch * pPlatform, BSTR bstrPath, BSTR bstrInclude, BSTR bstrLib)
{
	if(FAILED(hr))
		return;

	std::basic_string<OLECHAR> strPath = replace(std::basic_string<OLECHAR>(bstrPath, SysStringLen(bstrPath)), OLESTR("$(PATH)"), OLESTR("%PATH%"));

	VARIANT varPath;
	VariantInit(&varPath);
	V_VT(&varPath) = VT_BSTR;
	V_BSTR(&varPath) = SysAllocStringLen(strPath.c_str(), strPath.size());

	if(!V_BSTR(&varPath))
		hr = E_OUTOFMEMORY;

	VARIANT varRet = dispInvoke(hr, pPlatform, OLESTR("Evaluate"), varPath, VT_BSTR);

	if(SUCCEEDED(hr))
	{
		std::basic_string<OLECHAR> strPathOverride = replace
		(
			std::basic_string<OLECHAR>(V_BSTR(&varRet), V_BSTR(&varRet) + SysStringLen(V_BSTR(&varRet))),
			OLESTR("%PATH%"),
			get_env(OLESTR("PATH"))
		);

		cl_version clVersion;
		link_version linkVersion;

		if(wantCompiler)
			clVersion = CheckClVersion(arch, strPathOverride.c_str());

		if(wantLinker)
			linkVersion = CheckLinkVersion(arch, strPathOverride.c_str());

		// TODO: need a way to choose the desired tools and which version
		// BUGBUG: for now, only set the new version if both tools are the highest version yet
		if((!wantCompiler || (clVersion && clVersion > msCompilerVersion)) && (!wantLinker || (linkVersion && linkVersion > msLinkerVersion)))
		{
			std::string strPath;
			BSTRToString(hr, strPath, V_BSTR(&varRet));
			strPath = replace(strPath, "%PATH%", "$(PATH)");

			std::vector<std::string> include = ParsePlatformPath(hr, pPlatform, bstrInclude);
			std::vector<std::string> lib = ParsePlatformPath(hr, pPlatform, bstrLib);

			if(SUCCEEDED(hr))
			{
				if(wantCompiler)
				{
					msCompilerVersion = clVersion;
					msCompilerPath = strPath;
					msCompilerIncludeDirs = include;
					//msCompilerSource; // TODO: fill this in
				}

				if(wantLinker)
				{
					msLinkerVersion = linkVersion;
					msLinkerPath = strPath;
					msLinkerLibDirs = lib;
					//msLinkerSource; // TODO: fill this in
				}
			}
		}
	}

	VariantClear(&varPath);
	VariantClear(&varRet);
}

bool IsVCPlatformSupported(HRESULT& hr, const std::string& arch, IDispatch * pPlatform)
{
	bool ret = false;

	VARIANT varPlatformName = dispPropGet(hr, pPlatform, OLESTR("Name"), VT_BSTR);

	if(arch == "i386")
	{
		VARIANT varWin32 = oleString(hr, OLESTR("Win32"));

		if(SUCCEEDED(hr))
		{
			hr = VarBstrCmp(V_BSTR(&varWin32), V_BSTR(&varPlatformName), 0, NORM_IGNORECASE);
			ret = hr == VARCMP_EQ;
		}

		VariantClear(&varWin32);
	}
	else if(arch == "amd64")
	{
		VARIANT varX64 = oleString(hr, OLESTR("x64"));

		if(SUCCEEDED(hr))
		{
			hr = VarBstrCmp(V_BSTR(&varX64), V_BSTR(&varPlatformName), 0, NORM_IGNORECASE);
			ret = hr == VARCMP_EQ;
		}

		VariantClear(&varX64);
	}
	else if(arch == "arm")
	{
		VARIANT varARM = oleString(hr, OLESTR("ARM"));
		VARIANT varARCHFAM = oleString(hr, OLESTR("ARCHFAM"));
		VARIANT varArchFamily = dispInvoke(hr, pPlatform, OLESTR("GetMacroValue"), varARCHFAM, VT_BSTR);

		if(SUCCEEDED(hr))
		{
			hr = VarBstrCmp(V_BSTR(&varARM), V_BSTR(&varArchFamily), 0, NORM_IGNORECASE);
			ret = hr == VARCMP_EQ;
		}

		VariantClear(&varArchFamily);
		VariantClear(&varARCHFAM);
		VariantClear(&varARM);
	}

	VariantClear(&varPlatformName);

	return SUCCEEDED(hr) && ret;
}

void EnumerateVCTools(const std::string& arch, bool wantCompiler, bool wantLinker, const OLECHAR * szProductTag)
{
	size_t cchProductTag = std::wcslen(szProductTag);
	HRESULT hr;

	hr = OleInitialize(NULL);

	if(SUCCEEDED(hr))
	{
		std::basic_string<TCHAR> strProductKey;
		CharsToString(hr, strProductKey, szProductTag, szProductTag + cchProductTag);

		if(SUCCEEDED(hr))
		{
			strProductKey = TEXT("SOFTWARE\\Microsoft\\") + strProductKey;

			HKEY hkVisualStudio;
			hr = HRESULT_FROM_WIN32(RegOpenKeyEx(HKEY_LOCAL_MACHINE, strProductKey.c_str(), 0, KEY_ENUMERATE_SUB_KEYS, &hkVisualStudio));

			if(SUCCEEDED(hr))
			{
				for(DWORD i = 0; ; ++ i)
				{
					TCHAR szProductVersion[255 + 1];
					DWORD cchProductVersion = ARRAYSIZE(szProductVersion);

					hr = HRESULT_FROM_WIN32(RegEnumKeyEx(hkVisualStudio, i, szProductVersion, &cchProductVersion, NULL, NULL, NULL, NULL));

					if(SUCCEEDED(hr))
					{
						std::basic_string<OLECHAR> strProductVersion;
						CharsToString(hr, strProductVersion, szProductVersion, szProductVersion + cchProductVersion);

						if(SUCCEEDED(hr))
						{
							std::basic_string<OLECHAR> strDTEProgId(szProductTag, szProductTag + cchProductTag);
							strDTEProgId += OLESTR(".DTE.");
							strDTEProgId += strProductVersion;

							CLSID clsidDTE;
							hr = CLSIDFromProgID(strDTEProgId.c_str(), &clsidDTE);

							if(SUCCEEDED(hr))
							{
								IDispatch * pDTE;
								hr = CoCreateInstance(clsidDTE, NULL, CLSCTX_ALL, IID_IDispatch, (void **)(void *)&pDTE);

								if(SUCCEEDED(hr))
								{
									VARIANT varProjectEngine = GetDTEProjectEngine(hr, pDTE);
									VARIANT varPlatforms = dispPropGet(hr, V_DISPATCH(&varProjectEngine), OLESTR("Platforms"), VT_DISPATCH);
									VARIANT varCount = dispPropGet(hr, V_DISPATCH(&varPlatforms), OLESTR("Count"), VT_I4);
									VARIANT varI;
									VariantInit(&varI);
									V_VT(&varI) = VT_I4;

									for(V_I4(&varI) = 1; V_I4(&varI) <= V_I4(&varCount); ++ V_I4(&varI))
									{
										VARIANT varPlatform = dispInvoke(hr, V_DISPATCH(&varPlatforms), varI, VT_DISPATCH);

										if(IsVCPlatformSupported(hr, arch, V_DISPATCH(&varPlatform)))
										{
											VARIANT varPlatformName = dispPropGet(hr, V_DISPATCH(&varPlatform), OLESTR("Name"), VT_BSTR);
											VARIANT varPath = GetDTEPlatformPath(hr, pDTE, V_BSTR(&varPlatformName), OLESTR("ExecutableDirectories"));
											VARIANT varInclude = GetDTEPlatformPath(hr, pDTE, V_BSTR(&varPlatformName), OLESTR("IncludeDirectories"));
											VARIANT varLib = GetDTEPlatformPath(hr, pDTE, V_BSTR(&varPlatformName), OLESTR("LibraryDirectories"));

											ProcessVCPlatform(hr, arch, wantCompiler, wantLinker, V_DISPATCH(&varProjectEngine), V_DISPATCH(&varPlatform), V_BSTR(&varPath), V_BSTR(&varInclude), V_BSTR(&varLib));

											VariantClear(&varLib);
											VariantClear(&varInclude);
											VariantClear(&varPath);
											VariantClear(&varPlatformName);
										}

										VariantClear(&varPlatform);
									}

									VariantClear(&varI);
									VariantClear(&varCount);
									VariantClear(&varPlatforms);
									VariantClear(&varProjectEngine);

									pDTE->Release();
								}
							}

							// TODO: print an error message here
						}

						hr = S_OK;
					}
					else if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
					{
						hr = S_OK;
						break;
					}
					else
						break;
				}
			}
		}

		OleUninitialize();
	}
}

const TCHAR * GetDDKArchPath(const std::string& arch)
{
	if(arch == "i386")
		return TEXT("x86");
	else if(arch == "amd64")
		return TEXT("amd64");
	else
		return NULL;
}

void EnumerateDDKTools(const std::string& arch, bool wantCompiler, bool wantLinker)
{
	const TCHAR * pszArchPath = GetDDKArchPath(arch);

	if(pszArchPath == NULL)
		return;

	HRESULT hr;

	HKEY hkDDK;
	hr = HRESULT_FROM_WIN32(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\WINDDK"), 0, KEY_ENUMERATE_SUB_KEYS, &hkDDK));

	if(SUCCEEDED(hr))
	{
		for(DWORD i = 0; ; ++ i)
		{
			TCHAR szDDKVersion[255 + ARRAYSIZE(TEXT("\\Setup"))];
			DWORD cchDDKVersion = ARRAYSIZE(szDDKVersion);

			hr = HRESULT_FROM_WIN32(RegEnumKeyEx(hkDDK, i, szDDKVersion, &cchDDKVersion, NULL, NULL, NULL, NULL));

			if(SUCCEEDED(hr))
			{
				memcpy(szDDKVersion + cchDDKVersion, TEXT("\\Setup"), sizeof(TEXT("\\Setup")));

				HKEY hkDDKVersion;
				hr = HRESULT_FROM_WIN32(RegOpenKeyEx(hkDDK, szDDKVersion, 0, KEY_QUERY_VALUE, &hkDDKVersion));

				if(SUCCEEDED(hr))
				{
					TCHAR szDDKPath[MAX_PATH];
					DWORD cbDDKPath = sizeof(szDDKPath);
					DWORD dwType = 0;

					hr = HRESULT_FROM_WIN32(RegQueryValueEx(hkDDKVersion, TEXT("BUILD"), NULL, &dwType, reinterpret_cast<BYTE *>(szDDKPath), &cbDDKPath));

					if(SUCCEEDED(hr) && dwType == REG_SZ)
					{
#if defined(_X86_)
						static const TCHAR * const a_szBinPaths[] = { TEXT("x86") };
#elif defined(_AMD64_)
						// Prefer native tools, fall back on x86 if necessary
						static const TCHAR * const a_szBinPaths[] = { TEXT("amd64"), TEXT("x86") };
#elif defined(_IA64_)
						// Prefer native tools, fall back on x86 if necessary
						static const TCHAR * const a_szBinPaths[] = { TEXT("ia64"), TEXT("x86") };
#endif
						szDDKPath[cbDDKPath / sizeof(TCHAR)] = 0;
						std::basic_string<TCHAR> strDDKPath(szDDKPath);
						strDDKPath += TEXT("\\bin\\");

						std::basic_string<TCHAR>::size_type cchCutoff = strDDKPath.length();

						for(unsigned i = 0; i < ARRAYSIZE(a_szBinPaths); ++ i)
						{
							strDDKPath.resize(cchCutoff);
							strDDKPath += a_szBinPaths[i];
							strDDKPath += TEXT("\\");
							strDDKPath += pszArchPath;

							cl_version clVersion;
							link_version linkVersion;

							if(wantCompiler)
								clVersion = CheckClVersion(arch, strDDKPath.c_str());

							if(wantLinker)
								linkVersion = CheckLinkVersion(arch, strDDKPath.c_str());

							if((!wantCompiler || (clVersion && clVersion > msCompilerVersion)) && (!wantLinker || (linkVersion && linkVersion > msLinkerVersion)))
							{
								std::string strPath;
								CharsToString(hr, strPath, strDDKPath.c_str(), strDDKPath.c_str() + strDDKPath.length());

								if(SUCCEEDED(hr))
								{
									if(wantCompiler)
									{
										msCompilerVersion = clVersion;
										msCompilerPath = strPath;
										msCompilerIncludeDirs.clear();
										//msCompilerSource; // TODO: fill this in
									}

									if(wantLinker)
									{
										msLinkerVersion = linkVersion;
										msLinkerPath = strPath;
										msLinkerLibDirs.clear();
										//msLinkerSource; // TODO: fill this in
									}
								}
								else
									break;
							}
						}
					}

					RegCloseKey(hkDDKVersion);
				}
			}
			else if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
			{
				hr = S_OK;
				break;
			}

			if(FAILED(hr))
				break;
		}

		RegCloseKey(hkDDK);
	}
}

void DetectEnvironmentVCTools(const std::string& arch, bool wantCompiler, bool wantLinker)
{
	cl_version clVersion;
	link_version linkVersion;

	if(wantCompiler)
		clVersion = CheckClVersion(arch);

	if(wantLinker)
		linkVersion = CheckLinkVersion(arch);

	if((!wantCompiler || (clVersion && clVersion > msCompilerVersion)) && (!wantLinker || (linkVersion && linkVersion > msLinkerVersion)))
	{
		if(wantCompiler)
		{
			msCompilerVersion = clVersion;
			msCompilerPath.clear();
			msCompilerIncludeDirs = SplitPath(get_env("INCLUDE"));
			msCompilerSource = "environment";
		}

		if(wantLinker)
		{
			msLinkerVersion = linkVersion;
			msLinkerPath.clear();
			msLinkerLibDirs = SplitPath(get_env("LIB"));
			msLinkerSource = "environment";
		}
	}
}

void EnumerateMicrosoftTools(const std::string& arch, bool wantCompiler, bool wantLinker)
{
	DetectEnvironmentVCTools(arch, wantCompiler, wantLinker);

	if(!msCompilerVersion || !msLinkerVersion)
	{
		EnumerateVCTools(arch, wantCompiler, wantLinker, OLESTR("VisualStudio"));
		EnumerateVCTools(arch, wantCompiler, wantLinker, OLESTR("VCExpress"));
		EnumerateDDKTools(arch, wantCompiler, wantLinker);
	}
}

}

bool
MingwBackend::DetectMicrosoftCompiler ( std::string& version )
{
	bool wantCompiler = ProjectNode.configuration.Compiler == MicrosoftC;
	bool wantLinker = ProjectNode.configuration.Linker == MicrosoftLink;

	if ( wantCompiler && !msCompilerVersion )
		EnumerateMicrosoftTools ( Environment::GetArch(), wantCompiler, wantLinker );

	bool ret = wantCompiler && msCompilerVersion;

	if ( ret )
	{
		compilerNeedsHelper = true;
		compilerCommand = "cl";

		std::ostringstream compilerVersion;
		compilerVersion << msCompilerVersion;
		version = compilerVersion.str();
	}

	return ret;
}

bool
MingwBackend::DetectMicrosoftLinker ( std::string& version )
{
	bool wantCompiler = ProjectNode.configuration.Compiler == MicrosoftC;
	bool wantLinker = ProjectNode.configuration.Linker == MicrosoftLink;

	if ( wantLinker && !msLinkerVersion )
		EnumerateMicrosoftTools ( Environment::GetArch(), wantCompiler, wantLinker );

	bool ret = wantLinker && msLinkerVersion;

	if ( ret )
	{
		binutilsNeedsHelper = msLinkerPath.length() != 0;
		binutilsCommand = "link";

		std::ostringstream linkerVersion;
		linkerVersion << msLinkerVersion;
		version = linkerVersion.str();
	}

	return ret;
}

#endif

// TODO? attempt to support Microsoft tools on non-Windows?
#if !defined(WIN32)

#include "../../pch.h"

#include "mingw.h"

bool
MingwBackend::DetectMicrosoftCompiler ( std::string& )
{
	return false;
}

bool
MingwBackend::DetectMicrosoftLinker ( std::string& )
{
	return false;
}

#endif

// EOF
