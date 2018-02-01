/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: common.proto */

#ifndef PROTOBUF_C_common_2eproto__INCLUDED
#define PROTOBUF_C_common_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1002001 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _PhiCmd PhiCmd;
typedef struct _Cookie Cookie;


/* --- enums --- */


/* --- messages --- */

struct  _PhiCmd
{
  ProtobufCMessage base;
  uint32_t type;
  uint32_t arg_count;
  int32_t phi_result_code;
  protobuf_c_boolean has_phi_errorno;
  int32_t phi_errorno;
  size_t n_int_args;
  int32_t *int_args;
  size_t n_uint_args;
  uint32_t *uint_args;
  size_t n_extra_args;
  ProtobufCBinaryData *extra_args;
};
#define PHI_CMD__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&phi_cmd__descriptor) \
    , 0, 0, 0, 0,0, 0,NULL, 0,NULL, 0,NULL }


struct  _Cookie
{
  ProtobufCMessage base;
  uint32_t type;
  PhiCmd *phi_cmd;
};
#define COOKIE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&cookie__descriptor) \
    , 0, NULL }


/* PhiCmd methods */
void   phi_cmd__init
                     (PhiCmd         *message);
size_t phi_cmd__get_packed_size
                     (const PhiCmd   *message);
size_t phi_cmd__pack
                     (const PhiCmd   *message,
                      uint8_t             *out);
size_t phi_cmd__pack_to_buffer
                     (const PhiCmd   *message,
                      ProtobufCBuffer     *buffer);
PhiCmd *
       phi_cmd__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   phi_cmd__free_unpacked
                     (PhiCmd *message,
                      ProtobufCAllocator *allocator);
/* Cookie methods */
void   cookie__init
                     (Cookie         *message);
size_t cookie__get_packed_size
                     (const Cookie   *message);
size_t cookie__pack
                     (const Cookie   *message,
                      uint8_t             *out);
size_t cookie__pack_to_buffer
                     (const Cookie   *message,
                      ProtobufCBuffer     *buffer);
Cookie *
       cookie__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   cookie__free_unpacked
                     (Cookie *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*PhiCmd_Closure)
                 (const PhiCmd *message,
                  void *closure_data);
typedef void (*Cookie_Closure)
                 (const Cookie *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor phi_cmd__descriptor;
extern const ProtobufCMessageDescriptor cookie__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_common_2eproto__INCLUDED */
