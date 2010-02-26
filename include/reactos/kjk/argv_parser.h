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

#ifndef KJK_ARGV_PARSER_H_
#define KJK_ARGV_PARSER_H_

#include <iterator>
#include <kjk/null_output_iterator.h>

namespace kjk
{
	namespace details
	{
		template<class Traits>
		bool is_separator(typename Traits::int_type c)
		{
			return Traits::eq_int_type(c, ' ') || Traits::eq_int_type(c, '\t');
		}
	}

	template<class Elem, class Traits, class InIter, class OutIter>
	InIter copy_argument(InIter cur, InIter end, OutIter arg)
	{
		typename Traits::int_type c;
		bool quoting = false;

		while(cur != end)
		{
			c = Traits::to_int_type(*cur);

			if(!details::is_separator<Traits>(c))
				break;

			++ cur;
		}

		while(cur != end)
		{
			typedef typename std::iterator_traits<InIter>::difference_type difference_type;
			difference_type backslashes(0);

			do
			{
				c = Traits::to_int_type(*cur);
				++ cur;

				if(Traits::eq_int_type(c, '\\'))
					++ backslashes;
				else
					break;
			}
			while(cur != end);

			if(Traits::eq_int_type(c, '"'))
			{
				// c == '"'

				if((backslashes % 2) == 0)
				{
					// 2N backslashes + "" in quote = N backslashes, literal "
					if(quoting && cur != end && Traits::eq_int_type(Traits::to_int_type(*cur), '"'))
					{
						c = '"';
						++ cur;
					}
					// 2N backslashes + " = N backslashes, toggle quoting
					else
					{
						quoting = !quoting;
						c = Traits::eof();
					}

				}
				// 2N+1 backslashes + " = N backslashes, literal "

				backslashes /= 2;
			}

			// Flush backslashes
			for(difference_type i = 0; i < backslashes; ++ i)
				*arg ++ = Traits::to_char_type('\\');

			// Handle current character, unless it was a special quote
			if(c != Traits::eof())
			{
				if(details::is_separator<Traits>(c) && !quoting)
					break;
				else
					*arg ++ = Traits::to_char_type(c);
			}
		}

		while(cur != end)
		{
			c = Traits::to_int_type(*cur);

			if(!details::is_separator<Traits>(c))
				break;

			++ cur;
		}

		return cur;
	}

	template<class InIter, class OutIter>
	InIter copy_argument(InIter cur, InIter end, OutIter arg)
	{
		return copy_argument
		<
			typename std::iterator_traits<InIter>::value_type,
			typename std::char_traits<typename std::iterator_traits<InIter>::value_type>,
			InIter,
			OutIter
		>
		(cur, end, arg);
	}

	template<class Elem, class Traits, class InIter>
	InIter skip_argument(InIter cur, InIter end)
	{
		return copy_argument(cur, end, null_output_iterator<Elem>());
	}

	template<class InIter>
	InIter skip_argument(InIter cur, InIter end)
	{
		return copy_argument
		<
			typename std::iterator_traits<InIter>::value_type,
			typename std::char_traits<typename std::iterator_traits<InIter>::value_type>,
			InIter
		>
		(cur, end, null_output_iterator<typename std::iterator_traits<InIter>::value_type>());
	}
}

#endif

// EOF
