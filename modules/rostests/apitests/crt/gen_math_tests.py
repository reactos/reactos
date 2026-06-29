#!/usr/bin/env python3
"""
 PROJECT:     ReactOS CRT
 LICENSE:     MIT (https://spdx.org/licenses/MIT)
 PURPOSE:     Script to generate test data tables for math functions
 COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
"""

from mpmath import mp
import struct
import argparse
import math
import array
import sys

# Set precision (e.g., 100 decimal places)
mp.dps = 100

def ldexp_manual(x_float, exp):
    """
    Compute x_float * 2**exp for a floating-point number, handling denormals and rounding.
    This is a workaround for broken ldexp on Windows (before Win 11), which does not handle denormals correctly.

    Args:
        x_float (float): The floating-point number to scale.
        exp (int): The integer exponent of 2 by which to scale.

    Returns:
        float: The result of x_float * 2**exp, respecting IEEE 754 rules.
    """
    # Handle special cases: zero, infinity, or NaN
    if x_float == 0.0 or not math.isfinite(x_float):
        return x_float

    # Get the 64-bit IEEE 754 representation
    bits = struct.unpack('Q', struct.pack('d', x_float))[0]

    # Extract components
    sign = bits >> 63
    exponent = (bits >> 52) & 0x7FF
    mantissa = bits & 0xFFFFFFFFFFFFF

    if exponent == 0:
        # Denormal number
        if mantissa == 0:
            return x_float  # Zero

        # Normalize it
        shift_amount = 52 - mantissa.bit_length()
        mantissa <<= shift_amount
        exponent = 1 - shift_amount
    else:
        # Normal number, add implicit leading 1
        mantissa |= (1 << 52)

    # Adjust exponent with exp
    new_exponent = exponent + exp

    if new_exponent > 2046:
        # Overflow to infinity
        return math.copysign(math.inf, x_float)
    elif new_exponent <= 0:
        # Denormal or underflow
        if new_exponent < -52:
            # Underflow to zero
            return 0.0

        # Calculate how much we need to shift the mantissa
        mantissa_shift = 1 - new_exponent

        # Get the highest bit, that would be shifted off
        round_bit = (mantissa >> (mantissa_shift - 1)) & 1

        # Adjust mantissa for denormal
        mantissa >>= mantissa_shift
        mantissa += round_bit
        new_exponent = 0

    # Reconstruct the float
    bits = (sign << 63) | (new_exponent << 52) | (mantissa & 0xFFFFFFFFFFFFF)
    return float(struct.unpack('d', struct.pack('Q', bits))[0])

def mpf_to_float(mpf_value):
    """
    Convert an mpmath mpf value to the nearest Python float.
    We use ldexp_manual, because ldexp is broken on Windows.
    """
    sign = mpf_value._mpf_[0]
    mantissa = mpf_value._mpf_[1]
    exponent = mpf_value._mpf_[2]

    result = ldexp_manual(mantissa, exponent)
    if sign == 1:
        result = -result
    return result

def generate_range_of_floats(start, end, steps, typecode):
    if not isinstance(steps, int) or steps < 1:
        raise ValueError("steps must be an integer >= 1")
    if steps == 1:
        return [start]
    step = (end - start) / (steps - 1)
    #return [float(start + i * step) for i in range(steps)]
    return array.array(typecode, [float(start + i * step) for i in range(steps)])

def gen_table_header(func_name):
    print("static TESTENTRY_DBL_APPROX s_" f"{func_name}" "_approx_tests[] =")
    print("{")
    print("//  {    x,                     {    y_rounded,               y_difference           } }")

def gen_table_range(func_name, typecode, func, start, end, steps, max_ulp):

    angles = generate_range_of_floats(start, end, steps, typecode)

    for x in angles:
        # Compute high-precision cosine
        high_prec = func(x)

        # Convert to double (rounds to nearest double)
        rounded_double = mpf_to_float(high_prec)

        # Compute difference
        difference = high_prec - mp.mpf(rounded_double)

        # Print the table line
        print("    { " f"{float(x).hex():>24}"
              ", { " f"{float(rounded_double).hex():>24}"
              ", " f"{float(difference).hex():>24}"
              " }, " f"{max_ulp}"
              " }, // " f"{func_name}" "(" f"{float(x)}" ") == " f"{mp.nstr(high_prec, 20)}")

def generate_acos_table(func_name = "acos", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.acos, -1.0, 1.0, 101, 1)
    print("};\n")

def generate_acosf_table():
    generate_acos_table("acosf", 'f')

def generate_asin_table(func_name = "asin", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.asin, -1.0, 1.0, 101, 1)
    print("};\n")

def generate_asinf_table():
    generate_asin_table("asinf", 'f')

def generate_atan_table(func_name = "atan", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.atan, -10.0, -0.9, 33, 1)
    gen_table_range(func_name, typecode, mp.atan, -1.0, 1.0, 33, 1)
    gen_table_range(func_name, typecode, mp.atan, 1.1, 10.0, 33, 1)
    print("};\n")

def generate_atanf_table():
    generate_atan_table("atanf", 'f')

def generate_cos_table(func_name = "cos", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.cos, -10000*mp.pi, -10200*mp.pi, 9, 1)
    gen_table_range(func_name, typecode, mp.cos, -100*mp.pi, -98*mp.pi, 9, 1)
    gen_table_range(func_name, typecode, mp.cos, -math.pi, math.pi, 57, 1)
    gen_table_range(func_name, typecode, mp.cos, 2000*mp.pi, 2002*mp.pi, 9, 1)
    gen_table_range(func_name, typecode, mp.cos, 2000000*mp.pi, 2070000*mp.pi, 9, 1)
    print("};\n")

