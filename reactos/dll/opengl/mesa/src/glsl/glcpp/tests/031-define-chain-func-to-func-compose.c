#define baz(failure) failure
#define bar(failure) failure
#define foo() bar(baz(success))
foo()
