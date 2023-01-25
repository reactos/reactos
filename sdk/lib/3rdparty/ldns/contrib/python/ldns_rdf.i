/******************************************************************************
 * ldns_rdf.i: LDNS record data
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

/* Creates a temporary instance of (ldns_rdf *). */
%typemap(in, numinputs=0, noblock=1) (ldns_rdf **)
{
  ldns_rdf *$1_rdf = NULL;
  $1 = &$1_rdf;
}
          
/* Result generation, appends (ldns_rdf *) after the result. */
%typemap(argout, noblock=1) (ldns_rdf **)
{
  $result = SWIG_Python_AppendOutput($result,
    SWIG_NewPointerObj(SWIG_as_voidptr($1_rdf),
      SWIGTYPE_p_ldns_struct_rdf, SWIG_POINTER_OWN | 0));
}

/*
 * Automatic conversion of const (ldns_rdf *) parameter from string.
 * Argument default value.
 */
%typemap(arginit, noblock=1) const ldns_rdf *
{
  char *$1_str = NULL;
}

/*
 * Automatic conversion of const (ldns_rdf *) parameter from string.
 * Preparation of arguments.
 */
%typemap(in, noblock=1) const ldns_rdf * (void* argp, $1_ltype tmp = 0, int res)
{
  if (Python_str_Check($input)) {
    $1_str = SWIG_Python_str_AsChar($input);
    if ($1_str == NULL) {
      %argument_fail(SWIG_TypeError, "char *", $symname, $argnum);
    }
    tmp = ldns_dname_new_frm_str($1_str);
    if (tmp == NULL) {
      %argument_fail(SWIG_TypeError, "char *", $symname, $argnum);
    }
    $1 = ($1_ltype) tmp;
  } else {
    res = SWIG_ConvertPtr($input, &argp, SWIGTYPE_p_ldns_struct_rdf, 0 | 0);
    if (!SWIG_IsOK(res)) {
      %argument_fail(res, "ldns_rdf const *", $symname, $argnum);
    }
    $1 = ($1_ltype) argp;
  }
}

/*
 * Automatic conversion of const (ldns_rdf *) parameter from string.
 * Freeing of allocated memory (in Python 3 when daling with strings).
 */
%typemap(freearg, noblock=1) const ldns_rdf *
{
  if ($1_str != NULL) {
    /* Is not NULL only when a conversion form string occurred. */
    SWIG_Python_str_DelForPy3($1_str); /* Is a empty macro for Python < 3. */
  }
}

%nodefaultctor ldns_struct_rdf; /* No default constructor. */
%nodefaultdtor ldns_struct_rdf; /* No default destructor. */


/*
 * This file must contain all %newobject and %delobject tags also for
 * ldns_dname. This is because the ldns_dname is a derived class from ldns_rdf.
 */


%newobject ldns_rdf_new;
%newobject ldns_rdf_new_frm_str;
%newobject ldns_rdf_new_frm_data;

%newobject ldns_rdf_address_reverse;
%newobject ldns_rdf_clone;
%newobject ldns_rdf2str;

%newobject ldns_dname_new;
%newobject ldns_dname_new_frm_str;
%newobject ldns_dname_new_frm_data;

%newobject ldns_dname_cat_clone;
%newobject ldns_dname_label;
%newobject ldns_dname_left_chop;
%newobject ldns_dname_reverse;

%delobject ldns_rdf_deep_free;
%delobject ldns_rdf_free;


/*
 * Should the ldns_rdf_new() also be marked as deleting its data parameter?
 */
%delobject ldns_rdf_set_data; /* Because data are directly coupled into rdf. */

%rename(ldns_rdf) ldns_struct_rdf;


/* ========================================================================= */
/* Debugging related code. */
/* ========================================================================= */

#ifdef LDNS_DEBUG
%rename(__ldns_rdf_deep_free) ldns_rdf_deep_free;
%rename(__ldns_rdf_free) ldns_rdf_free;
%inline
%{
  /*!
   * @brief Prints information about deallocated rdf and deallocates.
   */
  void _ldns_rdf_deep_free (ldns_rdf *r)
  {
    printf("******** LDNS_RDF deep free 0x%lX ************\n",
      (long unsigned int) r);
    ldns_rdf_deep_free(r);
  }

  /*!
   * @brief Prints information about deallocated rdf and deallocates.
   */
  void _ldns_rdf_free (ldns_rdf* r)
  {
    printf("******** LDNS_RDF free 0x%lX ************\n",
      (long unsigned int) r);
    ldns_rdf_free(r);
  }
%}
#else /* !LDNS_DEBUG */
%rename(_ldns_rdf_deep_free) ldns_rdf_deep_free;
%rename(_ldns_rdf_free) ldns_rdf_free;
#endif /* LDNS_DEBUG */


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */


