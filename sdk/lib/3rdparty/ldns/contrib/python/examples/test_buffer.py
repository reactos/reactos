#!/usr/bin/env python

#
# ldns_buffer testing script.
#
# Do not use constructs that differ between Python 2 and 3.
# Use write on stdout or stderr.
#


import ldns
import sys
import os
import inspect


class_name = "ldns_buffer"
method_name = None
error_detected = False


def set_error():
    """
        Writes an error message and sets error flag.
    """
    global class_name
    global method_name
    global error_detected
    error_detected = True
    sys.stderr.write("(line %d): malfunctioning method %s.\n" % \
       (inspect.currentframe().f_back.f_lineno, method_name))
    

# Buffer creation.
capacity = 1024

#if not error_detected:
if True:
    method_name = "ldns_buffer.__init__()"
    try:
        buf = ldns.ldns_buffer(1024)
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.__str__()"
    buf.printf("abcedf")
    try:
        string = buf.__str__()
    except:
        set_error()
    if not isinstance(string, str):
        # Should be string.
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.at()"
    try:
        ret = buf.at(512)
    except:
        set_error()
    try:
        # Must raise TypeError.
        ret = buf.at("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.available()"
    try:
        ret = buf.available(capacity)
    except:
        set_error()
    if not isinstance(ret, bool):
        # Should be bool.
        set_error()
    if not buf.available(capacity):
        # Should return True.
        set_error()
    if buf.available(capacity + 1):
        # Should return False.
        set_error()
    try:
        # Must raise TypeError.
        ret = buf.available("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        # Must raise ValueError.
#        ret = buf.available("")
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.available_at()"
    try:
        ret = buf.available_at(512, capacity - 512)
    except:
        set_error()
    if not isinstance(ret, bool):
        # Should be bool.
        set_error()
    if not buf.available_at(512, capacity - 512):
        # Should return True.
        set_error()
    if buf.available_at(512, capacity - 512 + 1):
        # Should return False.
        set_error()
    try:
        # Must raise TypeError.
        ret = buf.available_at("", 1)
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    try:
        # Must raise TypeError.
        ret = buf.available_at(1, "")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        # Must raise ValueError.
#        ret = buf.available_at(-1, 512)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
#    try:
#        # Must raise ValueError.
#        ret = buf.available_at(512, -1)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.begin()"
    try:
        ret = buf.begin()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.capacity()"
    try:
        ret = buf.capacity()
    except:
        set_error()
    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
        # Should be int.
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.clear()"
    try:
        buf.clear()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.copy()"
    sys.stderr.write("%s not tested.\n" % (method_name))
#    buf2 = ldns.ldns_buffer(10)
#    buf2.printf("abcdef")
#    try:
#        buf.copy(buf2)
#        print buf.capacity()
#        print buf2.capacity()
#    except:
#        set_error()
#    buf.printf("2")
#    print buf


#if not error_detected:
if True:
    method_name = "ldns_buffer.current()"
    try:
        ret = buf.current()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.end()"
    try:
        ret = buf.end()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.export()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_buffer.flip()"
    buf.printf("abcdef")
    try:
        buf.flip()
    except:
        set_error()
#    if buf.remaining() != capacity:
#        # Should be at beginning.
#        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.getc()"
    buf.printf("a")
    buf.rewind()
    try:
        ret = buf.getc()
    except:
        set_error()
    if ret != ord("a"):
        set_error()
# Test return value for -1
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.invariant()"
    try:
        buf.invariant()
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.limit()"
    try:
        ret = buf.limit()
    except:
        set_error()
    if ret != capacity:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.position()"
    try:
        ret = buf.position()
    except:
        set_error()
    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.printf()"
    try:
        ret = buf.printf("abcdef")
    except:
        set_error()
    if not isinstance(ret, int):
        set_error()
    try:
        ret = buf.printf(10)
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.read()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_buffer.read_at()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_buffer.read_u16()"
    buf.printf("aac")
    buf.rewind()
    try:
        ret = buf.read_u16()
    except:
        set_error()
    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
        set_error()
    if ret != (ord("a") * 0x0101):
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.read_u16_at()"
    buf.printf("abbc")
    try:
        ret = buf.read_u16_at(1)
    except:
        set_error()
    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
        set_error()
    if ret != (ord("b") * 0x0101):
        set_error()
    try:
        ret = buf.read_u16_at("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        ret = buf.read_u16_at(-1)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.read_u32()"
    buf.printf("aaaac")
    buf.rewind()
    try:
        ret = buf.read_u32()
    except:
        set_error()
    if not isinstance(ret, int):
        set_error()
    if ret != (ord("a") * 0x01010101):
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.read_u32_at()"
    buf.printf("abbbbc")
    try:
        ret = buf.read_u32_at(1)
    except:
        set_error()
    if not isinstance(ret, int):
        set_error()
    if ret != (ord("b") * 0x01010101):
        set_error()
    try:
        ret = buf.read_u32_at("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        ret = buf.read_u32_at(-1)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.read_u8()"
    buf.printf("ac")
    buf.rewind()
    try:
        ret = buf.read_u8()
    except:
        set_error()
    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
        set_error()
    if ret != ord("a"):
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.read_u8_at()"
    buf.printf("abc")
    try:
        ret = buf.read_u8_at(1)
    except:
        set_error()
    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
        set_error()
    if ret != ord("b"):
        set_error()
    try:
        ret = buf.read_u8_at("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        ret = buf.read_u8_at(-1)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.remaining()"
    buf.printf("abcdef")
    try:
        ret = buf.remaining()
    except:
        set_error()
    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
        set_error()
    if ret != (capacity - 6):
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.remaining_at()"
    buf.printf("abcdef")
    try:
        ret = buf.remaining_at(1)
    except:
        set_error()
    if (not isinstance(ret, int)) and (not isinstance(ret, long)):
        set_error()
    if ret != (capacity - 1):
        set_error()
    try:
        ret = buf.remaining_at("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        ret = buf.remaining_at(-1)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.reserve()"
    buf2 = ldns.ldns_buffer(512)
    try:
        ret = buf2.reserve(1024)
    except:
        set_error()
    if not isinstance(ret, bool):
        set_error()
    try:
        ret = buf2.reserve("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        ret = buf2.reserve(-1)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.rewind()"
    buf.printf("abcdef")
    try:
        buf.rewind()
    except:
        set_error()
    if buf.position() != 0:
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.set_capacity()"
    try:
        ret = buf.set_capacity(capacity)
    except:
        set_error()
    if not isinstance(ret, bool):
        set_error()
    try:
        ret = buf.set_capacity("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        ret = buf.set_capacity(-1)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.set_limit()"
    try:
        buf.set_limit(0)
    except:
        set_error()
    try:
        buf.set_limit("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        buf.set_limit(-1)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    buf.clear()
    

#if not error_detected:
if True:
    method_name = "ldns_buffer.set_position()"
    try:
        buf.set_position(0)
    except:
        set_error()
    try:
        buf.set_position("")
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        buf.set_position(-1)
#    except ValueError:
#        pass
#    except:
#        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.skip()"
    try:
        buf.skip(10)
    except:
        set_error()
    try:
        buf.skip(-1)
    except:
        set_error()
    try:
        buf.skip("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.status()"
    try:
        ret = buf.status()
    except:
        set_error()
    # Returned status is an integer.
    if not isinstance(ret, int):
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.status_ok()"
    try:
        ret = buf.status_ok()
    except:
        set_error()
    if not isinstance(ret, bool):
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.write()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_at()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_string()"
    try:
        buf.write_string("abcdef")
    except:
        set_error()
#    try:
#        buf.write_sring(-1)
#        set_error()
#    except TypeError:
#        pass
#    except:
#        set_error()
    sys.stderr.write("%s not tested for parameter correctness.\n" % \
        (method_name))
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_string_at()"
    sys.stderr.write("%s not tested.\n" % (method_name))


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_u16()"
    try:
        buf.write_u16(ord("b") * 0x0101)
    except:
        set_error()
    try:
        buf.write_u16("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_u16_at()"
    buf.printf("a")
    try:
        buf.write_u16_at(1, ord("b") * 0x0101)
    except:
        set_error()
    try:
        buf.write_u16_at("", ord("b") * 0x0101)
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        buf.write_u16_at(-1, ord("b") * 0x0101)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    try:
        buf.write_u16_at(1, "")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_u32()"
    try:
        buf.write_u32(ord("b") * 0x01010101)
    except:
        set_error()
    try:
        buf.write_u32("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_u32_at()"
    buf.printf("a")
    try:
        buf.write_u32_at(1, ord("b") * 0x01010101)
    except:
        set_error()
    try:
        buf.write_u32_at("", ord("b") * 0x01010101)
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        buf.write_u32_at(-1, ord("b") * 0x01010101)
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    try:
        buf.write_u32_at(1, "")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_u8()"
    try:
        buf.write_u8(ord("b"))
    except:
        set_error()
    try:
        buf.write_u8("")
        set_error()
    except TypeError:
        pass
    except:
        set_error()
    buf.clear()


#if not error_detected:
if True:
    method_name = "ldns_buffer.write_u8_at()"
    buf.printf("a")
    try:
        buf.write_u8_at(1, ord("b"))
    except:
        set_error()
    try:
        buf.write_u8_at("", ord("b"))
        set_error()
    except TypeError:
        pass
    except:
        set_error()
#    try:
#        buf.write_u8_at(-1, ord("b"))
#        set_error()
#    except ValueError:
#        pass
#    except:
#        set_error()
    try:
        buf.write_u8_at(1, "")
        set_error()
    except TypeError:
        pass
    except:
        set_error()


if not error_detected:
    sys.stdout.write("%s: passed.\n" % (os.path.basename(__file__)))
else:
    sys.stdout.write("%s: errors detected.\n" % (os.path.basename(__file__)))
    sys.exit(1)