def generate_cosf_table():
    generate_cos_table("cosf", 'f')

def generate_exp_table(func_name = "exp", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.exp, -100.0, -0.9, 33, 1)
    gen_table_range(func_name, typecode, mp.exp, -1.0, 1.0, 33, 1)
    gen_table_range(func_name, typecode, mp.exp, 1.1, 100.0, 33, 1)
    print("};\n")

def generate_expf_table():
    generate_exp_table("expf", 'f')

def generate_log_table(func_name = "log", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.log, 0.0, sys.float_info.epsilon, 5, 1)
    gen_table_range(func_name, typecode, mp.log, sys.float_info.epsilon, 0.99, 5, 1)
    gen_table_range(func_name, typecode, mp.log, 1.0, 9.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log, 10.0, 99.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log, 100.0, 999.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log, 1000.0, 9999.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log, 10000.0, 99999.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log, 100000.0, 999999.9, 5, 1)
    print("};\n")

def generate_logf_table():
    generate_log_table("logf", 'f')

def generate_log10_table(func_name = "log10", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.log10, 0.0, sys.float_info.epsilon, 5, 1)
    gen_table_range(func_name, typecode, mp.log10, sys.float_info.epsilon, 0.99, 5, 1)
    gen_table_range(func_name, typecode, mp.log10, 1.0, 9.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log10, 10.0, 99.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log10, 100.0, 999.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log10, 1000.0, 9999.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log10, 10000.0, 99999.9, 5, 1)
    gen_table_range(func_name, typecode, mp.log10, 100000.0, 999999.9, 5, 1)
    print("};\n")

def generate_log10f_table():
    generate_log10_table("log10f", 'f')

def generate_sin_table(func_name = "sin", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.sin, -10000*mp.pi, -10200*mp.pi, 9, 1)
    gen_table_range(func_name, typecode, mp.sin, -100*mp.pi, -98*mp.pi, 9, 1)
    gen_table_range(func_name, typecode, mp.sin, -math.pi, math.pi, 57, 1)
    gen_table_range(func_name, typecode, mp.sin, 2000*mp.pi, 2002*mp.pi, 9, 1)
    gen_table_range(func_name, typecode, mp.sin, 2000000*mp.pi, 2070000*mp.pi, 9, 1)
    print("};\n")

def generate_sinf_table():
    generate_sin_table("sinf", 'f')

def generate_sqrt_table(func_name = "sqrt", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.sqrt, 0.0, 2.2250738585072009e-308, 3, 0)
    gen_table_range(func_name, typecode, mp.sqrt, 2.2250738585072009e-308, 0.99, 17, 0)
    gen_table_range(func_name, typecode, mp.sqrt, 1.0, 9.99, 17, 0)
    gen_table_range(func_name, typecode, mp.sqrt, 10.0, 99.9, 17, 0)
    gen_table_range(func_name, typecode, mp.sqrt, 100.0, 999.9, 17, 0)
    gen_table_range(func_name, typecode, mp.sqrt, 1000.0, 9999.9, 17, 0)
    gen_table_range(func_name, typecode, mp.sqrt, 10000.0, 99999.9, 17, 0)
    print("};\n")

def generate_sqrtf_table():
    generate_sqrt_table("sqrtf", 'f')

def generate_tan_table(func_name = "tan", typecode = 'd'):
    gen_table_header(func_name)
    gen_table_range(func_name, typecode, mp.tan, -0.5 * mp.pi, -0.499, 23, 1)
    gen_table_range(func_name, typecode, mp.tan, -0.5, -2 * sys.float_info.epsilon, 23, 1)
    gen_table_range(func_name, typecode, mp.tan, -sys.float_info.epsilon, sys.float_info.epsilon, 7, 1)
    gen_table_range(func_name, typecode, mp.tan, 2 * sys.float_info.epsilon, 0.5, 23, 1)
    gen_table_range(func_name, typecode, mp.tan, 0.501, 0.5 * mp.pi, 23, 1)
    gen_table_range(func_name, typecode, mp.tan, 10.501 * mp.pi, 11.499 * mp.pi, 5, 1)
    print("};\n")

def generate_tanf_table():
    generate_tan_table("tanf", 'f')

# Dictionary to map math function names to generator functions
TABLE_FUNCTIONS = {
    "acos": generate_acos_table,
    "acosf": generate_acosf_table,
    "asin": generate_asin_table,
    "asinf": generate_asinf_table,
    "atan": generate_atan_table,
    "atanf": generate_atanf_table,
    "cos": generate_cos_table,
    "cosf": generate_cosf_table,
    "exp": generate_exp_table,
    "expf": generate_expf_table,
    "log": generate_log_table,
    "logf": generate_logf_table,
    "log10": generate_log10_table,
    "log10f": generate_log10f_table,
    "sin": generate_sin_table,
    "sinf": generate_sinf_table,
    "sqrt": generate_sqrt_table,
    "sqrtf": generate_sqrtf_table,
    "tan": generate_tan_table,
    "tanf": generate_tanf_table,
}

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Output a specific test table based on the provided function name.")
    parser.add_argument("function_name", help="Name of the function to output the table for (e.g., sin, cos)")
    args = parser.parse_args()

    # Get the table name from the command-line argument
    function_name = args.function_name.lower()

    # Check if the table name is valid
    if function_name in TABLE_FUNCTIONS:
        # Call the corresponding function
        TABLE_FUNCTIONS[function_name]()
    else:
        print(f"Error: Unsupported function '{function_name}'. Available tables: {', '.join(TABLE_FUNCTIONS.keys())}")
        exit(1)

if __name__ == "__main__":
    main()
