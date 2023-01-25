/******************************************************************************
 * ldns_dname.i: LDNS domain name class
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

/*
 * Not here (with the exception of functions defined in this C code section),
 * must be set in ldns_rdf.i.
 */


/* ========================================================================= */
/* Debugging related code. */
/* ========================================================================= */

/*
 * Not here (with the exception of functions defined in this C code section),
 * must be set in ldns_rdf.i.
 */


/* ========================================================================= */
/* Added C code. */
/* ========================================================================= */

/* None */


/* ========================================================================= */
/* Encapsulating Python code. */
/* ========================================================================= */

%pythoncode
%{
    class ldns_dname(ldns_rdf):
        """
           Domain name.

           This class contains methods to read and manipulate domain name drfs.
           Domain names are stored in :class:`ldns_rdf` structures,
           with the type LDNS_RDF_TYPE_DNAME. This class encapsulates such
           rdfs.

           **Usage** 

               >>> import ldns
               >>> dn1 = ldns.ldns_dname("test.nic.cz")
               >>> print dn1
               test.nic.cz.
               >>> dn2 = ldns.ldns_dname("nic.cz")
               >>> if dn2.is_subdomain(dn1): print dn2, "is sub-domain of", dn1
               >>> if dn1.is_subdomain(dn2): print dn1, "is sub-domain of", dn2
               test.nic.cz. is sub-domain of nic.cz.

           The following two examples show the creation of :class:`ldns_dname`
           from :class:`ldns_rdf`. The first shows the creation of
           :class:`ldns_dname` instance which is independent of the original
           `rdf`.
           

               >>> import ldns
               >>> rdf = ldns.ldns_rdf.new_frm_str("a.ns.nic.cz", ldns.LDNS_RDF_TYPE_DNAME)
               >>> dn = ldns.ldns_dname(rdf)
               >>> print dn
               a.ns.nic.cz.

           The latter shows the wrapping of a :class:`ldns_rdf` onto
           a :class:`ldns_dname` without the creation of a copy.

               >>> import ldns
               >>> dn = ldns.ldns_dname(ldns.ldns_rdf.new_frm_str("a.ns.nic.cz", ldns.LDNS_RDF_TYPE_DNAME), clone=False)
               >>> print dn
               a.ns.nic.cz.
        """
        def __init__(self, initialiser, clone=True):
            """
               Creates a new dname rdf from a string or :class:`ldns_rdf`.
            
               :param initialiser: string or :class:`ldns_rdf`
               :type initialiser: string or :class:`ldns_rdf` containing
                   a dname
               :param clone: Whether to clone or directly grab the parameter.
               :type clone: bool
               :throws TypeError: When `initialiser` of invalid type.
            """
            if isinstance(initialiser, ldns_rdf) and \
               (initialiser.get_type() == _ldns.LDNS_RDF_TYPE_DNAME):
                if clone == True:
                    self.this = _ldns.ldns_rdf_clone(initialiser)
                else:
                    self.this = initialiser
            else:
                self.this = _ldns.ldns_dname_new_frm_str(initialiser)

        #
        # LDNS_DNAME_CONSTRUCTORS_
        #
       
        @staticmethod
        def new_frm_str(string):
            """
               Creates a new dname rdf instance from a string.
            
               This static method is equivalent to using default
               :class:`ldns_dname` constructor.
 
               :param string: String to use.
               :type string: string
               :throws TypeError: When `string` not a string.
               :return: (:class:`ldns_dname`) dname rdf.
            """
            return ldns_dname(string)

        @staticmethod
        def new_frm_rdf(rdf, clone=True):
            """
               Creates a new dname rdf instance from a dname :class:`ldns_rdf`.

               This static method is equivalent to using the default
               :class:`ldns_dname` constructor.

               :param rdf: A dname :class:`ldns_rdf`.
               :type rdf: :class:`ldns_rdf`
               :throws TypeError: When `rdf` of inappropriate type.
               :param clone: Whether to create a clone or to wrap present
                   instance.
               :type clone: bool
               :return: (:class:`ldns_dname`) dname rdf.
            """
            return ldns_dname(rdf, clone=clone)

        #
        # _LDNS_DNAME_CONSTRUCTORS
        #

        def write_to_buffer(self, buffer):
            """
               Copies the dname data to the buffer in wire format.
               
               :param buffer: Buffer to append the result to.
               :type param: :class:`ldns_buffer`
               :throws TypeError: When `buffer` of non-:class:`ldns_buffer`
                   type.
               :return: (ldns_status) ldns_status
            """
            return _ldns.ldns_dname2buffer_wire(buffer, self)
            #parameters: ldns_buffer *, const ldns_rdf *,
            #retvals: ldns_status


        #
        # LDNS_DNAME_METHODS_
        #

        def absolute(self):
            """
               Checks whether the given dname string is absolute (i.e.,
               ends with a '.').

               :return: (bool) True or False
            """
            string = self.__str__()
            return _ldns.ldns_dname_str_absolute(string) != 0

        def make_canonical(self):
            """
               Put a dname into canonical format (i.e., convert to lower case).
            """
            _ldns.ldns_dname2canonical(self)

        def __cmp__(self, other):
            """
               Compares two dname rdf according to the algorithm for
               ordering in RFC4034 Section 6.
               
               :param other: The second dname rdf to compare.
               :type other: :class:`ldns_dname`
               :throws TypeError: When `other` of invalid type.
               :return: (int) -1, 0 or 1 if self comes before other,
                   self is equal or self comes after other respectively.

               .. note::
                   The type checking of parameter `other` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.                   
            """
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(other, ldns_dname)) and \
               isinstance(other, ldns_rdf) and \
               other.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.__cmp__() method will" +
                    " drop the possibility to compare ldns_rdf." +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            if not isinstance(other, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (other.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            return _ldns.ldns_dname_compare(self, other)

        def __lt__(self, other):
            """
               Compares two dname rdf according to the algorithm for
               ordering in RFC4034 Section 6.
               
               :param other: The second dname rdf to compare.
               :type other: :class:`ldns_dname`
               :throws TypeError: When `other` of invalid type.
               :return: (bool) True when `self` is less than 'other'.

               .. note::
                   The type checking of parameter `other` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.                   
            """
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(other, ldns_dname)) and \
               isinstance(other, ldns_rdf) and \
               other.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.__lt__() method will" +
                    " drop the possibility to compare ldns_rdf." +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            if not isinstance(other, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (other.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            return _ldns.ldns_dname_compare(self, other) == -1

        def __le__(self, other):
            """
               Compares two dname rdf according to the algorithm for
               ordering in RFC4034 Section 6.
               
               :param other: The second dname rdf to compare.
               :type other: :class:`ldns_dname`
               :throws TypeError: When `other` of invalid type.
               :return: (bool) True when `self` is less than or equal to
                   'other'.

               .. note::
                   The type checking of parameter `other` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.                   
            """
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(other, ldns_dname)) and \
               isinstance(other, ldns_rdf) and \
               other.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.__le__() method will" +
                    " drop the possibility to compare ldns_rdf." +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            if not isinstance(other, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (other.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            return _ldns.ldns_dname_compare(self, other) != 1

        def __eq__(self, other):
            """
               Compares two dname rdf according to the algorithm for
               ordering in RFC4034 Section 6.
               
               :param other: The second dname rdf to compare.
               :type other: :class:`ldns_dname`
               :throws TypeError: When `other` of invalid type.
               :return: (bool) True when `self` is equal to 'other'.

               .. note::
                   The type checking of parameter `other` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.                   
            """
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(other, ldns_dname)) and \
               isinstance(other, ldns_rdf) and \
               other.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.__eq__() method will" +
                    " drop the possibility to compare ldns_rdf." +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            if not isinstance(other, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (other.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            return _ldns.ldns_dname_compare(self, other) == 0

        def __ne__(self, other):
            """
               Compares two dname rdf according to the algorithm for
               ordering in RFC4034 Section 6.
               
               :param other: The second dname rdf to compare.
               :type other: :class:`ldns_dname`
               :throws TypeError: When `other` of invalid type.
               :return: (bool) True when `self` is not equal to 'other'.

               .. note::
                   The type checking of parameter `other` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.                   
            """
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(other, ldns_dname)) and \
               isinstance(other, ldns_rdf) and \
               other.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.__ne__() method will" +
                    " drop the possibility to compare ldns_rdf." +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            if not isinstance(other, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (other.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            return _ldns.ldns_dname_compare(self, other) != 0

        def __gt__(self, other):
            """
               Compares two dname rdf according to the algorithm for
               ordering in RFC4034 Section 6.
               
               :param other: The second dname rdf to compare.
               :type other: :class:`ldns_dname`
               :throws TypeError: When `other` of invalid type.
               :return: (bool) True when `self` is greater than 'other'.

               .. note::
                   The type checking of parameter `other` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.                   
            """
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(other, ldns_dname)) and \
               isinstance(other, ldns_rdf) and \
               other.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.__gt__() method will" +
                    " drop the possibility to compare ldns_rdf." +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            if not isinstance(other, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (other.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            return _ldns.ldns_dname_compare(self, other) == 1

        def __ge__(self, other):
            """
               Compares two dname rdf according to the algorithm for
               ordering in RFC4034 Section 6.
               
               :param other: The second dname rdf to compare.
               :type other: :class:`ldns_dname`
               :throws TypeError: When `other` of invalid type.
               :return: (bool) True when `self` is greater than or equal to
                   'other'.

               .. note::
                   The type checking of parameter `other` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.                   
            """
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(other, ldns_dname)) and \
               isinstance(other, ldns_rdf) and \
               other.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.__ge__() method will" +
                    " drop the possibility to compare ldns_rdf." +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            if not isinstance(other, ldns_rdf):
                raise TypeError("Parameter must be derived from ldns_rdf.")
            if (other.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("Operands must be ldns_dname.")
            return _ldns.ldns_dname_compare(self, other) != -1

        def cat(self, rd2):
            """
               Concatenates rd2 after this dname (`rd2` is copied,
               `this` dname is modified).
               
               :param rd2: The right-hand side.
               :type rd2: :class:`ldns_dname`
               :throws TypeError: When `rd2` of invalid type.
               :return: (ldns_status) LDNS_STATUS_OK on success

               .. note::
                   The type checking of parameter `rd2` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(rd2, ldns_dname)) and \
               isinstance(rd2, ldns_rdf) and \
               rd2.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.cat() method will" +
                    " drop the support of ldns_rdf."  +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_cat(self, rd2)
            #parameters: ldns_rdf *, ldns_rdf *,
            #retvals: ldns_status

        def cat_clone(self, rd2):
            """
               Concatenates two dnames together.
               
               :param rd2: The right-hand side.
               :type rd2: :class:`ldns_dname`
               :throws TypeError: When `rd2` of invalid type.
               :return: (:class:`ldns_dname`) A new rdf with
                   left-hand side + right-hand side content None when
                   error.

               .. note::
                   The type checking of parameter `rd2` is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(rd2, ldns_dname)) and \
               isinstance(rd2, ldns_rdf) and \
               rd2.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.cat_clone() method will" +
                    " drop the support of ldns_rdf."  +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            ret = _ldns.ldns_dname_cat_clone(self, rd2)
            if ret != None:
                ret = ldns_dname(ret, clone=False)
            return ret
            #parameters: const ldns_rdf *, const ldns_rdf *,
            #retvals: ldns_rdf *

        def interval(self, middle, next):
            """
               Check whether `middle` lays in the interval defined by
               `this` and `next` (`this` <= `middle` < `next`).
               
               This method is useful for nsec checking.
               
               :param middle: The dname to check.
               :type middle: :class:`ldns_dname`
               :param next: The boundary.
               :type next: :class:`ldns_dname`
               :throws TypeError: When `middle` or `next` of
                   non-:class:`ldns_rdf` type.
               :throws Exception: When non-dname rdfs compared.
               :return: (int) 0 on error or unknown,
                   -1 when middle is in the interval, 1 when not.

               .. note::
                   The type checking of parameters is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            #
            # The wrapped function generates asserts instead of setting
            # error status. They cannot be caught from Python so a check
            # is necessary. 
            #
            if (not isinstance(middle, ldns_rdf)) or \
               (not isinstance(next, ldns_rdf)):
                raise TypeError("Parameters must be derived from ldns_dname.")
            if (self.get_type() != _ldns.LDNS_RDF_TYPE_DNAME) or \
               (middle.get_type() != _ldns.LDNS_RDF_TYPE_DNAME) or \
               (next.get_type() != _ldns.LDNS_RDF_TYPE_DNAME):
                raise Exception("All operands must be dname rdfs.")
            if (not isinstance(middle, ldns_dname)) or \
               (not isinstance(next, ldns_dname)):
                warnings.warn("The ldns_dname.interval() method will" +
                    " drop the possibility to compare ldns_rdf."  +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_interval(self, middle, next)
            #parameters: const ldns_rdf *, const ldns_rdf *, const ldns_rdf *,
            #retvals: int

        def is_subdomain(self, parent):
            """
               Tests whether the name of the instance falls under
               `parent` (i.e., is a sub-domain of `parent`). 

               This function will return false if the given dnames are equal.
               
               :param parent: The parent's name.
               :type parent: :class:`ldns_dname`
               :throws TypeError: When `parent` of non-:class:`ldns_rdf`
                   or derived type.
               :return: (bool) True if `this` falls under `parent`, otherwise
                   False.

               .. note::
                   The type checking of parameters is benevolent.
                   It allows also to pass a dname :class:`ldns_rdf` object.
                   This will probably change in future.
            """
            if (not isinstance(parent, ldns_dname)) and \
               isinstance(parent, ldns_rdf) and \
               parent.get_type() == _ldns.LDNS_RDF_TYPE_DNAME:
                warnings.warn("The ldns_dname.is_subdomain() method will" +
                    " drop the support of ldns_rdf."  +
                    " Convert arguments to ldns_dname.",
                    PendingDeprecationWarning, stacklevel=2)
            return _ldns.ldns_dname_is_subdomain(self, parent)
            #parameters: const ldns_rdf *, const ldns_rdf *,
            #retvals: bool

        def label(self, labelpos):
            """
               Look inside the rdf and retrieve a specific label.
               
               The labels are numbered starting from 0 (left most).

               :param labelpos: Index of the label. (Labels are numbered
                   0, which is the left most.)
               :type labelpos: integer
               :throws TypeError: When `labelpos` of non-integer type.
               :return: (:class:`ldns_dname`) A new rdf with the label
                   as name or None on error.
            """
            ret = _ldns.ldns_dname_label(self, labelpos)
            if ret != None:
                ret = ldns_dname(ret, clone=False)
            return ret
            #parameters: const ldns_rdf *, uint8_t,
            #retvals: ldns_rdf *

        def label_count(self):
            """
               Counts the number of labels.
               
               :return: (uint8_t) the number of labels. Will return 0
                   if not a dname.
            """
            return _ldns.ldns_dname_label_count(self)
            #parameters: const ldns_rdf *,
            #retvals: uint8_t

        def left_chop(self):
            """
               Chop one label off the left side of a dname.
               
               (e.g., wwww.nlnetlabs.nl, becomes nlnetlabs.nl)
               
               :return: (:class:`ldns_dname`) The remaining dname or None
                   when error.
            """
            return ldns_dname(_ldns.ldns_dname_left_chop(self), clone=False)
            #parameters: const ldns_rdf *,
            #retvals: ldns_rdf *

        def reverse(self):
            """
               Returns a clone of the given dname with the labels reversed.
               
               :return: (:class:`ldns_dname`) A clone of the dname with
                   the labels reversed.
            """
            return ldns_dname(_ldns.ldns_dname_reverse(self), clone=False)
            #parameters: const ldns_rdf *,
            #retvals: ldns_rdf *

        #
        # _LDNS_DNAME_METHODS
        #
%}
