// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <stdio.h>

class A 
{
  public:
    A(){printf("In A()\n");}
    ~A(){printf("In ~A()\n");}
};

void foo()
{
    printf("Throwing\n");
    throw 1;
}

int main()
{
    try{
	try{
	    A a;
	    foo();
	    goto Label;
	}catch(...){
	    printf("In first catch\nDoing new throw\n");
	    throw 2;
	    goto Label;
	}
    }catch(...){
	printf("In outer catch\n");
    }
    printf("End\n");
Label:;
}
	
