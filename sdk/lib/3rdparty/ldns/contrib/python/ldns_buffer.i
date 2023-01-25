/******************************************************************************
 * ldns_buffer.i: LDNS buffer class
 *
 * Copyright (c) 2009, Zdenek Vasicek (vasicek AT fit.vutbr.cz)
 *                     Karel Slany    (slany AT fit.vutbr.cz)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the organization nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/


/* ========================================================================= */
/* SWIG setting and definitions. */
/* ========================================================================= */

/* Creates a temporary instance of (ldns_buffer *). */
%typemap(in, numinputs=0, noblock=1) (ldns_buffer **)
{
  ldns_buffer *$1_buf;
  $1 = &$1_buf;
}
          
/* Result generation, appends (ldns_buffer *) after the result. */
%typemap(argout, noblock=1) (ldns_buffer **)
{
  $result = SWIG_Python_AppendOutput($result,
     SWIG_NewPointerObj(SWIG_as_voidptr($1_buf),
       SWIGTYPE_p_ldns_struct_buffer, SWIG_POINTER_OWN | 0));
}

/*
 * Limit the number of arguments to 2 and deal with variable
 * number of arguments in the Python way.
 */
%varargs(2, char *arg = NULL) ldns_buffer_printf;

%nodefaultctor ldns_struct_buffer; /* No default constructor. */
%nodefaultdtor ldns_struct_buffer; /* No default destructor. */

%newobject ldns_buffer_new;
%newobject ldns_dname_new_frm_data;

%delobject ldns_buffer_free;

%rename(ldns_buffer) ldns_struct_buffer;

%ignore ldns_struct_buffer::_position;
%ignore ldns_struct_buffer::_limit;
%ignore ldns_struct_buffer::_capacity;
%ignore ldns_struct_buffer::_data;
%ignore ldns_struct_buffer::_fixed;
%ignore ldns_struct_buffer::_status;

%ignore ldns_buffer_new_frm_data;


/* ========================================================================= */
/* Debugging related code. */
/* ========================================================================= */

#ifdef LDNS_DEBUG
%rename(__ldns_buffer_free) ldns_buffer_free;
%inline
%{
  /*!
   * @brief Frees the buffer and print a message.
   */
  void _ldns_buffer_free (ldns_buffer* b)
  {
    printf("******** LDNS_BUFFER free 0x%lX ************\n",
    (long unsigned int) b);
    ldns_buffer_free(b);
  }
%}
#else /* !LDNS_DEBUG */
%rename(_ldns_buffer_free) ldns_buffer_free;
#endif /* LDNS_DEBUG */


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */

/* None. */


/* ========================================================================= */
/* Encapsulating Python code. */
/* ========================================================================= */

%feature("docstring") "LDNS buffer."

