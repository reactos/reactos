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

#ifndef KJK_STRINGZ_ITERATOR_H_
#define KJK_STRINGZ_ITERATOR_H_

#include <cstddef>
#include <iterator>

namespace kjk
{
	template<class Type, class Traits = std::char_traits<Type> > class stringz_iterator;

	template<class CharT> stringz_iterator<CharT> stringz_begin(const CharT *);
	template<class Traits, class CharT> stringz_iterator<CharT, Traits> stringz_begin(const CharT *);

	template<class CharT> stringz_iterator<CharT> stringz_end(const CharT *);
	template<class Traits, class CharT> stringz_iterator<CharT, Traits> stringz_end(const CharT *);

	template<class Type, class Traits>
	class stringz_iterator: public std::iterator<std::forward_iterator_tag, Type, std::ptrdiff_t, const Type *, const Type&>
	{
	private:
		template<class CharT2> friend stringz_iterator<CharT2> stringz_begin(const CharT2 *);
		template<class Traits2, class CharT2> friend stringz_iterator<CharT2, Traits2> stringz_begin(const CharT2 *);

		template<class CharT2> friend stringz_iterator<CharT2> stringz_end(const CharT2 *);
		template<class Traits2, class CharT2> friend stringz_iterator<CharT2, Traits2> stringz_end(const CharT2 *);

		// FIXME: this sucks because GCC sucks
		typedef const Type * pointer_;
		typedef const Type& reference_;

		pointer_ m_p;

		static pointer_ set_pointer(pointer_ p)
		{
			if(p != 0 && Traits::eq_int_type(Traits::to_int_type(*p), 0))
				return 0;
			else
				return p;
		}

		explicit stringz_iterator(pointer_ p): m_p(set_pointer(p)) { }

	public:
		stringz_iterator(): m_p() {}
		stringz_iterator(const stringz_iterator& That): m_p(set_pointer(That.m_p)) { }

		const stringz_iterator& operator=(const stringz_iterator& That)
		{
			m_p = set_pointer(That.m_p);
			return *this;
		}

		bool operator==(const stringz_iterator& That) const
		{
			return m_p == That.m_p;
		}

		bool operator!=(const stringz_iterator& That) const
		{
			return m_p != That.m_p;
		}

		reference_ operator*() const { return *m_p; }

		const stringz_iterator& operator++()
		{
			m_p = set_pointer(m_p + 1);
			return *this;
		}

		stringz_iterator operator++(int)
		{
			stringz_iterator oldValue(*this);
			++ (*this);
			return oldValue;
		}

		pointer_ base() const { return m_p; }
	};

	template<class CharT>
	stringz_iterator<CharT> stringz_begin(const CharT * p)
	{
		return stringz_iterator<CharT>(p);
	}

	template<class Traits, class CharT>
	stringz_iterator<CharT> stringz_begin(const CharT * p)
	{
		return stringz_iterator<CharT, Traits>(p);
	}

	template<class CharT>
	stringz_iterator<CharT> stringz_end(const CharT *)
	{
		return stringz_iterator<CharT>(0);
	}

	template<class Traits, class CharT>
	stringz_iterator<CharT> stringz_end(const CharT *)
	{
		return stringz_iterator<CharT, Traits>(0);
	}
}

#endif

// EOF
