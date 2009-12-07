/*
	Copyright (c) 2008 KJK::Hyperion

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

extern
int return_arg(int arg)
{
	return arg;
}

extern
void no_op(void)
{
}

extern
int return_zero(void)
{
	return 0;
}

extern
int return_positive(void)
{
	return 1234;
}

extern
int return_negative(void)
{
	return -1234;
}

extern
int return_one(void)
{
	return 1;
}

extern
int return_minusone(void)
{
	return -1;
}

extern
int return_zero_2(void * p)
{
	return 0;
}

extern
int return_positive_2(void * p)
{
	return 1234;
}

extern
int return_negative_2(void * p)
{
	return -1234;
}

extern
int return_one_2(void * p)
{
	return 1;
}

extern
int return_minusone_2(void * p)
{
	return -1;
}

extern
int return_zero_3(int n)
{
	return 0;
}

extern
int return_positive_3(int n)
{
	return 1234;
}

extern
int return_negative_3(int n)
{
	return -1234;
}

extern
int return_one_3(int n)
{
	return 1;
}

extern
int return_minusone_3(int n)
{
	return -1;
}

extern
int return_zero_4(void * p, int n)
{
	return 0;
}

extern
int return_positive_4(void * p, int n)
{
	return 1234;
}

extern
int return_negative_4(void * p, int n)
{
	return -1234;
}

extern
int return_one_4(void * p, int n)
{
	return 1;
}

extern
int return_minusone_4(void * p, int n)
{
	return -1;
}

extern
void set_positive(int * p)
{
	*p = 1234;
}

/* EOF */