%extend ldns_struct_buffer {
 
  %pythoncode
  %{
        def __init__(self, capacity):
            """
               Creates a new buffer with the specified capacity.

               :param capacity: Number of bytes to allocate for the buffer.
               :type capacity: integer
               :throws TypeError: When `capacity` of non-integer type.
               :return: (:class:`ldns_buffer`)
            """
            self.this = _ldns.ldns_buffer_new(capacity)
       
        __swig_destroy__ = _ldns._ldns_buffer_free

        def __str__(self):
            """
               Returns the data in the buffer as a string.
               Buffer data must be char * type.

               :return: string
            """
            return _ldns.ldns_buffer2str(self)

        def getc(self):
            """
               Returns the next character from a buffer.
               
               Advances the position pointer with 1. When end of buffer
               is reached returns EOF. This is the buffer's equivalent
               for getc().
               
               :return: (integer) EOF on failure otherwise return
                   the character.
            """
            return _ldns.ldns_bgetc(self)

        #
        # LDNS_BUFFER_METHODS_
        #

        def at(self, at):
            """
               Returns a pointer to the data at the indicated position.
               
               :param at: position
               :type at: positive integer
               :throws TypeError: When `at` of non-integer type.
               :return: (uint8_t \*) The pointer to the data.
            """
            return _ldns.ldns_buffer_at(self, at)
            #parameters: const ldns_buffer *, size_t,
            #retvals: uint8_t *

        def available(self, count):
            """
               Checks whether the buffer has count bytes available at
               the current position.
               
               :param count: How much is available.
               :type count: integer
               :throws TypeError: When `count` of non-integer type.
               :return: (bool) True or False.
            """
            return _ldns.ldns_buffer_available(self, count) != 0
            #parameters: ldns_buffer *, size_t,
            #retvals: int

        def available_at(self, at, count):
            """
               Checks if the buffer has at least `count` more bytes available.
               
               Before reading or writing the caller needs to ensure that
               enough space is available!
               
               :param at: Indicated position.
               :type at: positive integer
               :param count: How much is available.
               :type count: positive integer
               :throws TypeError: When `at` or `count` of non-integer type.
               :return: (bool) True or False.
            """
            return _ldns.ldns_buffer_available_at(self, at, count) != 0
            #parameters: ldns_buffer *,size_t,size_t,
            #retvals: int

        def begin(self):
            """
               Returns a pointer to the beginning of the buffer
               (the data at position 0).
               
               :return: (uint8_t \*) Pointer.
            """
            return _ldns.ldns_buffer_begin(self)
            #parameters: const ldns_buffer *,
            #retvals: uint8_t *

        def capacity(self):
            """
               Returns the number of bytes the buffer can hold.
               
               :return: (size_t) The number of bytes.
            """
            return _ldns.ldns_buffer_capacity(self)
            #parameters: ldns_buffer *,
            #retvals: size_t

        def clear(self):
            """
               Clears the buffer and make it ready for writing.
               
               The buffer's limit is set to the capacity and the position
               is set to 0.
            """
            _ldns.ldns_buffer_clear(self)
            #parameters: ldns_buffer *,
            #retvals: 

        def copy(self, bfrom):
            """
               Copy contents of the other buffer to this buffer.
               
               Silently truncated if this buffer is too small.
               
               :param bfrom: Source buffer.
               :type bfrom: :class:`ldns_buffer`
               :throws TypeError: When `bfrom` of non-:class:`ldns_buffer`
                   type.
            """
            _ldns.ldns_buffer_copy(self, bfrom)
            #parameters: ldns_buffer *, ldns_buffer *,
            #retvals: 

        def current(self):
            """
               Returns a pointer to the data at the buffer's current position.
               
               :return: (uint8_t \*) A pointer.
            """
            return _ldns.ldns_buffer_current(self)
            #parameters: ldns_buffer *,
            #retvals: uint8_t *

        def end(self):
            """
               Returns a pointer to the end of the buffer (the data
               at the buffer's limit).
               
               :return: (uint8_t \*) Pointer.
            """
            return _ldns.ldns_buffer_end(self)
            #parameters: ldns_buffer *,
            #retvals: uint8_t *

        def export(self):
            """
               Makes the buffer fixed and returns a pointer to the data.
               
               The caller is responsible for freeing the result.
               
               :return: (void \*) Void pointer.
            """
            return _ldns.ldns_buffer_export(self)
            #parameters: ldns_buffer *,
            #retvals: void *

        def flip(self):
            """
               Makes the buffer ready for reading the data that has been
               written to the buffer.
               
               The buffer's limit is set to the current position and
               the position is set to 0.
            """
            _ldns.ldns_buffer_flip(self)
            #parameters: ldns_buffer *,

        def invariant(self):
            """
               Performs no action.

               In debugging mode this method performs a buffer settings
               check. It asserts if something is wrong.
            """
            _ldns.ldns_buffer_invariant(self)
            #parameters: ldns_buffer *,

        def limit(self):
            """
               Returns the maximum size of the buffer.
               
               :return: (size_t) The size.
            """
            return _ldns.ldns_buffer_limit(self)
            #parameters: ldns_buffer *,
            #retvals: size_t

        def position(self):
            """
               Returns the current position in the buffer
               (as a number of bytes).
               
               :return: (size_t) The current position.
            """
            return _ldns.ldns_buffer_position(self)
            #parameters: ldns_buffer *,
            #retvals: size_t

        def printf(self, string, *args):
            """
               Prints to the buffer, increasing the capacity
               if required using buffer_reserve().
               
               The buffer's position is set to the terminating '\0'.
               Returns the number of characters written (not including
               the terminating '\0') or -1 on failure.

               :param string: A string to be written.
               :type string: string
               :throws: TypeError when `string` not a string.
               :return: (int) Number of written characters or -1 on failure.
            """
            data = string % args
            return _ldns.ldns_buffer_printf(self, data)
            #parameters: ldns_buffer *, const char *, ...
            #retvals: int

        def read(self, data, count):
            """
               Copies count bytes of data at the current position to the given
               `data`-array
               
               :param data: Target buffer to copy to.
               :type data: void \*
               :param count: The length of the data to copy.
               :type count: size_t
            """
            _ldns.ldns_buffer_read(self,data,count)
            #parameters: ldns_buffer *, void *, size_t,
            #retvals: 

        def read_at(self, at, data, count):
            """
               Copies count bytes of data at the given position to the
               given `data`-array.
               
               :param at: The position in the buffer to start reading.
               :type at: size_t
               :param data: Target buffer to copy to.
               :type data: void \*
               :param count: The length of the data to copy.
               :type count: size_t
            """
            _ldns.ldns_buffer_read_at(self,at,data,count)
            #parameters: ldns_buffer *, size_t, void *, size_t,
            #retvals: 

        def read_u16(self):
            """
               Returns the 2-byte integer value at the current position
               from the buffer.
               
               :return: (uint16_t) Word.
            """
            return _ldns.ldns_buffer_read_u16(self)
            #parameters: ldns_buffer *,
            #retvals: uint16_t

        def read_u16_at(self, at):
            """
               Returns the 2-byte integer value at the given position
               from the buffer.
               
               :param at: Position in the buffer.
               :type at: positive integer
               :throws TypeError: When `at` of non-integer type.
               :return: (uint16_t) Word.
            """
            return _ldns.ldns_buffer_read_u16_at(self, at)
            #parameters: ldns_buffer *, size_t,
            #retvals: uint16_t

        def read_u32(self):
            """
               Returns the 4-byte integer value at the current position
               from the buffer.
               
               :return: (uint32_t) Double-word.
            """
            return _ldns.ldns_buffer_read_u32(self)
            #parameters: ldns_buffer *,
            #retvals: uint32_t

        def read_u32_at(self, at):
            """
               Returns the 4-byte integer value at the given position
               from the buffer.
               
               :param at: Position in the buffer.
               :type at: positive integer
               :throws TypeError: When `at` of non-integer type.
               :return: (uint32_t) Double-word.
            """
            return _ldns.ldns_buffer_read_u32_at(self, at)
            #parameters: ldns_buffer *, size_t,
            #retvals: uint32_t

        def read_u8(self):
            """
               Returns the byte value at the current position from the buffer.
               
               :return: (uint8_t) A byte (not a character).
            """
            return _ldns.ldns_buffer_read_u8(self)
            #parameters: ldns_buffer *,
            #retvals: uint8_t

        def read_u8_at(self, at):
            """
               Returns the byte value at the given position from the buffer.
               
               :param at: The position in the buffer.
               :type at: positive integer
               :throws TypeError: When `at` of non-integer type.
               :return: (uint8_t) Byte value.
            """
            return _ldns.ldns_buffer_read_u8_at(self, at)
            #parameters: ldns_buffer *, size_t,
            #retvals: uint8_t

        def remaining(self):
            """
               Returns the number of bytes remaining between the buffer's
               position and limit.
               
               :return: (size_t) The number of bytes.
            """
            return _ldns.ldns_buffer_remaining(self)
            #parameters: ldns_buffer *,
            #retvals: size_t

        def remaining_at(self, at):
            """
               Returns the number of bytes remaining between the indicated
               position and the limit.
               
               :param at: Indicated position.
               :type at: positive integer
               :throws TypeError: When `at` of non-integer type.
               :return: (size_t) number of bytes
            """
            return _ldns.ldns_buffer_remaining_at(self, at)
            #parameters: ldns_buffer *,size_t,
            #retvals: size_t

        def reserve(self, amount):
            """
               Ensures that the buffer can contain at least `amount` more
               bytes.
               
               The buffer's capacity is increased if necessary using
               buffer_set_capacity().
               
               The buffer's limit is always set to the (possibly increased)
               capacity.
               
               :param amount: Amount to use.
               :type amount: positive integer
               :throws TypeError: When `amount` of non-integer type.
               :return: (bool) Whether this failed or succeeded.
            """
            return _ldns.ldns_buffer_reserve(self, amount)
            #parameters: ldns_buffer *, size_t,
            #retvals: bool

        def rewind(self):
            """
               Make the buffer ready for re-reading the data.
               
               The buffer's position is reset to 0.
            """
            _ldns.ldns_buffer_rewind(self)
            #parameters: ldns_buffer *,
            #retvals: 

        def set_capacity(self, capacity):
            """
               Changes the buffer's capacity.
               
               The data is reallocated so any pointers to the data may become
               invalid. The buffer's limit is set to the buffer's new capacity.
               
               :param capacity: The capacity to use.
               :type capacity: positive integer
               :throws TypeError: When `capacity` of non-integer type.
               :return: (bool) whether this failed or succeeded
            """
            return _ldns.ldns_buffer_set_capacity(self, capacity)
            #parameters: ldns_buffer *, size_t,
            #retvals: bool

        def set_limit(self, limit):
            """
               Changes the buffer's limit.
               
               If the buffer's position is greater than the new limit
               then the position is set to the limit.
               
               :param limit: The new limit.
               :type limit: positive integer
               :throws TypeError: When `limit` of non-integer type.
            """
            _ldns.ldns_buffer_set_limit(self, limit)
            #parameters: ldns_buffer *, size_t,
            #retvals: 

        def set_position(self,mark):
            """
               Sets the buffer's position to `mark`.
               
               The position must be less than or equal to the buffer's limit.
               
               :param mark: The mark to use.
               :type mark: positive integer
               :throws TypeError: When `mark` of non-integer type.
            """
            _ldns.ldns_buffer_set_position(self,mark)
            #parameters: ldns_buffer *,size_t,
            #retvals: 

        def skip(self, count):
            """
               Changes the buffer's position by `count` bytes.
               
               The position must not be moved behind the buffer's limit or
               before the beginning of the buffer.
               
               :param count: The count to use.
               :type count: integer
               :throws TypeError: When `count` of non-integer type.
            """
            _ldns.ldns_buffer_skip(self, count)
            #parameters: ldns_buffer *, ssize_t,
            #retvals: 

        def status(self):
            """
               Returns the status of the buffer.
               
               :return: (ldns_status) The status.
            """
            return _ldns.ldns_buffer_status(self)
            #parameters: ldns_buffer *,
            #retvals: ldns_status

        def status_ok(self):
            """
               Returns True if the status of the buffer is LDNS_STATUS_OK,
               False otherwise.
               
               :return: (bool) True or False.
            """
            return _ldns.ldns_buffer_status_ok(self)
            #parameters: ldns_buffer *,
            #retvals: bool

        def write(self, data, count):
            """
               Writes count bytes of data to the current position of
               the buffer.
               
               :param data: The data to write.
               :type data: void \*
               :param count: The length of the data to write.
               :type count: size_t
            """
            _ldns.ldns_buffer_write(self, data, count)
            #parameters: ldns_buffer *, const void *, size_t,
            #retvals: 

        def write_at(self, at, data, count):
            """
               Writes the given data to the buffer at the specified position
               by `at`.
               
               :param at: The position (in number of bytes) to write the
                   data at.
               :param data: Pointer to the data to write to the buffer.
               :param count: The number of bytes of data to write.
            """
            _ldns.ldns_buffer_write_at(self, at, data, count)
            #parameters: ldns_buffer *, size_t, const void *, size_t,
            #retvals: 

        def write_string(self, string):
            """
               Copies the given (null-delimited) string to the current
               position into the buffer.
               
               :param string: The string to write.
               :type string: string
               :throws TypeError: When `string` not a string.
            """
            _ldns.ldns_buffer_write_string(self,string)
            #parameters: ldns_buffer *,const char *,
            #retvals: 

        def write_string_at(self, at, string):
            """
               Copies the given (null-delimited) string to the specified
               position `at` into the buffer.
               
               :param at: The position in the buffer.
               :type at: positive integer
               :param string: The string to write.
               :type string: string
               :throws TypeError: When types mismatch.
            """
            _ldns.ldns_buffer_write_string_at(self, at, string)
            #parameters: ldns_buffer *, size_t, const char *,
            #retvals: 

        def write_u16(self, data):
            """Writes the given 2 byte integer at the current
               position in the buffer.
               
               :param data: The word to write.
               :type data: uint16_t
               :throws TypeError: When `data` of non-integer type.
            """
            _ldns.ldns_buffer_write_u16(self, data)
            #parameters: ldns_buffer *, uint16_t,
            #retvals: 

        def write_u16_at(self, at, data):
            """
               Writes the given 2 byte integer at the given position
               in the buffer.
               
               :param at: The position in the buffer.
               :type at: positive integer
               :param data: The word to write.
               :type data: uint16_t
               :throws TypeError: When `at` or `data` of non-integer type.
            """
            _ldns.ldns_buffer_write_u16_at(self,at,data)
            #parameters: ldns_buffer *,size_t,uint16_t,
            #retvals: 

        def write_u32(self, data):
            """
               Writes the given 4 byte integer at the current position
               in the buffer.
               
               :param data: The double-word to write.
               :type data: uint32_t
               :throws TypeError: When `data` of non-integer type.
            """
            _ldns.ldns_buffer_write_u32(self, data)
            #parameters: ldns_buffer *, uint32_t,
            #retvals: 

        def write_u32_at(self, at, data):
            """
               Writes the given 4 byte integer at the given position
               in the buffer.
               
               :param at: The position in the buffer.
               :type at: positive integer
               :param data: The double-word to write.
               :type data: uint32_t
               :throws TypeError: When `at` or `data` of non-integer type.
            """
            _ldns.ldns_buffer_write_u32_at(self, at, data)
            #parameters: ldns_buffer *,size_t,uint32_t,
            #retvals: 

        def write_u8(self, data):
            """
               Writes the given byte of data at the current position
               in the buffer.
               
               :param data: The byte to write.
               :type data: uint8_t
               :throws TypeError: When `data` of non-integer type.
            """
            _ldns.ldns_buffer_write_u8(self, data)
            #parameters: ldns_buffer *, uint8_t,
            #retvals: 

        def write_u8_at(self,at,data):
            """
               Writes the given byte of data at the given position
               in the buffer.
               
               :param at: The position in the buffer.
               :type at: positive integer
               :param data: The byte to write.
               :type data: uint8_t
               :throws TypeError: When `at` or `data` of non-integer type.
            """
            _ldns.ldns_buffer_write_u8_at(self,at,data)
            #parameters: ldns_buffer *,size_t,uint8_t,
            #retvals: 

        #
        # _LDNS_BUFFER_METHODS
        #
  %}
}
