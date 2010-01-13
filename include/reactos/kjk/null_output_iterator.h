/*
	Copyright (c) 2009 KJK::Hyperion

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#ifndef KJK_NULL_OUTPUT_ITERATOR_H_
#define KJK_NULL_OUTPUT_ITERATOR_H_

#include <cstddef>
#include <iterator>

namespace kjk
{
	template<class Type>
	struct null_output_reference
	{
		const Type& operator=(const Type& x) { return x; }
	};

	template<class Type, class Distance = std::ptrdiff_t, class Pointer = Type *>
	struct null_output_iterator: public std::iterator<std::output_iterator_tag, Type, Distance, Pointer, null_output_reference<Type> >
	{
		null_output_iterator() {}
		null_output_iterator(const null_output_iterator&) {}
		const null_output_iterator& operator=(const null_output_iterator&) { return *this; }
		const null_output_iterator& operator++() { return *this; }
		null_output_iterator operator++(int) { return null_output_iterator(); }
		null_output_reference<Type> operator*() { return null_output_reference<Type>(); }
	};
}

#endif

// EOF