%inline
%{
	/*!
	 * @brief returns a human readable string containing rdf type.
	 */
	const char *ldns_rdf_type2str(const ldns_rdf *rdf)
	{
		if (rdf) {
			switch(ldns_rdf_get_type(rdf)) {
			case LDNS_RDF_TYPE_NONE:       return 0;
			case LDNS_RDF_TYPE_DNAME:      return "DNAME";
			case LDNS_RDF_TYPE_INT8:       return "INT8";
			case LDNS_RDF_TYPE_INT16:      return "INT16";
			case LDNS_RDF_TYPE_INT32:      return "INT32";
			case LDNS_RDF_TYPE_A:          return "A";
			case LDNS_RDF_TYPE_AAAA:       return "AAAA";
			case LDNS_RDF_TYPE_STR:        return "STR";
			case LDNS_RDF_TYPE_APL:        return "APL";
			case LDNS_RDF_TYPE_B32_EXT:    return "B32_EXT";
			case LDNS_RDF_TYPE_B64:        return "B64";
			case LDNS_RDF_TYPE_HEX:        return "HEX";
			case LDNS_RDF_TYPE_NSEC:       return "NSEC";
			case LDNS_RDF_TYPE_TYPE:       return "TYPE";
			case LDNS_RDF_TYPE_CLASS:      return "CLASS";
			case LDNS_RDF_TYPE_CERT_ALG:   return "CER_ALG";
			case LDNS_RDF_TYPE_ALG:        return "ALG";
			case LDNS_RDF_TYPE_UNKNOWN:    return "UNKNOWN";
			case LDNS_RDF_TYPE_TIME:       return "TIME";
			case LDNS_RDF_TYPE_PERIOD:     return "PERIOD";
			case LDNS_RDF_TYPE_TSIGTIME:   return "TSIGTIME";
			case LDNS_RDF_TYPE_HIP:        return "HIP";
			case LDNS_RDF_TYPE_INT16_DATA: return "INT16_DATA";
			case LDNS_RDF_TYPE_SERVICE:    return "SERVICE";
			case LDNS_RDF_TYPE_LOC:        return "LOC";
			case LDNS_RDF_TYPE_WKS:        return "WKS";
			case LDNS_RDF_TYPE_NSAP:       return "NSAP";
			case LDNS_RDF_TYPE_ATMA:       return "ATMA";
			case LDNS_RDF_TYPE_IPSECKEY:   return "IPSECKEY";
			case LDNS_RDF_TYPE_NSEC3_SALT: return "NSEC3_SALT";
			case LDNS_RDF_TYPE_NSEC3_NEXT_OWNER:
			    return "NSEC3_NEXT_OWNER";
			case LDNS_RDF_TYPE_ILNP64:     return "ILNP64";
                        case LDNS_RDF_TYPE_EUI48:      return "EUI48";
                        case LDNS_RDF_TYPE_EUI64:      return "EUI64";
                        case LDNS_RDF_TYPE_TAG:        return "TAG";
                        case LDNS_RDF_TYPE_LONG_STR:   return "LONG_STR";
			case LDNS_RDF_TYPE_AMTRELAY:   return "AMTRELAY";
                        case LDNS_RDF_TYPE_SVCPARAMS:  return "SVCPARAMS";
                        case LDNS_RDF_TYPE_CERTIFICATE_USAGE:
                            return "CERTIFICATE_USAGE";
                        case LDNS_RDF_TYPE_SELECTOR:   return "SELECTOR";
                        case LDNS_RDF_TYPE_MATCHING_TYPE:
                            return "MATCHING_TYPE";
			}
		}
		return 0;
	}
%}


