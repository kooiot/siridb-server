/*
 * qpack.h - efficient binary serialization format
 *
 * author       : Jeroen van der Heijden
 * email        : jeroen@transceptor.technology
 * copyright    : 2016, Transceptor Technology
 *
 * changes
 *  - initial version, 11-03-2016
 *
 */
#pragma once

#include <inttypes.h>
#include <stdlib.h>

#define QP_SUGGESTED_SIZE 65536

#define QP_OPEN_ARRAY 252
#define QP_CLOSE_ARRAY 253
#define QP_OPEN_MAP 254
#define QP_CLOSE_MAP 255

typedef enum
{
    /*
     * Values with -##- will never be returned while unpacking. For example
     * a QP_INT8 (1 byte signed integer) will be returned as QP_INT64.
     */
    QP_ERR=-1,          // when an error occurred
    QP_END,             // at the end while unpacking
    QP_RAW,             // raw string
    /*
     * Both END and RAW are never actually packed but 0 and 1 are reseverd
     * for positive signed integers.
     *
     * Fixed positive integers from 0 till 63       [  0...63  ]
     *
     * Fixed negative integers from -61 till -1     [ 64...124 ]
     *
     */
    QP_DOUBLE_N1=125,   // ## double value -1.0
    QP_DOUBLE_0,        // ## double value 0.0
    QP_DOUBLE_1,        // ## double value 1.0
    /*
     * Fixed raw strings lengths from 0 till 99     [ 128...227 ]
     */
    QP_RAW8=228,        // ## raw string with length < 1 byte
    QP_RAW16,           // ## raw string with length < 1 byte
    QP_RAW32,           // ## raw string with length < 1 byte
    QP_RAW64,           // ## raw string with length < 1 byte
    QP_INT8,            // ## 1 byte signed integer
    QP_INT16,           // ## 2 byte signed integer
    QP_INT32,           // ## 4 byte signed integer
    QP_INT64,           // 8 bytes signed integer
    QP_DOUBLE,          // 8 bytes double
    QP_ARRAY0,          // empty array
    QP_ARRAY1,          // array with 1 item
    QP_ARRAY2,          // array with 2 items
    QP_ARRAY3,          // array with 3 items
    QP_ARRAY4,          // array with 4 items
    QP_ARRAY5,          // array with 5 items
    QP_MAP0,            // empty map
    QP_MAP1,            // map with 1 item
    QP_MAP2,            // map with 2 items
    QP_MAP3,            // map with 3 items
    QP_MAP4,            // map with 4 items
    QP_MAP5,            // map with 5 items
    QP_TRUE,            // boolean true
    QP_FALSE,           // boolean false
    QP_NULL,            // null (none, nil)
    QP_ARRAY_OPEN,      // open a new array
    QP_MAP_OPEN,        // open a new map
    QP_ARRAY_CLOSE,     // close array
    QP_MAP_CLOSE,       // close map
} qp_types_t;

typedef union qp_via_u
{
    int64_t int64;
    double real;
    const char * raw;
} qp_via_t;

typedef struct qp_obj_s
{
    uint8_t tp;
    size_t len;
    qp_via_t * via;
} qp_obj_t;

typedef struct qp_unpacker_s
{
    char * source; // can be NULL or a copy or the source
    const char * pt;
    const char * end;
    qp_obj_t * qp_obj;
} qp_unpacker_t;

typedef struct qp_packer_s
{
    size_t len;
    size_t buffer_size;
    size_t alloc_size;
    char * buffer;
} qp_packer_t;

qp_packer_t * qp_new_packer(size_t alloc_size);

void qp_free_packer(qp_packer_t * packer);

void qp_extend_packer(qp_packer_t * packer, qp_packer_t * source);

qp_unpacker_t * qp_new_unpacker(const char * pt, size_t len);

qp_unpacker_t * qp_from_file_unpacker(const char * fn);


void qp_free_unpacker(qp_unpacker_t * unpacker);

/* do not forget to run qp_free_object() on the returned object */
qp_obj_t * qp_copy_object(qp_unpacker_t * unpacker);

/* do not forget to run qp_free_object() on the given object */
qp_types_t qp_copy_next_object(qp_unpacker_t * unpacker, qp_obj_t ** qp_obj);

void qp_free_object(qp_obj_t * qp_obj);

qp_types_t qp_next_object(qp_unpacker_t * unpacker);

void qp_print(const char * pt, size_t len);

/* qp test functions */
int qp_is_array(qp_types_t tp);
int qp_is_map(qp_types_t tp);

/* add functions */
void qp_add_raw(qp_packer_t * packer, const char * raw, size_t len);
void qp_add_raw_term(qp_packer_t * packer, const char * raw, size_t len);
void qp_add_string(qp_packer_t * packer, const char * str);
void qp_add_string_term(qp_packer_t * packer, const char * str);
void qp_add_double(qp_packer_t * packer, double real);
void qp_add_int8(qp_packer_t * packer, int8_t integer);
void qp_add_int16(qp_packer_t * packer, int16_t integer);
void qp_add_int32(qp_packer_t * packer, int32_t integer);
void qp_add_int64(qp_packer_t * packer, int64_t integer);
void qp_add_array0(qp_packer_t * packer);
void qp_add_array1(qp_packer_t * packer);
void qp_add_array2(qp_packer_t * packer);
void qp_add_array3(qp_packer_t * packer);
void qp_add_array4(qp_packer_t * packer);
void qp_add_array5(qp_packer_t * packer);
void qp_add_map0(qp_packer_t * packer);
void qp_add_map1(qp_packer_t * packer);
void qp_add_map2(qp_packer_t * packer);
void qp_add_map3(qp_packer_t * packer);
void qp_add_map4(qp_packer_t * packer);
void qp_add_map5(qp_packer_t * packer);
void qp_add_true(qp_packer_t * packer);
void qp_add_false(qp_packer_t * packer);
void qp_add_null(qp_packer_t * packer);

/* also *add* functions but somewhat special */
void qp_array_open(qp_packer_t * packer);
void qp_map_open(qp_packer_t * packer);

/* its not need to close an array or map but you might need it
 * to tell qp that one is closed.
 */
void qp_array_close(qp_packer_t * packer);
void qp_map_close(qp_packer_t * packer);

/* adds a format string to the packer, but take in account that only
 * QPACK_MAX_FMT_SIZE characters are supported. (rest will be cut off)
 */
void qp_add_fmt(qp_packer_t * packer, const char * fmt, ...);
