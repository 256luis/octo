#pragma once

#define OCTO_DEFINE_ARRAY( T )\
    typedef struct OctoArray_##T\
    {\
        u64 length;\
        typeof( T )* data;\
    } OctoArray_##T;\
    T* OctoArray_##T##_at(OctoArray_##T octo_array, u64 index)\
    {\
        return octo_array.data + index;\
    }


typedef signed char        i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef float              f32;
typedef double             f64;

// ( OctoArray_T ){
//     .length = <length>,
//     .data = ( T[<length>] ){
//         <rvalues>
//     }
// };