%inline
%{
  /*!
   * @brief Returns the rdf data organised into a list of bytes.
   */
  PyObject * ldns_rdf_data_as_bytearray(const ldns_rdf *rdf)
  {
    Py_ssize_t len;
    uint8_t *data;

    assert(rdf != NULL);

    len = ldns_rdf_size(rdf);
    data = ldns_rdf_data(rdf);

    return PyByteArray_FromStringAndSize((char *) data, len);
  }
%}


/* ========================================================================= */
/* Encapsulating Python code. */
/* ========================================================================= */

%feature("docstring") ldns_struct_rdf "Resource record data field.

The data is a network ordered array of bytes, which size is specified
by the (16-bit) size field. To correctly parse it, use the type
specified in the (16-bit) type field with a value from ldns_rdf_type."

%extend ldns_struct_rdf {
 
  %pythoncode
  %{
        def __init__(self):
            """
               Cannot be created directly from Python.
            """
            raise Exception("This class can't be created directly. " +
                "Please use: ldns_rdf_new, ldns_rdf_new_frm_data, " +
                "ldns_rdf_new_frm_str, ldns_rdf_new_frm_fp, " +
                "ldns_rdf_new_frm_fp_l")
       
        __swig_destroy__ = _ldns._ldns_rdf_deep_free

        #
        # LDNS_RDF_CONSTRUCTORS_
        #

        @staticmethod
        def new_frm_str(string, rr_type, raiseException = True):
            """
               Creates a new rdf from a string of a given type.
               
               :param string: string to use
               :type string: string
               :param rr_type: The type of the rdf. See predefined `RDF_TYPE_`
                   constants.
               :type rr_type: integer
               :param raiseException: If True, an exception occurs in case
                   a RDF object can't be created.
               :type raiseException: bool
               :throws TypeError: When parameters of mismatching types.
               :throws Exception: When raiseException set and rdf couldn't
                   be created.
               :return: :class:`ldns_rdf` object or None. If the object
                   can't be created and `raiseException` is True,
                   an exception occurs.

               **Usage**

                   >>> rdf = ldns.ldns_rdf.new_frm_str("74.125.43.99", ldns.LDNS_RDF_TYPE_A)
                   >>> print rdf, rdf.get_type_str()
                   A 74.125.43.99
                   >>> name = ldns.ldns_resolver.new_frm_file().get_name_by_addr(rdf)
                   >>> if (name): print name
                   99.43.125.74.in-addr.arpa.	85277	IN	PTR	bw-in-f99.google.com.
            """
            rr = _ldns.ldns_rdf_new_frm_str(rr_type, string)
            if (not rr) and raiseException:
                raise Exception("Can't create query packet")
            return rr

        #
        # _LDNS_RDF_CONSTRUCTORS
        #

        def __str__(self):
            """
               Converts the rdata field to presentation format.
            """
            return _ldns.ldns_rdf2str(self)

        def __cmp__(self, other):
            """
               Compares two rdfs on their wire formats.
               
               (To order dnames according to rfc4034, use ldns_dname_compare.)
               
               :param other: The second one RDF.
               :type other: :class:`ldns_rdf`
               :throws TypeError: When `other` of non-:class:`ldns_rdf` type.
               :return: (int) -1, 0 or 1 if self comes before other,
                   is equal or self comes after other respectively.
            """
            return _ldns.ldns_rdf_compare(self, other)

        def __lt__(self, other):
            """
                Compares two rdfs on their formats.

                :param other: The socond one RDF.
                :type other: :class:`ldns_rdf`
                :throws TypeError: When `other` of non-:class:`ldns_rdf` type.
                :return: (bool) True when `self` is less than 'other'.
            """
            return _ldns.ldns_rdf_compare(self, other) == -1

        def __le__(self, other):
            """
                Compares two rdfs on their formats.

                :param other: The socond one RDF.
                :type other: :class:`ldns_rdf`
                :throws TypeError: When `other` of non-:class:`ldns_rdf` type.
                :return: (bool) True when `self` is less than or equal to
                    'other'.
            """
            return _ldns.ldns_rdf_compare(self, other) != 1

        def __eq__(self, other):
            """
                Compares two rdfs on their formats.

                :param other: The socond one RDF.
                :type other: :class:`ldns_rdf`
                :throws TypeError: When `other` of non-:class:`ldns_rdf` type.
                :return: (bool) True when `self` is equal to 'other'.
            """
            return _ldns.ldns_rdf_compare(self, other) == 0

        def __ne__(self, other):
            """
                Compares two rdfs on their formats.

                :param other: The socond one RDF.
                :type other: :class:`ldns_rdf`
                :throws TypeError: When `other` of non-:class:`ldns_rdf` type.
                :return: (bool) True when `self` is not equal to 'other'.
            """
            return _ldns.ldns_rdf_compare(self, other) != 0

        def __gt__(self, other):
            """
                Compares two rdfs on their formats.

                :param other: The socond one RDF.
                :type other: :class:`ldns_rdf`
                :throws TypeError: When `other` of non-:class:`ldns_rdf` type.
                :return: (bool) True when `self` is greater than 'other'.
            """
            return _ldns.ldns_rdf_compare(self, other) == 1

        def __ge__(self, other):
            """
                Compares two rdfs on their formats.

                :param other: The socond one RDF.
                :type other: :class:`ldns_rdf`
                :throws TypeError: When `other` of non-:class:`ldns_rdf` type.
                :return: (bool) True when `self` is greater than or equal to
                    'other'.
            """
            return _ldns.ldns_rdf_compare(self, other) != -1

        def print_to_file(self, output):
            """
               Prints the data in the rdata field to the given `output` file
               stream (in presentation format).
            """
            _ldns.ldns_rdf_print(output, self)

        def get_type_str(self):
            """
               Returns the type of the rdf as a human readable string.

               :return: String containing rdf type.
            """
            return ldns_rdf_type2str(self)

        def write_to_buffer(self, buffer):
            """
               Copies the rdata data to the buffer in wire format.
               
               :param buffer: Buffer to append the rdf to.
               :type param: :class:`ldns_buffer`
               :throws TypeError: When `buffer` of non-:class:`ldns_buffer`
                   type.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_rdf2buffer_wire(buffer, self)
            #parameters: ldns_buffer *, const ldns_rdf *,
            #retvals: ldns_status

        def write_to_buffer_canonical(self, buffer):
            """
               Copies the rdata data to the buffer in wire format.
               If the rdata is a dname, the letters will be converted
               to lower case during the conversion.
               
               :param buffer: LDNS buffer.
               :type buffer: :class:`ldns_buffer`
               :throws TypeError: When `buffer` of non-:class:`ldns_buffer`
                   type.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_rdf2buffer_wire_canonical(buffer, self)
            #parameters: ldns_buffer *, const ldns_rdf *,
            #retvals: ldns_status

        #
        # LDNS_RDF_METHODS_
        #

        def address_reverse(self):
            """
               Reverses an rdf, only actually useful for AAAA and A records.
               
               The returned rdf has the type LDNS_RDF_TYPE_DNAME!
               
               :return: (:class:`ldns_rdf`) The reversed rdf
                   (a newly created rdf).
            """
            return _ldns.ldns_rdf_address_reverse(self)
            #parameters: ldns_rdf *,
            #retvals: ldns_rdf *

        def clone(self):
            """
               Clones a rdf structure.
               
               The data are copied.
               
               :return: (:class:`ldns_rdf`) A new rdf structure.
            """
            return _ldns.ldns_rdf_clone(self)
            #parameters: const ldns_rdf *,
            #retvals: ldns_rdf *

        def data(self):
            """
               Returns the data of the rdf.
               
               :return: (uint8_t \*) uint8_t* pointer to the rdf's data.
            """
            return _ldns.ldns_rdf_data(self)
            #parameters: const ldns_rdf *,
            #retvals: uint8_t *

        def data_as_bytearray(self):
            """
               Returns the data of the rdf as a bytearray.

               :return: (bytearray) Bytearray containing the rdf data.
            """
            return _ldns.ldns_rdf_data_as_bytearray(self)
            #parameters: const ldns_rdf *,
            #retvals: bytearray

        def get_type(self):
            """
               Returns the type of the rdf.
               
               We need to prepend the prefix get_ here to prevent conflict
               with the rdf_type TYPE.
               
               :return: (ldns_rdf_type) Identifier of the type.
            """
            return _ldns.ldns_rdf_get_type(self)
            #parameters: const ldns_rdf *,
            #retvals: ldns_rdf_type

        def set_data(self, data):
            """
               Sets the data portion of the rdf.

               The data are not copied, but are assigned to the rdf,
               `data` are decoupled from the Python engine.
               
               :param data: Data to be set.
               :type data: void \*
            """
            _ldns.ldns_rdf_set_data(self, data)
            #parameters: ldns_rdf *, void *,
            #retvals: 

        def set_size(self, size):
            """
               Sets the size of the rdf.
               
               :param size: The new size.
               :type size: integer
               :throws TypeError: When size of non-integer type.
            """
            _ldns.ldns_rdf_set_size(self,size)
            #parameters: ldns_rdf *,size_t,
            #retvals: 

        def set_type(self, atype):
            """
               Sets the type of the rdf.
               
               :param atype: rdf type
               :type atype: integer
               :throws TypeError: When atype of non-integer type.
            """
            _ldns.ldns_rdf_set_type(self, atype)
            #parameters: ldns_rdf *, ldns_rdf_type,
            #retvals: 

        def size(self):
            """
               Returns the size of the rdf.
               
               :return: (size_t) uint16_t with the size.
            """
            return _ldns.ldns_rdf_size(self)
            #parameters: const ldns_rdf *,
            #retvals: size_t

        @staticmethod
        def dname_new_frm_str(string):
            """
               Creates a new dname rdf instance from a given string.
            
               This static method is equivalent to using of default
               :class:`ldns_rdf` constructor.
 
               :parameter string: String to use.
               :type string: string
               :throws TypeError: When not a string used.
               :return: :class:`ldns_rdf` or None if error.

               .. warning::

                   It is scheduled to be deprecated and removed. Use
                   :class:`ldns_dname` constructor instead.
            """
            warnings.warn("The ldns_rdf.dname_new_frm_str() method is" +
                " scheduled to be deprecated in future releases." +
                " Use ldns_dname constructor instead.",
                PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_new_frm_str(string)

        def absolute(self):
            """
               Checks whether the given dname string is absolute
               (i.e., ends with a '.').

               :return: (bool) True or False

               .. note::

                   This method was malfunctioning in ldns-1.3.16 and also
                   possibly earlier.

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname` to use the method.
            """
            warnings.warn("The ldns_rdf.absolute() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            if self.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                string = self.__str__()
                return _ldns.ldns_dname_str_absolute(string) != 0
            else:
                return False

        def make_canonical(self):
            """
               Put a dname into canonical format (i.e., convert to lower case).

               Performs no action if not a dname.

               .. warning::

                   This method is scheduled to be deprecated and removed.
                   Convert :class:`ldns_rdf` to :class:`ldns_dname` to use
                   the method.
            """
            warnings.warn("The ldns_rdf.make_canonical() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            _ldns.ldns_dname2canonical(self)

        def dname_compare(self, other):
            """
               Compares two dname rdf according to the algorithm
               for ordering in RFC4034 Section 6.

               :param other: The second dname rdf to compare.
               :type other: :class:`ldns_rdf`
               :throws TypeError: When not a :class:`ldns_rdf` used.
               :throws Exception: When not dnames compared.
               :return: (int) -1, 0 or 1 if `self` comes before `other`,
                   `self` is equal or `self` comes after `other` respectively.

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.dname_compare() method is" +
                " scheduled to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if not isinstance(other, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (self.get_type() != _ldns.LDNS_RDF_TYPE_DNAME) or \
               (other.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Both operands must be dname rdfs.")
            return _ldns.ldns_dname_compare(self, other)

        def cat(self, rd2):
            """
               Concatenates `rd2` after `this` dname (`rd2` is copied,
               `this` dname is modified).
               
               :param rd2: The right-hand side.
               :type rd2: :class:`ldns_rdf`
               :throws TypeError: When `rd2` of non-:class:`ldns_rdf` or
                   non-:class:`ldns_dname` type.
               :return: (ldns_status) LDNS_STATUS_OK on success.

               .. warning::

                  It is scheduled to be deprecated and removed. Convert
                  :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.cat() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_cat(self, rd2)
            #parameters: ldns_rdf *, ldns_rdf *,
            #retvals: ldns_status

        def cat_clone(self, rd2):
            """
               Concatenates two dnames together.

               :param rd2: The right-hand side.
               :type rd2: :class:`ldns_rdf`
               :throws TypeError: When `rd2` of non-:class:`ldns_rdf` or
                   non-:class:`ldns_dname` type.
               :return: (:class:`ldns_rdf`) A new rdf with
                   left-hand side + right-hand side content None when
                   error.

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.cat_clone() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_cat_clone(self, rd2)
            #parameters: const ldns_rdf *, const ldns_rdf *,
            #retvals: ldns_rdf *

        def interval(self, middle, next):
            """
               Check whether the `middle` lays in the interval defined by
               `this` and `next` (`this` <= `middle` < `next`).

               This method is useful for nsec checking

               :param middle: The dname to check.
               :type middle: :class:`ldns_rdf`
               :param next: The boundary.
               :type next: :class:`ldns_rdf`
               :throws TypeError: When `middle` or `next` of
                   non-:class:`ldns_rdf` type.
               :throws Exception: When non-dname rdfs compared.
               :return: (int) 0 on error or unknown,
                   -1 when middle is in the interval, 1 when not.

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.interval() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(middle, ldns_rdf)) or \
               (not isinstance(next, ldns_rdf)):
                raise TypeError("Parameters must be derived from ldns_rdf.")
            if (self.get_type() != _ldns.LDNS_RDF_TYPE_DNAME) or \
               (middle.get_type() != _ldns.LDNS_RDF_TYPE_DNAME) or \
               (next.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("All operands must be dname rdfs.")
            return _ldns.ldns_dname_interval(self, middle, next)
            #parameters: const ldns_rdf *, const ldns_rdf *, const ldns_rdf *,
            #retvals: int

        def is_subdomain(self, parent):
            """
               Tests whether the name of the given instance falls under
               `parent` (i.e., is a sub-domain of `parent`).

               This function will return False if the given dnames
               are equal.
               
               :param parent: The parent's name.
               :type parent: :class:`ldns_rdf`
               :throws TypeError: When `parent` of non-:class:`ldns_rdf` type.
               :return: (bool) True if `this` falls under `parent`, otherwise
                   False.

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.is_subdomain() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_is_subdomain(self, parent)
            #parameters: const ldns_rdf *, const ldns_rdf *,
            #retvals: bool

        def label(self, labelpos):
            """
               Look inside the rdf and if it is an LDNS_RDF_TYPE_DNAME try
               and retrieve a specific label.
               
               The labels are numbered starting from 0 (left most).
               
               :param labelpos: Index of the label. (Labels are numbered
                   0, which is the left most.)
               :type labelpos: integer
               :throws TypeError: When `labelpos` of non-integer type.
               :return: (:class:`ldns_rdf`) A new rdf with the label
                   as name or None on error.

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.label() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_label(self, labelpos)
            #parameters: const ldns_rdf *, uint8_t,
            #retvals: ldns_rdf *

        def label_count(self):
            """
               Count the number of labels inside a LDNS_RDF_DNAME type rdf.
               
               :return: (uint8_t) The number of labels. Will return 0 if
                   not a dname.

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.label_count() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_label_count(self)
            #parameters: const ldns_rdf *,
            #retvals: uint8_t

        def left_chop(self):
            """
               Chop one label off the left side of a dname.
               
               (e.g., wwww.nlnetlabs.nl, becomes nlnetlabs.nl)
               
               :return: (:class:`ldns_rdf`) The remaining dname or None when
                   error.

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.left_chop() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_left_chop(self)
            #parameters: const ldns_rdf *,
            #retvals: ldns_rdf *

        def reverse(self):
            """
               Returns a clone of the given dname with the labels reversed.

               When reversing non-dnames a "." (root name) dname is returned.

               :throws Exception: When used on non-dname rdfs.
               :return: (:class:`ldns_rdf`) Clone of the dname with the labels
                   reversed or ".".

               .. warning::

                   It is scheduled to be deprecated and removed. Convert
                   :class:`ldns_rdf` to :class:`ldns_dname`.
            """
            warnings.warn("The ldns_rdf.reverse() method is scheduled" +
                " to be deprecated in future releases." +
                " Convert the ldns_rdf to ldns_dname and the use its" +
                " methods.", PendingDeprecationWarning, stacklevel=2)
            if self.get_type() != _ldns.LDNS_RDF_TYPE_DNAME:
                raise Exception("Operand must be a dname rdf.")
            return _ldns.ldns_dname_reverse(self)
            #parameters: const ldns_rdf *,
            #retvals: ldns_rdf *

        #
        # _LDNS_RDF_METHODS
        #
  %}
} 
