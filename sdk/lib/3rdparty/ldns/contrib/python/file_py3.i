/*
 * file_py3.i: Typemaps for FILE* for Python 3
 *
 * Copyright (c) 2011, Karel Slany (karel.slany AT nic.cz)
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
 *       contributors may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
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
 */

%{
#include <unistd.h>
#include <fcntl.h>
%}

%types(FILE *);

/* converts basic file descriptor flags onto a string */
%fragment("fdfl_to_str", "header") {
const char *
fdfl_to_str(int fdfl) {

  static const char * const file_mode[] = {"w+", "w", "r"};

  if (fdfl & O_RDWR) {
    return file_mode[0];
  } else if (fdfl & O_WRONLY) {
    return file_mode[1];
  } else {
    return file_mode[2];
  }
}
}

%fragment("obj_to_file","header", fragment="fdfl_to_str") {
FILE *
obj_to_file(PyObject *obj) {
%#if PY_VERSION_HEX >= 0x03000000
  int fd, fdfl;
  FILE *fp;
  if (!PyLong_Check(obj) &&                                /* is not an integer */
      PyObject_HasAttrString(obj, "fileno") &&             /* has fileno method */
      (PyObject_CallMethod(obj, "flush", NULL) != NULL) && /* flush() succeeded */
      ((fd = PyObject_AsFileDescriptor(obj)) != -1) &&     /* got file descriptor */
      ((fdfl = fcntl(fd, F_GETFL)) != -1)                  /* got descriptor flags */
    ) {
    fp = fdopen(dup(fd), fdfl_to_str(fdfl)); /* the FILE* must be flushed
                                                and closed after being used */
#ifdef SWIG_FILE3_DEBUG
    fprintf(stderr, "opening fd %d (fl %d \"%s\") as FILE %p\n",
            fd, fdfl, fdfl_to_str(fdfl), (void *)fp);
#endif
    return fp;
  }
%#endif
  return NULL;
}
}

/* returns -1 if error occurred */
%fragment("dispose_file", "header") {
int
dispose_file(FILE **fp) {
#ifdef SWIG_FILE3_DEBUG
  fprintf(stderr, "flushing FILE %p\n", (void *)fp);
#endif
  if (*fp == NULL) {
    return 0;
  }
  if ((fflush(*fp) == 0) &&  /* flush file */
      (fclose(*fp) == 0)) {  /* close file */
    *fp = NULL;
    return 0;
  }
  return -1;
}
}

%typemap(arginit, noblock = 1) FILE* {
  $1 = NULL;
}

%typemap(check, noblock = 1) FILE* {
  if ($1 == NULL) {
    /* The generated wrapper function raises TypeError on mismatching types. */
    SWIG_exception_fail(SWIG_TypeError, "in method '" "$symname" "', argument "
                        "$argnum"" of type '" "$type""'");
  }
}

%typemap(in, noblock = 1, fragment = "obj_to_file") FILE* {
  $1 = obj_to_file($input);
}

%typemap(freearg, noblock = 1, fragment = "dispose_file") FILE* {
  if (dispose_file(&$1) == -1) {
    SWIG_exception_fail(SWIG_IOError, "closing file in method '" "$symname" "', argument "
                        "$argnum"" of type '" "$type""'");
  }
}
