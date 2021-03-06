/*
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


#ifndef _TS_LUA_COMMON_H
#define _TS_LUA_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <tcl.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <ts/ts.h>
#include <ts/experimental.h>
#include <ts/remap.h>
#include "ts_lua_coroutine.h"

#define TS_LUA_RELEASE_VERSION                  "v0.1.6"

#define TS_LUA_FUNCTION_REMAP                   "do_remap"
#define TS_LUA_FUNCTION_POST_REMAP              "do_post_remap"
#define TS_LUA_FUNCTION_CACHE_LOOKUP_COMPLETE   "do_cache_lookup_complete"
#define TS_LUA_FUNCTION_SEND_REQUEST            "do_send_request"
#define TS_LUA_FUNCTION_READ_RESPONSE           "do_read_response"
#define TS_LUA_FUNCTION_SEND_RESPONSE           "do_send_response"

#define TS_LUA_EVENT_COROUTINE_CONT             20000

#define TS_LUA_DEBUG_TAG                        "ts_lua"

#define TS_LUA_MAX_SCRIPT_FNAME_LENGTH          1024
#define TS_LUA_MAX_CONFIG_VARS_COUNT            256
#define TS_LUA_MAX_SHARED_DICT_NAME_LENGTH      128
#define TS_LUA_MAX_SHARED_DICT_COUNT            32
#define TS_LUA_MAX_URL_LENGTH                   2048
#define TS_LUA_MAX_OVEC_SIZE                    (3 * 32)
#define TS_LUA_MAX_RESIDENT_PCRE                64

#define TS_LUA_MAKE_VAR_ITEM(X)                 {X, #X}

#define ee(...)     fprintf(stderr, "Lua *** %s: ", __func__); \
                            fprintf(stderr, __VA_ARGS__);   \
                            fprintf(stderr, " @%s:%d\n", __FILE__, __LINE__)


/* for http config or cntl var */
typedef struct {
    int64_t nvar;
    char    *svar;
} ts_lua_var64_item;

typedef struct {
    int     nvar;
    char    *svar;
} ts_lua_var_item;

/* for dict */
typedef struct {
    Tcl_HashTable   t;
    TSMutex         mutexp;
} ts_lua_hash_map;

typedef struct {
    uint16_t        ksize;      // sizeof(long) or strlen(key)
    uint16_t        vsize;      // 0 or strlen(val)

    union
    {
      int64_t  n;
      char     *s;
    } v;

    char            vtype;
} ts_lua_shared_dict_item;

typedef struct {
    char            name[TS_LUA_MAX_SHARED_DICT_NAME_LENGTH];
    ts_lua_hash_map map;
    int64_t         quota;
    int64_t         used;
    int             flags;
    int             initialized:1;
} ts_lua_shared_dict;

/* plugin instance conf */
typedef struct {
    char    *content;
    char    script[TS_LUA_MAX_SCRIPT_FNAME_LENGTH];
    void    *conf_vars[TS_LUA_MAX_CONFIG_VARS_COUNT];

    ts_lua_hash_map     regex_map;
    ts_lua_shared_dict  shdict[TS_LUA_MAX_SHARED_DICT_COUNT];
    int                 shdict_n;

    int                 _first: 1;      // create current instance for 1st ts_lua_main_ctx
    int                 _last: 1;       // create current instance for the last ts_lua_main_ctx

} ts_lua_instance_conf;

/* lua state for http request */
typedef struct {
    ts_lua_cont_info        cinfo;

    TSHttpTxn   txnp;
    TSMBuffer   client_request_bufp;
    TSMLoc      client_request_hdrp;
    TSMLoc      client_request_url;

    TSMBuffer   server_request_bufp;
    TSMLoc      server_request_hdrp;
    TSMLoc      server_request_url;

    TSMBuffer   server_response_bufp;
    TSMLoc      server_response_hdrp;

    TSMBuffer   client_response_bufp;
    TSMLoc      client_response_hdrp;

    TSMBuffer   cached_response_bufp;
    TSMLoc      cached_response_hdrp;

    TSRemapRequestInfo      *rri;
    ts_lua_instance_conf    *instance_conf;
    uint32_t                hooks;

} ts_lua_http_ctx;


typedef struct {
    TSVIO               vio;
    TSIOBuffer          buffer;
    TSIOBufferReader    reader;
} ts_lua_io_handle;

typedef struct {
    ts_lua_cont_info    cinfo;

    ts_lua_io_handle    output;
    ts_lua_io_handle    reserved;

    ts_lua_http_ctx     *hctx;
    int64_t             upstream_bytes;
    int64_t             downstream_bytes;
    int64_t             total;
} ts_lua_http_transform_ctx;

typedef struct {
    ts_lua_cont_info    cinfo;

    ts_lua_io_handle    input;
    ts_lua_io_handle    output;

    TSVConn             net_vc;
    ts_lua_http_ctx     *hctx;

    int64_t             to_flush;
    int                 reuse:1;
    int                 recv_complete:1;
    int                 send_complete:1;
    int                 all_ready:1;
} ts_lua_http_intercept_ctx;


#define TS_LUA_RELEASE_IO_HANDLE(ih) do {   \
    if (ih->reader) {                       \
        TSIOBufferReaderFree(ih->reader);   \
        ih->reader = NULL;                  \
    }                                       \
    if (ih->buffer) {                       \
        TSIOBufferDestroy(ih->buffer);      \
        ih->buffer = NULL;                  \
    }                                       \
} while (0)

#endif
