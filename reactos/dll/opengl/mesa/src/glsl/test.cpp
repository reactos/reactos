/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file test.cpp
 *
 * Standalone tests for the GLSL compiler.
 *
 * This file provides a standalone executable which can be used to
 * test components of the GLSL.
 *
 * Each test is a function with the same signature as main().  The
 * main function interprets its first argument as the name of the test
 * to run, strips out that argument, and then calls the test function.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_optpass.h"

/**
 * Print proper usage and exit with failure.
 */
static void
usage_fail(const char *name)
{
   printf("*** usage: %s <command> <options>\n", name);
   printf("\n");
   printf("Possible commands are:\n");
   printf("  optpass: test an optimization pass in isolation\n");
   exit(EXIT_FAILURE);
}

static const char *extract_command_from_argv(int *argc, char **argv)
{
   if (*argc < 2) {
      usage_fail(argv[0]);
   }
   const char *command = argv[1];
   --*argc;
   memmove(&argv[1], &argv[2], (*argc) * sizeof(argv[1]));
   return command;
}

int main(int argc, char **argv)
{
   const char *command = extract_command_from_argv(&argc, argv);
   if (strcmp(command, "optpass") == 0) {
      return test_optpass(argc, argv);
   } else {
      usage_fail(argv[0]);
   }

   /* Execution should never reach here. */
   return EXIT_FAILURE;
}
