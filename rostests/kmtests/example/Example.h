/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Example Test declarations
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */
 
#ifndef _KMTEST_EXAMPLE_H_
#define _KMTEST_EXAMPLE_H_

typedef struct
{
    int a;
    char b[8];
} MY_STRUCT, *PMY_STRUCT;

#define IOCTL_NOTIFY        1
#define IOCTL_SEND_STRING   2
#define IOCTL_SEND_MYSTRUCT 3

#endif /* !defined _KMTEST_EXAMPLE_H_ */
