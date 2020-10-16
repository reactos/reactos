/* cvector.h --- std::vector-like container in C language.
 * Based on Evan Teran's c-vector: https://github.com/eteran
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Evan Teran.
 * Copyright (c) 2020 Katayama Hirofumi MZ.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef CVECTOR_H_
#define CVECTOR_H_

#ifndef CVECTOR_ASSERT
    #include <assert.h>
    #define CVECTOR_ASSERT assert
#endif

#include <stdlib.h> /* for malloc/realloc/free */
#ifndef CVECTOR_MALLOC
    #define CVECTOR_MALLOC malloc
#endif
#ifndef CVECTOR_REALLOC
    #define CVECTOR_REALLOC realloc
#endif
#ifndef CVECTOR_FREE
    #define CVECTOR_FREE free
#endif

/**
 * @brief cvector_vector_type - The vector type used in this library
 */
#define cvector_vector_type(type) type *

/**
 * @brief cvector_set_capacity - For internal use, sets the capacity variable of the vector
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 */
#define cvector_set_capacity(vec, size)     \
    do {                                    \
        if (vec) {                          \
            ((size_t *)(vec))[-1] = (size); \
        }                                   \
    } while (0)

/**
 * @brief cvector_set_size - For internal use, sets the size variable of the vector
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 */
#define cvector_set_size(vec, size)         \
    do {                                    \
        if (vec) {                          \
            ((size_t *)(vec))[-2] = (size); \
        }                                   \
    } while (0)

/**
 * @brief cvector_capacity - gets the current capacity of the vector
 * @param vec - the vector
 * @return the capacity as a size_t
 */
#define cvector_capacity(vec) \
    ((vec) ? ((size_t *)(vec))[-1] : (size_t)0)

/**
 * @brief cvector_size - gets the current size of the vector
 * @param vec - the vector
 * @return the size as a size_t
 */
#define cvector_size(vec) \
    ((vec) ? ((size_t *)(vec))[-2] : (size_t)0)

/**
 * @brief cvector_empty - returns non-zero if the vector is empty
 * @param vec - the vector
 * @return non-zero if empty, zero if non-empty
 */
#define cvector_empty(vec) \
    (cvector_size(vec) == 0)

/**
 * @brief cvector_reserve - For internal use, ensures that the vector is at least <count> elements big
 * @param vec - the vector
 * @param count - the new capacity to set
 * @return void
 */
#define cvector_reserve(vec, count)                                           \
    do {                                                                      \
        const size_t cv_sz = (count) * sizeof(*(vec)) + (sizeof(size_t) * 2); \
        if (!(vec)) {                                                         \
            size_t *cv_p = CVECTOR_MALLOC(cv_sz);                             \
            assert(cv_p);                                                     \
            (vec) = (void *)(&cv_p[2]);                                       \
            cvector_set_capacity((vec), (count));                             \
            cvector_set_size((vec), 0);                                       \
        } else {                                                              \
            size_t *cv_p1 = &((size_t *)(vec))[-2];                           \
            size_t *cv_p2 = CVECTOR_REALLOC(cv_p1, (cv_sz));                  \
            assert(cv_p2);                                                    \
            (vec) = (void *)(&cv_p2[2]);                                      \
            cvector_set_capacity((vec), (count));                             \
        }                                                                     \
    } while (0)

/**
 * @brief cvector_pop_back - removes the last element from the vector
 * @param vec - the vector
 * @return void
 */
#define cvector_pop_back(vec)                           \
    do {                                                \
        cvector_set_size((vec), cvector_size(vec) - 1); \
    } while (0)

/**
 * @brief cvector_erase - removes the element at index i from the vector
 * @param vec - the vector
 * @param i - index of element to remove
 * @return void
 */
#define cvector_erase(vec, i)                                  \
    do {                                                       \
        if (vec) {                                             \
            const size_t cv_sz = cvector_size(vec);            \
            if ((i) < cv_sz) {                                 \
                cvector_set_size((vec), cv_sz - 1);            \
                size_t cv_x;                                   \
                for (cv_x = (i); cv_x < (cv_sz - 1); ++cv_x) { \
                    (vec)[cv_x] = (vec)[cv_x + 1];             \
                }                                              \
            }                                                  \
        }                                                      \
    } while (0)

/**
 * @brief cvector_free - frees all memory associated with the vector
 * @param vec - the vector
 * @return void
 */
#define cvector_free(vec)                        \
    do {                                         \
        if (vec) {                               \
            size_t *p1 = &((size_t *)(vec))[-2]; \
            CVECTOR_FREE(p1);                    \
        }                                        \
    } while (0)

/**
 * @brief cvector_begin - returns an iterator to first element of the vector
 * @param vec - the vector
 * @return a pointer to the first element (or NULL)
 */
#define cvector_begin(vec) \
    (vec)

/**
 * @brief cvector_end - returns an iterator to one past the last element of the vector
 * @param vec - the vector
 * @return a pointer to one past the last element (or NULL)
 */
#define cvector_end(vec) \
    ((vec) ? &((vec)[cvector_size(vec)]) : NULL)

/* user request to use logarithmic growth algorithm */
#ifdef CVECTOR_LOGARITHMIC_GROWTH

/**
 * @brief cvector_push_back - adds an element to the end of the vector
 * @param vec - the vector
 * @param value - the value to add
 * @return void
 */
#define cvector_push_back(vec, value)                                  \
    do {                                                               \
        size_t cv_cap = cvector_capacity(vec);                         \
        if (cv_cap <= cvector_size(vec)) {                             \
            cvector_reserve((vec), !cv_cap ? cv_cap + 1 : cv_cap * 2); \
        }                                                              \
        vec[cvector_size(vec)] = (value);                              \
        cvector_set_size((vec), cvector_size(vec) + 1);                \
    } while (0)

#else

/**
 * @brief cvector_push_back - adds an element to the end of the vector
 * @param vec - the vector
 * @param value - the value to add
 * @return void
 */
#define cvector_push_back(vec, value)                   \
    do {                                                \
        size_t cv_cap = cvector_capacity(vec);          \
        if (cv_cap <= cvector_size(vec)) {              \
            cvector_reserve((vec), cv_cap + 1);         \
        }                                               \
        vec[cvector_size(vec)] = (value);               \
        cvector_set_size((vec), cvector_size(vec) + 1); \
    } while (0)

#endif /* CVECTOR_LOGARITHMIC_GROWTH */

#endif /* CVECTOR_H_ */
