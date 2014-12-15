#ifndef _TEST_ADD_H
#define _TEST_ADD_H


#include <stdint.h>
#include <va/va_version.h>

/* This is meaningless. But it is really needed when linking to C++ library
 * if there is no other *.cpp file under the directory of src. Otherwise
 * the C++ library can't be linked. (I don't know the reason).
 *
 * After another *.cpp file is added under src, this can be removed.
 * Anyway, it can be kept
 */
#ifdef __cplusplus
extern "C" {
#endif

extern void test_add_function(int x, int y);

#ifdef __cplusplus
}
#endif
#endif
