// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <stdio.h>

typedef void (*OperationF)();
void TestFunction() { throw 1; }
void RethrowIt() { throw; }

int HandleRethrown1(OperationF perform) {
    int ret = 0;
    try {
        perform();
        return 0;
    } catch (...) {
        // ellipsis catch
        try {
            RethrowIt(); 
        } catch (...) {
            ret = 1;
        }
    }

    return ret;
}

int HandleRethrown2(OperationF perform) {
    int ret = 0;
    try {
        perform();
        return 0;
    } catch (int j) {
        // non-ellipsis catch
        try {
            RethrowIt(); 
        } catch (int i) {
            ret = i;
        }
    }

    return ret;
}

int main() {
    int exit_code1;
    int exit_code2;
    exit_code1 = HandleRethrown1(TestFunction);
    exit_code2 = HandleRethrown2(TestFunction);

    if (exit_code1 == 1 && exit_code2 == 1) {
        printf("passed");
    } else {
        printf("failed");
    }

    return 0;
}
