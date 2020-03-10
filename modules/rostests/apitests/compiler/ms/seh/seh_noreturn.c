// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

// SEH + no-return (explicit)

__declspec(noreturn) int bar(int);

void foo(int arg) { bar(arg); }

int filter() { return bar(3); }

void moo1(int arg) {
  __try { bar(arg); }
  __except(filter()) { bar(1); }

  bar(0);
}

void moo2(int arg) {
  __try { bar(arg); }
  __finally { bar(2); }

  bar(0);
}
