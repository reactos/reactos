#define foo(x) success
#define bar foo
#define baz bar
#define joe baz
joe (failure)
