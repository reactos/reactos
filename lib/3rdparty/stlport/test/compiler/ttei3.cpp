/*
 * It is known that this code not compiled by following compilers:
 *   gcc 2.95.3
 *   gcc 3.3.3
 *   gcc 3.4.1
 *   gcc 4.1.1
 *
 * It is known that this code compiled by following compilers:
 *
 *   MSVC 6
 *   MSVC 8 Beta
 */

/*
 * Indeed this code is wrong: explicit template specialization
 * have to appear out-of-class.
 *
 */

struct A
{
  private:
    struct B
    {
        template <typename T>
        static void f( T& ) {}

        template <bool V>
        struct C
        {
            template <typename T>
            static void f( T& ) {}
        };

        template <>
        struct C<true>
        {
            template <typename T>
            static void f( T& ) {}
        };
    };
};

