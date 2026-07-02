/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-07-02T11:22:37+0800
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#include "ai_lite_inspect.h"
#include "ai_platform_interface.h"
#include "layers.h"
#include "core_convert.h"
#include "network.h"
#include "network_details.h"
#include "network_data.h"
#include "stai_events.h"

#include "lite_operators.h"

#include "ai_lite_inspect.h"
/*****************************************************************************/
#define STAI_INTERNAL_API_MAJOR               (1)
#define STAI_INTERNAL_API_MINOR               (0)
#define STAI_INTERNAL_API_MICRO               (0)

#define STAI_MAGIC                            (0xB1C00100)

/*****************************************************************************/
#define _STAI_CONCAT_ARG(a, b)     a ## b
#define STAI_CONCAT(a, b)         _STAI_CONCAT_ARG(a, b)

/*!  STAI_CAST SECTION                       *********************************/
#define STAI_CAST(type, expr) \
  ((type)(expr))


/*****************************************************************************/
#define STAI_SIZE(_size) \
  ((stai_size)(_size))

/*****************************************************************************/
#define STAI_INIT_BUFFER(_flags, _size, _address) \
  { \
    .size = (_size), \
    .address = (uintptr_t)(_address), \
    .flags = (_flags), \
  }

#define STAI_INIT_TENSOR(_name, _flags, _fmt, _size_bytes, _shape, _scale, _zeropoint) \
  { \
    .size_bytes = (_size_bytes), \
    .flags = (_flags), \
    .format = (stai_format)(_fmt), \
    .shape = STAI_PACK(_shape), \
    .scale = STAI_PACK(_scale), \
    .zeropoint = STAI_PACK(_zeropoint), \
    .name = (_name) \
  }

#define STAI_INIT_ARRAY(_size, _ptr) \
  { .size = STAI_SIZE(_size), .data = STAI_PACK(_ptr) }


#define STAI_CAST_ARRAY(_type, _size, _ptr) \
  { .size = STAI_SIZE(_size), .data = (_type)STAI_PACK(_ptr) }


#define STAI_DECLARE_ARRAY(_type, _size, ...) \
  { .size = STAI_SIZE(_size), .data = (_type[_size]) { STAI_PACK(__VA_ARGS__) } }


#define STAI_EMPTY_ARRAY() \
  { .size = 0, .data = NULL }


#define STAI_INIT_VERSION(_major, _minor, _micro) \
  { .major = (_major), .minor = (_minor), .micro = (_micro), .reserved = 0x0 }

/*****************************************************************************/
/**  Getters and setters  **/

#define STAI_GET_ARRAY_SIZE(nd_array) \
  (nd_array.size)


#define STAI_GET_ARRAY_ELEM(nd_array, pos) \
  (nd_array.data[(pos)])

#define _STAI_SET_ERROR(net_ctx, cond, value, exit) { \
  if (!(net_ctx)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE; } \
  if (((uintptr_t)net_ctx) & (_STAI_CONTEXT_ALIGNMENT-1)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_ALIGNMENT; } \
  if (((value) >= STAI_ERROR_GENERIC) && (cond)) { \
    if ((net_ctx)->_return_code == STAI_SUCCESS) { \
      (net_ctx)->_return_code = (value); \
    } \
    return (exit); \
  } \
}

/*****************************************************************************/
/* TODO REMOVE THESE TWO MACROS */
#define STAI_EVENT_NODE_START_CB
#define STAI_EVENT_NODE_STOP_CB

#ifdef STAI_EVENT_NODE_START_CB
#ifndef _STAI_NETWORK_EVENT_NODE_START_CB
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _start_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(const stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_START, (const void*)&_start_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_START_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_START_CB */

#ifdef STAI_EVENT_NODE_STOP_CB
#ifndef _STAI_NETWORK_EVENT_NODE_STOP_CB
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _stop_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_STOP, (const void*)&_stop_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_STOP_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_STOP_CB */


/*****************************************************************************/
#define _STAI_NETWORK_MODEL_SIGNATURE     "0x14e562299b91d6f1a77e8845bcbe4972"
#define _STAI_NETWORK_DATETIME            "2026-07-02T11:22:37+0800"
#define _STAI_NETWORK_COMPILE_DATETIME    __DATE__ " " __TIME__

#define _STAI_CONTEXT_ALIGNMENT        STAI_NETWORK_CONTEXT_ALIGNMENT

/*****************************************************************************/
#define g_network_activations_1     (NULL)




#if defined(HAVE_NETWORK_INFO)
/*****************************************************************************/
static const stai_network_info g_network_info = {
  .model_signature = _STAI_NETWORK_MODEL_SIGNATURE,
  .c_compile_datetime = _STAI_NETWORK_COMPILE_DATETIME,
  .c_model_name = STAI_NETWORK_MODEL_NAME,
  .c_model_datetime = _STAI_NETWORK_DATETIME,
  .c_model_signature = 0x0,
  .runtime_version = STAI_INIT_VERSION(12, 0, 1),
  .tool_version = STAI_INIT_VERSION(4, 0, 1),
  .api_version = STAI_INIT_VERSION(1, 0, 0),
  .n_macc = STAI_NETWORK_MACC_NUM,
  .n_nodes = STAI_NETWORK_NODES_NUM,
  .flags = STAI_NETWORK_FLAGS,
  .n_inputs = STAI_NETWORK_IN_NUM,
  .n_outputs = STAI_NETWORK_OUT_NUM,
  .n_activations = STAI_NETWORK_ACTIVATIONS_NUM,
  .n_weights = STAI_NETWORK_WEIGHTS_NUM,
  .n_states = STAI_NETWORK_STATES_NUM,
  .inputs = (stai_tensor[STAI_NETWORK_IN_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_IN_1_NAME,
      STAI_NETWORK_IN_1_FLAGS,
      STAI_NETWORK_IN_1_FORMAT,
      STAI_NETWORK_IN_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 3, 224, 224),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
    .outputs = (stai_tensor[STAI_NETWORK_OUT_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_1_NAME,
      STAI_NETWORK_OUT_1_FLAGS,
      STAI_NETWORK_OUT_1_FORMAT,
      STAI_NETWORK_OUT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 2, 1, 8),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .activations = (stai_tensor[STAI_NETWORK_ACTIVATIONS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_ACTIVATION_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_ACTIVATION_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2007040),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 6145144),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },

  .states = NULL
};
#endif

#define _STAI_CONTEXT_ACQUIRE(_net_ctx, _net_handle) \
  _stai_network_context* _net_ctx = (_stai_network_context*)(_net_handle); \
  STAI_ASSERT(_net_ctx != NULL) \
  _STAI_SET_ERROR(_net_ctx, _net_ctx->_magic != STAI_MAGIC, \
                  STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE, _net_ctx->_return_code)


/*****************************************************************************/
static
void _stai_network_check(_stai_network_context* net_ctx)
{
  stai_size idx;

// Check activations status
  for (idx=0; idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    if (net_ctx->_activations[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_ACTIVATIONS_NUM) ? STAI_FLAG_ACTIVATIONS : STAI_FLAG_NONE;
// Check inputs status
  for (idx=0; idx<STAI_NETWORK_IN_NUM; idx++) {
    if (net_ctx->_inputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_IN_NUM) ? STAI_FLAG_INPUTS : STAI_FLAG_NONE;

  // Check outputs status
  for (idx=0; idx<STAI_NETWORK_OUT_NUM; idx++) {
    if (net_ctx->_outputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_OUT_NUM) ? STAI_FLAG_OUTPUTS : STAI_FLAG_NONE;

// Check weights status
  for (idx=0; idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    if (net_ctx->_weights[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_WEIGHTS_NUM) ? STAI_FLAG_WEIGHTS : STAI_FLAG_NONE;
STAI_PRINT("  [_stai_network_check] flags: 0x%08x\n", net_ctx->_flags)
}


/*****************************************************************************/
STAI_API_ENTRY
stai_return_code stai_network_init(
  stai_network* network)
{
  /* Memory where to store internal context is provided by applications as a raw byte buffer */
  _stai_network_context* net_ctx = (_stai_network_context*)(network);
  net_ctx->_return_code = STAI_SUCCESS;
  STAI_PRINT("[Entering Network Init] network(%p) context_size(%d)\n", net_ctx, (int32_t)sizeof(_stai_network_context))

  _STAI_SET_ERROR(net_ctx, STAI_NETWORK_CONTEXT_SIZE != sizeof(_stai_network_context),
                 STAI_ERROR_NETWORK_INVALID_CONTEXT_SIZE, net_ctx->_return_code)

  {
    const _stai_network_context _network_context = {
      ._magic = STAI_MAGIC,
      ._signature = STAI_NETWORK_MODEL_SIGNATURE,
      ._flags = STAI_NETWORK_FLAGS,
      ._return_code = STAI_SUCCESS,
      ._callback = NULL,
      ._callback_cookie = NULL,
      ._activations = {
      (stai_ptr)g_network_activations_1
      },
      ._weights = {
      (stai_ptr)g_network_weights_array
      },
      ._inputs = {
    NULL},
      ._outputs = {
    NULL},
    };

    // Deep copy of internal context to opaque buffer provided by app
    *net_ctx = _network_context;

    _stai_network_check(net_ctx);
  }

  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_deinit(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /*  Reset flags to initial state  */
  net_ctx->_flags = STAI_NETWORK_FLAGS;
  return net_ctx->_return_code;
}

/*****************************************************************************/





/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  images_output_array, AI_ARRAY_FORMAT_FLOAT|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 150528, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  images_Transpose_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 150528, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  _model_0_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 200704, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  _model_0_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 200704, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  _model_0_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 200704, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  _model_1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  _model_1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  _model_1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_Split_output_0_output0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_Split_output_0_output1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_Split_output_0_num_or_size_splits_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 1, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_m_0_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_m_0_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_m_0_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_m_0_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_m_0_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_m_0_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#20 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_m_0_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#21 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_Concat_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 150528, AI_STATIC)

/* Array#22 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 200704, AI_STATIC)

/* Array#23 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 200704, AI_STATIC)

/* Array#24 */
AI_ARRAY_OBJ_DECLARE(
  _model_2_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 200704, AI_STATIC)

/* Array#25 */
AI_ARRAY_OBJ_DECLARE(
  _model_3_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#26 */
AI_ARRAY_OBJ_DECLARE(
  _model_3_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#27 */
AI_ARRAY_OBJ_DECLARE(
  _model_3_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#28 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#29 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#30 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 50176, AI_STATIC)

/* Array#31 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_Split_output_0_output0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#32 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_Split_output_0_output1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#33 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_Split_output_0_num_or_size_splits_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 1, AI_STATIC)

/* Array#34 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_m_0_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#35 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_m_0_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#36 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_m_0_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#37 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_m_0_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#38 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_m_0_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#39 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_m_0_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#40 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_m_0_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#41 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_Concat_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 75264, AI_STATIC)

/* Array#42 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#43 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#44 */
AI_ARRAY_OBJ_DECLARE(
  _model_4_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 100352, AI_STATIC)

/* Array#45 */
AI_ARRAY_OBJ_DECLARE(
  _model_5_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#46 */
AI_ARRAY_OBJ_DECLARE(
  _model_5_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#47 */
AI_ARRAY_OBJ_DECLARE(
  _model_5_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#48 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#49 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#50 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#51 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_Split_output_0_output0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#52 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_Split_output_0_output1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#53 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_Split_output_0_num_or_size_splits_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 1, AI_STATIC)

/* Array#54 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#55 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#56 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#57 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#58 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#59 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#60 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#61 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#62 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#63 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#64 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#65 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#66 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_0_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#67 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#68 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#69 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#70 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#71 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#72 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#73 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_m_m_1_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#74 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_Concat_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#75 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv3_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#76 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv3_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#77 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_m_0_cv3_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#78 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_Concat_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 37632, AI_STATIC)

/* Array#79 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#80 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#81 */
AI_ARRAY_OBJ_DECLARE(
  _model_6_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 25088, AI_STATIC)

/* Array#82 */
AI_ARRAY_OBJ_DECLARE(
  _model_7_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#83 */
AI_ARRAY_OBJ_DECLARE(
  _model_7_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#84 */
AI_ARRAY_OBJ_DECLARE(
  _model_7_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#85 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#86 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#87 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#88 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_Split_output_0_output0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#89 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_Split_output_0_output1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#90 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_Split_output_0_num_or_size_splits_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 1, AI_STATIC)

/* Array#91 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#92 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#93 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#94 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#95 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#96 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#97 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#98 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#99 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#100 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#101 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#102 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#103 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_0_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#104 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#105 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#106 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#107 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#108 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#109 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#110 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_m_m_1_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#111 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_Concat_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#112 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv3_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#113 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv3_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#114 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_m_0_cv3_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#115 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_Concat_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 18816, AI_STATIC)

/* Array#116 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#117 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#118 */
AI_ARRAY_OBJ_DECLARE(
  _model_8_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#119 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_cv1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#120 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_cv1_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#121 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_cv1_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#122 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_Split_output_0_output0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#123 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_Split_output_0_output1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#124 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_Split_output_0_num_or_size_splits_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 1, AI_STATIC)

/* Array#125 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_qkv_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#126 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#127 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#128 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_output0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#129 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_output1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#130 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_output2_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#131 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_num_or_size_splits_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 3, AI_STATIC)

/* Array#132 */
AI_ARRAY_OBJ_DECLARE(
  transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#133 */
AI_ARRAY_OBJ_DECLARE(
  transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#134 */
AI_ARRAY_OBJ_DECLARE(
  transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 3136, AI_STATIC)

/* Array#135 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4802, AI_STATIC)

/* Array#136 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#137 */
AI_ARRAY_OBJ_DECLARE(
  transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4802, AI_STATIC)

/* Array#138 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Softmax_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4802, AI_STATIC)

/* Array#139 */
AI_ARRAY_OBJ_DECLARE(
  transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4802, AI_STATIC)

/* Array#140 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_1_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#141 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_1_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#142 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#143 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#144 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#145 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_pe_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#146 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#147 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_attn_proj_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#148 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#149 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#150 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#151 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#152 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#153 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_m_m_0_Add_1_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6272, AI_STATIC)

/* Array#154 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_Concat_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#155 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_cv2_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#156 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_cv2_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#157 */
AI_ARRAY_OBJ_DECLARE(
  _model_9_cv2_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 12544, AI_STATIC)

/* Array#158 */
AI_ARRAY_OBJ_DECLARE(
  _model_10_conv_conv_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 62720, AI_STATIC)

/* Array#159 */
AI_ARRAY_OBJ_DECLARE(
  _model_10_conv_act_Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 62720, AI_STATIC)

/* Array#160 */
AI_ARRAY_OBJ_DECLARE(
  _model_10_conv_act_Mul_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 62720, AI_STATIC)

/* Array#161 */
AI_ARRAY_OBJ_DECLARE(
  _model_10_pool_GlobalAveragePool_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1280, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  images_Transpose_output, AI_STATIC,
  280, 0x0,
  AI_SHAPE_INIT(4, 1, 3, 224, 224), AI_STRIDE_INIT(4, 4, 4, 12, 2688),
  1, &images_Transpose_output_array, NULL)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  images_output, AI_STATIC,
  281, 0x0,
  AI_SHAPE_INIT(4, 1, 224, 224, 3), AI_STRIDE_INIT(4, 4, 4, 896, 200704),
  1, &images_output_array, NULL)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  _model_0_act_Mul_output_0_output, AI_STATIC,
  0, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 112, 112), AI_STRIDE_INIT(4, 4, 4, 64, 7168),
  1, &_model_0_act_Mul_output_0_output_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  _model_0_act_Sigmoid_output_0_output, AI_STATIC,
  1, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 112, 112), AI_STRIDE_INIT(4, 4, 4, 64, 7168),
  1, &_model_0_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  _model_0_conv_Conv_output_0_output, AI_STATIC,
  3, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 112, 112), AI_STRIDE_INIT(4, 4, 4, 64, 7168),
  1, &_model_0_conv_Conv_output_0_output_array, NULL)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  _model_1_act_Mul_output_0_output, AI_STATIC,
  16, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 56, 56), AI_STRIDE_INIT(4, 4, 4, 128, 7168),
  1, &_model_1_act_Mul_output_0_output_array, NULL)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  _model_1_act_Sigmoid_output_0_output, AI_STATIC,
  17, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 56, 56), AI_STRIDE_INIT(4, 4, 4, 128, 7168),
  1, &_model_1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  _model_1_conv_Conv_output_0_output, AI_STATIC,
  19, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 56, 56), AI_STRIDE_INIT(4, 4, 4, 128, 7168),
  1, &_model_1_conv_Conv_output_0_output_array, NULL)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_cv1_act_Mul_output_0_output, AI_STATIC,
  26, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 56, 56), AI_STRIDE_INIT(4, 4, 4, 128, 7168),
  1, &_model_2_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  27, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 56, 56), AI_STRIDE_INIT(4, 4, 4, 128, 7168),
  1, &_model_2_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_cv1_conv_Conv_output_0_output, AI_STATIC,
  29, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 56, 56), AI_STRIDE_INIT(4, 4, 4, 128, 7168),
  1, &_model_2_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_Split_output_0_num_or_size_splits, AI_STATIC,
  23, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_2_Split_output_0_num_or_size_splits_array, NULL)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_Split_output_0_output0, AI_STATIC,
  24, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 56, 56), AI_STRIDE_INIT(4, 4, 4, 64, 3584),
  1, &_model_2_Split_output_0_output0_array, NULL)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_Split_output_0_output1, AI_STATIC,
  25, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 56, 56), AI_STRIDE_INIT(4, 4, 4, 64, 3584),
  1, &_model_2_Split_output_0_output1_array, NULL)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_m_0_cv1_act_Mul_output_0_output, AI_STATIC,
  39, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 56, 56), AI_STRIDE_INIT(4, 4, 4, 32, 1792),
  1, &_model_2_m_0_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_m_0_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  40, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 56, 56), AI_STRIDE_INIT(4, 4, 4, 32, 1792),
  1, &_model_2_m_0_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_m_0_cv1_conv_Conv_output_0_output, AI_STATIC,
  42, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 56, 56), AI_STRIDE_INIT(4, 4, 4, 32, 1792),
  1, &_model_2_m_0_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_m_0_cv2_act_Mul_output_0_output, AI_STATIC,
  45, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 56, 56), AI_STRIDE_INIT(4, 4, 4, 64, 3584),
  1, &_model_2_m_0_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_m_0_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  46, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 56, 56), AI_STRIDE_INIT(4, 4, 4, 64, 3584),
  1, &_model_2_m_0_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_m_0_cv2_conv_Conv_output_0_output, AI_STATIC,
  48, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 56, 56), AI_STRIDE_INIT(4, 4, 4, 64, 3584),
  1, &_model_2_m_0_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #20 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_m_0_Add_output_0_output, AI_STATIC,
  38, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 56, 56), AI_STRIDE_INIT(4, 4, 4, 64, 3584),
  1, &_model_2_m_0_Add_output_0_output_array, NULL)

/* Tensor #21 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_Concat_output_0_output, AI_STATIC,
  22, 0x0,
  AI_SHAPE_INIT(4, 1, 48, 56, 56), AI_STRIDE_INIT(4, 4, 4, 192, 10752),
  1, &_model_2_Concat_output_0_output_array, NULL)

/* Tensor #22 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_cv2_act_Mul_output_0_output, AI_STATIC,
  32, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 56, 56), AI_STRIDE_INIT(4, 4, 4, 256, 14336),
  1, &_model_2_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #23 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  33, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 56, 56), AI_STRIDE_INIT(4, 4, 4, 256, 14336),
  1, &_model_2_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #24 */
AI_TENSOR_OBJ_DECLARE(
  _model_2_cv2_conv_Conv_output_0_output, AI_STATIC,
  35, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 56, 56), AI_STRIDE_INIT(4, 4, 4, 256, 14336),
  1, &_model_2_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #25 */
AI_TENSOR_OBJ_DECLARE(
  _model_3_act_Mul_output_0_output, AI_STATIC,
  51, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 28, 28), AI_STRIDE_INIT(4, 4, 4, 256, 7168),
  1, &_model_3_act_Mul_output_0_output_array, NULL)

/* Tensor #26 */
AI_TENSOR_OBJ_DECLARE(
  _model_3_act_Sigmoid_output_0_output, AI_STATIC,
  52, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 28, 28), AI_STRIDE_INIT(4, 4, 4, 256, 7168),
  1, &_model_3_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #27 */
AI_TENSOR_OBJ_DECLARE(
  _model_3_conv_Conv_output_0_output, AI_STATIC,
  54, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 28, 28), AI_STRIDE_INIT(4, 4, 4, 256, 7168),
  1, &_model_3_conv_Conv_output_0_output_array, NULL)

/* Tensor #28 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_cv1_act_Mul_output_0_output, AI_STATIC,
  61, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 28, 28), AI_STRIDE_INIT(4, 4, 4, 256, 7168),
  1, &_model_4_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #29 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  62, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 28, 28), AI_STRIDE_INIT(4, 4, 4, 256, 7168),
  1, &_model_4_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #30 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_cv1_conv_Conv_output_0_output, AI_STATIC,
  64, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 28, 28), AI_STRIDE_INIT(4, 4, 4, 256, 7168),
  1, &_model_4_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #31 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_Split_output_0_num_or_size_splits, AI_STATIC,
  58, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_4_Split_output_0_num_or_size_splits_array, NULL)

/* Tensor #32 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_Split_output_0_output0, AI_STATIC,
  59, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 28, 28), AI_STRIDE_INIT(4, 4, 4, 128, 3584),
  1, &_model_4_Split_output_0_output0_array, NULL)

/* Tensor #33 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_Split_output_0_output1, AI_STATIC,
  60, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 28, 28), AI_STRIDE_INIT(4, 4, 4, 128, 3584),
  1, &_model_4_Split_output_0_output1_array, NULL)

/* Tensor #34 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_m_0_cv1_act_Mul_output_0_output, AI_STATIC,
  74, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 4, 4, 64, 1792),
  1, &_model_4_m_0_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #35 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_m_0_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  75, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 4, 4, 64, 1792),
  1, &_model_4_m_0_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #36 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_m_0_cv1_conv_Conv_output_0_output, AI_STATIC,
  77, 0x0,
  AI_SHAPE_INIT(4, 1, 16, 28, 28), AI_STRIDE_INIT(4, 4, 4, 64, 1792),
  1, &_model_4_m_0_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #37 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_m_0_cv2_act_Mul_output_0_output, AI_STATIC,
  80, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 28, 28), AI_STRIDE_INIT(4, 4, 4, 128, 3584),
  1, &_model_4_m_0_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #38 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_m_0_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  81, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 28, 28), AI_STRIDE_INIT(4, 4, 4, 128, 3584),
  1, &_model_4_m_0_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #39 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_m_0_cv2_conv_Conv_output_0_output, AI_STATIC,
  83, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 28, 28), AI_STRIDE_INIT(4, 4, 4, 128, 3584),
  1, &_model_4_m_0_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #40 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_m_0_Add_output_0_output, AI_STATIC,
  73, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 28, 28), AI_STRIDE_INIT(4, 4, 4, 128, 3584),
  1, &_model_4_m_0_Add_output_0_output_array, NULL)

/* Tensor #41 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_Concat_output_0_output, AI_STATIC,
  57, 0x0,
  AI_SHAPE_INIT(4, 1, 96, 28, 28), AI_STRIDE_INIT(4, 4, 4, 384, 10752),
  1, &_model_4_Concat_output_0_output_array, NULL)

/* Tensor #42 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_cv2_act_Mul_output_0_output, AI_STATIC,
  67, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 28, 28), AI_STRIDE_INIT(4, 4, 4, 512, 14336),
  1, &_model_4_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #43 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  68, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 28, 28), AI_STRIDE_INIT(4, 4, 4, 512, 14336),
  1, &_model_4_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #44 */
AI_TENSOR_OBJ_DECLARE(
  _model_4_cv2_conv_Conv_output_0_output, AI_STATIC,
  70, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 28, 28), AI_STRIDE_INIT(4, 4, 4, 512, 14336),
  1, &_model_4_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #45 */
AI_TENSOR_OBJ_DECLARE(
  _model_5_act_Mul_output_0_output, AI_STATIC,
  86, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_5_act_Mul_output_0_output_array, NULL)

/* Tensor #46 */
AI_TENSOR_OBJ_DECLARE(
  _model_5_act_Sigmoid_output_0_output, AI_STATIC,
  87, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_5_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #47 */
AI_TENSOR_OBJ_DECLARE(
  _model_5_conv_Conv_output_0_output, AI_STATIC,
  89, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_5_conv_Conv_output_0_output_array, NULL)

/* Tensor #48 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_cv1_act_Mul_output_0_output, AI_STATIC,
  96, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_6_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #49 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  97, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_6_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #50 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_cv1_conv_Conv_output_0_output, AI_STATIC,
  99, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_6_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #51 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_Split_output_0_num_or_size_splits, AI_STATIC,
  93, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_6_Split_output_0_num_or_size_splits_array, NULL)

/* Tensor #52 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_Split_output_0_output0, AI_STATIC,
  94, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 14, 14), AI_STRIDE_INIT(4, 4, 4, 256, 3584),
  1, &_model_6_Split_output_0_output0_array, NULL)

/* Tensor #53 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_Split_output_0_output1, AI_STATIC,
  95, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 14, 14), AI_STRIDE_INIT(4, 4, 4, 256, 3584),
  1, &_model_6_Split_output_0_output1_array, NULL)

/* Tensor #54 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv2_act_Mul_output_0_output, AI_STATIC,
  115, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #55 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  116, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #56 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv2_conv_Conv_output_0_output, AI_STATIC,
  118, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #57 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv1_act_Mul_output_0_output, AI_STATIC,
  109, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #58 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  110, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #59 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv1_conv_Conv_output_0_output, AI_STATIC,
  112, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #60 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv1_act_Mul_output_0_output, AI_STATIC,
  128, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_0_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #61 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  129, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #62 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_output, AI_STATIC,
  131, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_0_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #63 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv2_act_Mul_output_0_output, AI_STATIC,
  134, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_0_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #64 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  135, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #65 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_output, AI_STATIC,
  137, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_0_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #66 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_0_Add_output_0_output, AI_STATIC,
  127, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_0_Add_output_0_output_array, NULL)

/* Tensor #67 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv1_act_Mul_output_0_output, AI_STATIC,
  141, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_1_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #68 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  142, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #69 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_output, AI_STATIC,
  144, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_1_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #70 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv2_act_Mul_output_0_output, AI_STATIC,
  147, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_1_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #71 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  148, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #72 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_output, AI_STATIC,
  150, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_1_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #73 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_m_m_1_Add_output_0_output, AI_STATIC,
  140, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 14, 14), AI_STRIDE_INIT(4, 4, 4, 128, 1792),
  1, &_model_6_m_0_m_m_1_Add_output_0_output_array, NULL)

/* Tensor #74 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_Concat_output_0_output, AI_STATIC,
  108, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 14, 14), AI_STRIDE_INIT(4, 4, 4, 256, 3584),
  1, &_model_6_m_0_Concat_output_0_output_array, NULL)

/* Tensor #75 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv3_act_Mul_output_0_output, AI_STATIC,
  121, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 14, 14), AI_STRIDE_INIT(4, 4, 4, 256, 3584),
  1, &_model_6_m_0_cv3_act_Mul_output_0_output_array, NULL)

/* Tensor #76 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv3_act_Sigmoid_output_0_output, AI_STATIC,
  122, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 14, 14), AI_STRIDE_INIT(4, 4, 4, 256, 3584),
  1, &_model_6_m_0_cv3_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #77 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_m_0_cv3_conv_Conv_output_0_output, AI_STATIC,
  124, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 14, 14), AI_STRIDE_INIT(4, 4, 4, 256, 3584),
  1, &_model_6_m_0_cv3_conv_Conv_output_0_output_array, NULL)

/* Tensor #78 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_Concat_output_0_output, AI_STATIC,
  92, 0x0,
  AI_SHAPE_INIT(4, 1, 192, 14, 14), AI_STRIDE_INIT(4, 4, 4, 768, 10752),
  1, &_model_6_Concat_output_0_output_array, NULL)

/* Tensor #79 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_cv2_act_Mul_output_0_output, AI_STATIC,
  102, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_6_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #80 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  103, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_6_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #81 */
AI_TENSOR_OBJ_DECLARE(
  _model_6_cv2_conv_Conv_output_0_output, AI_STATIC,
  105, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 14, 14), AI_STRIDE_INIT(4, 4, 4, 512, 7168),
  1, &_model_6_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #82 */
AI_TENSOR_OBJ_DECLARE(
  _model_7_act_Mul_output_0_output, AI_STATIC,
  153, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_7_act_Mul_output_0_output_array, NULL)

/* Tensor #83 */
AI_TENSOR_OBJ_DECLARE(
  _model_7_act_Sigmoid_output_0_output, AI_STATIC,
  154, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_7_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #84 */
AI_TENSOR_OBJ_DECLARE(
  _model_7_conv_Conv_output_0_output, AI_STATIC,
  156, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_7_conv_Conv_output_0_output_array, NULL)

/* Tensor #85 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_cv1_act_Mul_output_0_output, AI_STATIC,
  163, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_8_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #86 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  164, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_8_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #87 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_cv1_conv_Conv_output_0_output, AI_STATIC,
  166, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_8_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #88 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_Split_output_0_num_or_size_splits, AI_STATIC,
  160, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_8_Split_output_0_num_or_size_splits_array, NULL)

/* Tensor #89 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_Split_output_0_output0, AI_STATIC,
  161, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_8_Split_output_0_output0_array, NULL)

/* Tensor #90 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_Split_output_0_output1, AI_STATIC,
  162, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_8_Split_output_0_output1_array, NULL)

/* Tensor #91 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv2_act_Mul_output_0_output, AI_STATIC,
  182, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #92 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  183, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #93 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv2_conv_Conv_output_0_output, AI_STATIC,
  185, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #94 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv1_act_Mul_output_0_output, AI_STATIC,
  176, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #95 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  177, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #96 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv1_conv_Conv_output_0_output, AI_STATIC,
  179, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #97 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv1_act_Mul_output_0_output, AI_STATIC,
  195, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_0_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #98 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  196, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #99 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_output, AI_STATIC,
  198, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_0_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #100 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv2_act_Mul_output_0_output, AI_STATIC,
  201, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_0_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #101 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  202, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #102 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_output, AI_STATIC,
  204, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_0_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #103 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_0_Add_output_0_output, AI_STATIC,
  194, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_0_Add_output_0_output_array, NULL)

/* Tensor #104 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv1_act_Mul_output_0_output, AI_STATIC,
  208, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_1_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #105 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  209, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #106 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_output, AI_STATIC,
  211, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_1_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #107 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv2_act_Mul_output_0_output, AI_STATIC,
  214, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_1_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #108 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  215, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #109 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_output, AI_STATIC,
  217, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_1_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #110 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_m_m_1_Add_output_0_output, AI_STATIC,
  207, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 7, 7), AI_STRIDE_INIT(4, 4, 4, 256, 1792),
  1, &_model_8_m_0_m_m_1_Add_output_0_output_array, NULL)

/* Tensor #111 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_Concat_output_0_output, AI_STATIC,
  175, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_8_m_0_Concat_output_0_output_array, NULL)

/* Tensor #112 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv3_act_Mul_output_0_output, AI_STATIC,
  188, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_8_m_0_cv3_act_Mul_output_0_output_array, NULL)

/* Tensor #113 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv3_act_Sigmoid_output_0_output, AI_STATIC,
  189, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_8_m_0_cv3_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #114 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_m_0_cv3_conv_Conv_output_0_output, AI_STATIC,
  191, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_8_m_0_cv3_conv_Conv_output_0_output_array, NULL)

/* Tensor #115 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_Concat_output_0_output, AI_STATIC,
  159, 0x0,
  AI_SHAPE_INIT(4, 1, 384, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1536, 10752),
  1, &_model_8_Concat_output_0_output_array, NULL)

/* Tensor #116 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_cv2_act_Mul_output_0_output, AI_STATIC,
  169, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_8_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #117 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  170, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_8_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #118 */
AI_TENSOR_OBJ_DECLARE(
  _model_8_cv2_conv_Conv_output_0_output, AI_STATIC,
  172, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_8_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #119 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_cv1_act_Mul_output_0_output, AI_STATIC,
  224, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_cv1_act_Mul_output_0_output_array, NULL)

/* Tensor #120 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_cv1_act_Sigmoid_output_0_output, AI_STATIC,
  225, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_cv1_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #121 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_cv1_conv_Conv_output_0_output, AI_STATIC,
  227, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_cv1_conv_Conv_output_0_output_array, NULL)

/* Tensor #122 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_Split_output_0_num_or_size_splits, AI_STATIC,
  221, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_9_Split_output_0_num_or_size_splits_array, NULL)

/* Tensor #123 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_Split_output_0_output0, AI_STATIC,
  222, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_Split_output_0_output0_array, NULL)

/* Tensor #124 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_Split_output_0_output1, AI_STATIC,
  223, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_Split_output_0_output1_array, NULL)

/* Tensor #125 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output, AI_STATIC,
  252, 0x0,
  AI_SHAPE_INIT(4, 1, 7, 7, 256), AI_STRIDE_INIT(4, 4, 4, 28, 196),
  1, &_model_9_m_m_0_attn_Reshape_output_0_to_chlast_output_array, NULL)

/* Tensor #126 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_qkv_conv_Conv_output_0_output, AI_STATIC,
  267, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_m_m_0_attn_qkv_conv_Conv_output_0_output_array, NULL)

/* Tensor #127 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output, AI_STATIC,
  251, 0x0,
  AI_SHAPE_INIT(4, 1, 2, 49, 128), AI_STRIDE_INIT(4, 4, 4, 8, 392),
  1, &_model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output_array, NULL)

/* Tensor #128 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output0, AI_STATIC,
  253, 0x0,
  AI_SHAPE_INIT(4, 1, 49, 128, 2), AI_STRIDE_INIT(4, 4, 4, 196, 25088),
  1, &_model_9_m_m_0_attn_Reshape_output_0_to_chlast_output_array, NULL)

/* Tensor #129 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_num_or_size_splits, AI_STATIC,
  255, 0x0,
  AI_SHAPE_INIT(4, 1, 3, 1, 1), AI_STRIDE_INIT(4, 4, 4, 12, 12),
  1, &_model_9_m_m_0_attn_Split_output_0_num_or_size_splits_array, NULL)

/* Tensor #130 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_output0, AI_STATIC,
  256, 0x0,
  AI_SHAPE_INIT(4, 1, 2, 49, 32), AI_STRIDE_INIT(4, 4, 4, 8, 392),
  1, &_model_9_m_m_0_attn_Split_output_0_output0_array, NULL)

/* Tensor #131 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_output1, AI_STATIC,
  257, 0x0,
  AI_SHAPE_INIT(4, 1, 2, 49, 32), AI_STRIDE_INIT(4, 4, 4, 8, 392),
  1, &_model_9_m_m_0_attn_Split_output_0_output1_array, NULL)

/* Tensor #132 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_output2, AI_STATIC,
  258, 0x0,
  AI_SHAPE_INIT(4, 1, 2, 49, 64), AI_STRIDE_INIT(4, 4, 4, 8, 392),
  1, &_model_9_m_m_0_attn_Split_output_0_output2_array, NULL)

/* Tensor #133 */
AI_TENSOR_OBJ_DECLARE(
  transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output, AI_STATIC,
  284, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 49, 2), AI_STRIDE_INIT(4, 4, 4, 128, 6272),
  1, &transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output_array, NULL)

/* Tensor #134 */
AI_TENSOR_OBJ_DECLARE(
  transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output, AI_STATIC,
  283, 0x0,
  AI_SHAPE_INIT(4, 1, 49, 64, 2), AI_STRIDE_INIT(4, 4, 4, 196, 12544),
  1, &transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array, NULL)

/* Tensor #135 */
AI_TENSOR_OBJ_DECLARE(
  transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output, AI_STATIC,
  286, 0x0,
  AI_SHAPE_INIT(4, 1, 49, 32, 2), AI_STRIDE_INIT(4, 4, 4, 196, 6272),
  1, &transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output_array, NULL)

/* Tensor #136 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_output_0_bias, AI_STATIC,
  242, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_9_m_m_0_attn_MatMul_output_0_bias_array, NULL)

/* Tensor #137 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_output_0_output, AI_STATIC,
  243, 0x0,
  AI_SHAPE_INIT(4, 1, 49, 49, 2), AI_STRIDE_INIT(4, 4, 4, 196, 9604),
  1, &_model_9_m_m_0_attn_MatMul_output_0_output_array, NULL)

/* Tensor #138 */
AI_TENSOR_OBJ_DECLARE(
  transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_output, AI_STATIC,
  287, 0x0,
  AI_SHAPE_INIT(4, 1, 2, 49, 49), AI_STRIDE_INIT(4, 4, 4, 8, 392),
  1, &transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_output_array, NULL)

/* Tensor #139 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Softmax_output_0_output, AI_STATIC,
  254, 0x0,
  AI_SHAPE_INIT(4, 1, 2, 49, 49), AI_STRIDE_INIT(4, 4, 4, 8, 392),
  1, &_model_9_m_m_0_attn_Softmax_output_0_output_array, NULL)

/* Tensor #140 */
AI_TENSOR_OBJ_DECLARE(
  transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output, AI_STATIC,
  285, 0x0,
  AI_SHAPE_INIT(4, 1, 49, 49, 2), AI_STRIDE_INIT(4, 4, 4, 196, 9604),
  1, &transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array, NULL)

/* Tensor #141 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_1_output_0_bias, AI_STATIC,
  239, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_9_m_m_0_attn_MatMul_1_output_0_bias_array, NULL)

/* Tensor #142 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_1_output_0_output, AI_STATIC,
  240, 0x0,
  AI_SHAPE_INIT(4, 1, 49, 64, 2), AI_STRIDE_INIT(4, 4, 4, 196, 12544),
  1, &_model_9_m_m_0_attn_MatMul_1_output_0_output_array, NULL)

/* Tensor #143 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_1_output_0_output0, AI_STATIC,
  241, 0x0,
  AI_SHAPE_INIT(4, 1, 7, 7, 128), AI_STRIDE_INIT(4, 4, 4, 28, 196),
  1, &_model_9_m_m_0_attn_MatMul_1_output_0_output_array, NULL)

/* Tensor #144 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output, AI_STATIC,
  247, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output_array, NULL)

/* Tensor #145 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output, AI_STATIC,
  249, 0x0,
  AI_SHAPE_INIT(4, 1, 49, 64, 2), AI_STRIDE_INIT(4, 4, 4, 196, 12544),
  1, &_model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output_array, NULL)

/* Tensor #146 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_output, AI_STATIC,
  248, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_output_array, NULL)

/* Tensor #147 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output0, AI_STATIC,
  250, 0x0,
  AI_SHAPE_INIT(4, 1, 7, 7, 128), AI_STRIDE_INIT(4, 4, 4, 28, 196),
  1, &_model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output_array, NULL)

/* Tensor #148 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_Add_output_0_output, AI_STATIC,
  238, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_m_m_0_attn_Add_output_0_output_array, NULL)

/* Tensor #149 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_pe_conv_Conv_output_0_output, AI_STATIC,
  260, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_m_m_0_attn_pe_conv_Conv_output_0_output_array, NULL)

/* Tensor #150 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_Add_output_0_output, AI_STATIC,
  237, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_m_m_0_Add_output_0_output_array, NULL)

/* Tensor #151 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_attn_proj_conv_Conv_output_0_output, AI_STATIC,
  263, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_m_m_0_attn_proj_conv_Conv_output_0_output_array, NULL)

/* Tensor #152 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_output, AI_STATIC,
  270, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_output_array, NULL)

/* Tensor #153 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_output, AI_STATIC,
  271, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #154 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_output, AI_STATIC,
  273, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_output_array, NULL)

/* Tensor #155 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_Add_1_output_0_output, AI_STATIC,
  236, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_m_m_0_Add_1_output_0_output_array, NULL)

/* Tensor #156 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_output, AI_STATIC,
  277, 0x0,
  AI_SHAPE_INIT(4, 1, 128, 7, 7), AI_STRIDE_INIT(4, 4, 4, 512, 3584),
  1, &_model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_output_array, NULL)

/* Tensor #157 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_Concat_output_0_output, AI_STATIC,
  220, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_Concat_output_0_output_array, NULL)

/* Tensor #158 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_cv2_act_Mul_output_0_output, AI_STATIC,
  230, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_cv2_act_Mul_output_0_output_array, NULL)

/* Tensor #159 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_cv2_act_Sigmoid_output_0_output, AI_STATIC,
  231, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_cv2_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #160 */
AI_TENSOR_OBJ_DECLARE(
  _model_9_cv2_conv_Conv_output_0_output, AI_STATIC,
  233, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 7, 7), AI_STRIDE_INIT(4, 4, 4, 1024, 7168),
  1, &_model_9_cv2_conv_Conv_output_0_output_array, NULL)

/* Tensor #161 */
AI_TENSOR_OBJ_DECLARE(
  _model_10_conv_act_Mul_output_0_output, AI_STATIC,
  6, 0x0,
  AI_SHAPE_INIT(4, 1, 1280, 7, 7), AI_STRIDE_INIT(4, 4, 4, 5120, 35840),
  1, &_model_10_conv_act_Mul_output_0_output_array, NULL)

/* Tensor #162 */
AI_TENSOR_OBJ_DECLARE(
  _model_10_conv_act_Sigmoid_output_0_output, AI_STATIC,
  7, 0x0,
  AI_SHAPE_INIT(4, 1, 1280, 7, 7), AI_STRIDE_INIT(4, 4, 4, 5120, 35840),
  1, &_model_10_conv_act_Sigmoid_output_0_output_array, NULL)

/* Tensor #163 */
AI_TENSOR_OBJ_DECLARE(
  _model_10_conv_conv_Conv_output_0_output, AI_STATIC,
  9, 0x0,
  AI_SHAPE_INIT(4, 1, 1280, 7, 7), AI_STRIDE_INIT(4, 4, 4, 5120, 35840),
  1, &_model_10_conv_conv_Conv_output_0_output_array, NULL)

/* Tensor #164 */
AI_TENSOR_OBJ_DECLARE(
  _model_10_pool_GlobalAveragePool_output_0_output, AI_STATIC,
  15, 0x0,
  AI_SHAPE_INIT(4, 1, 1280, 1, 1), AI_STRIDE_INIT(4, 4, 4, 5120, 5120),
  1, &_model_10_pool_GlobalAveragePool_output_0_output_array, NULL)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  images_Transpose_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &images_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &images_Transpose_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  images_Transpose_layer, 2,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &images_Transpose_chain,
  NULL, &images_Transpose_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_0_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_0_conv_Conv_output_0_output, &_model_0_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_0_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_0_act_Mul_output_0_layer, 3,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_0_act_Mul_output_0_chain,
  NULL, &_model_0_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_1_conv_Conv_output_0_output, &_model_1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_1_act_Mul_output_0_layer, 6,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_1_act_Mul_output_0_chain,
  NULL, &_model_1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_2_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_2_cv1_conv_Conv_output_0_output, &_model_2_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_2_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_2_cv1_act_Mul_output_0_layer, 9,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_2_cv1_act_Mul_output_0_chain,
  NULL, &_model_2_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_2_Split_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_2_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_2_Split_output_0_output0, &_model_2_Split_output_0_output1),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_2_Split_output_0_num_or_size_splits),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_2_Split_output_0_layer, 11,
  SPLIT_TYPE, 0x0, NULL,
  split, forward_split,
  &_model_2_Split_output_0_chain,
  NULL, &_model_2_Split_output_0_layer, AI_STATIC, 
  .outer_elems = 3136, 
  .outer_elems_stride = 128, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_2_m_0_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_2_m_0_cv1_conv_Conv_output_0_output, &_model_2_m_0_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_2_m_0_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_2_m_0_cv1_act_Mul_output_0_layer, 14,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_2_m_0_cv1_act_Mul_output_0_chain,
  NULL, &_model_2_m_0_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_2_m_0_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_2_m_0_cv2_conv_Conv_output_0_output, &_model_2_m_0_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_2_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_2_m_0_cv2_act_Mul_output_0_layer, 17,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_2_m_0_cv2_act_Mul_output_0_chain,
  NULL, &_model_2_m_0_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_2_m_0_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_2_Split_output_0_output1, &_model_2_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_2_m_0_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_2_m_0_Add_output_0_layer, 18,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_2_m_0_Add_output_0_chain,
  NULL, &_model_2_m_0_Add_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_2_Concat_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_2_Split_output_0_output0, &_model_2_Split_output_0_output1, &_model_2_m_0_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_2_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_2_Concat_output_0_layer, 19,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &_model_2_Concat_output_0_chain,
  NULL, &_model_2_Concat_output_0_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_2_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_2_cv2_conv_Conv_output_0_output, &_model_2_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_2_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_2_cv2_act_Mul_output_0_layer, 22,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_2_cv2_act_Mul_output_0_chain,
  NULL, &_model_2_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_3_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_3_conv_Conv_output_0_output, &_model_3_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_3_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_3_act_Mul_output_0_layer, 25,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_3_act_Mul_output_0_chain,
  NULL, &_model_3_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_4_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_4_cv1_conv_Conv_output_0_output, &_model_4_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_4_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_4_cv1_act_Mul_output_0_layer, 28,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_4_cv1_act_Mul_output_0_chain,
  NULL, &_model_4_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_4_Split_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_4_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_4_Split_output_0_output0, &_model_4_Split_output_0_output1),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_4_Split_output_0_num_or_size_splits),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_4_Split_output_0_layer, 30,
  SPLIT_TYPE, 0x0, NULL,
  split, forward_split,
  &_model_4_Split_output_0_chain,
  NULL, &_model_4_Split_output_0_layer, AI_STATIC, 
  .outer_elems = 784, 
  .outer_elems_stride = 256, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_4_m_0_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_4_m_0_cv1_conv_Conv_output_0_output, &_model_4_m_0_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_4_m_0_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_4_m_0_cv1_act_Mul_output_0_layer, 33,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_4_m_0_cv1_act_Mul_output_0_chain,
  NULL, &_model_4_m_0_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_4_m_0_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_4_m_0_cv2_conv_Conv_output_0_output, &_model_4_m_0_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_4_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_4_m_0_cv2_act_Mul_output_0_layer, 36,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_4_m_0_cv2_act_Mul_output_0_chain,
  NULL, &_model_4_m_0_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_4_m_0_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_4_Split_output_0_output1, &_model_4_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_4_m_0_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_4_m_0_Add_output_0_layer, 37,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_4_m_0_Add_output_0_chain,
  NULL, &_model_4_m_0_Add_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_4_Concat_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_4_Split_output_0_output0, &_model_4_Split_output_0_output1, &_model_4_m_0_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_4_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_4_Concat_output_0_layer, 38,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &_model_4_Concat_output_0_chain,
  NULL, &_model_4_Concat_output_0_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_4_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_4_cv2_conv_Conv_output_0_output, &_model_4_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_4_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_4_cv2_act_Mul_output_0_layer, 41,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_4_cv2_act_Mul_output_0_chain,
  NULL, &_model_4_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_5_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_5_conv_Conv_output_0_output, &_model_5_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_5_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_5_act_Mul_output_0_layer, 44,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_5_act_Mul_output_0_chain,
  NULL, &_model_5_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_cv1_conv_Conv_output_0_output, &_model_6_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_cv1_act_Mul_output_0_layer, 47,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_cv1_act_Mul_output_0_chain,
  NULL, &_model_6_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_Split_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_Split_output_0_output0, &_model_6_Split_output_0_output1),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_Split_output_0_num_or_size_splits),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_Split_output_0_layer, 49,
  SPLIT_TYPE, 0x0, NULL,
  split, forward_split,
  &_model_6_Split_output_0_chain,
  NULL, &_model_6_Split_output_0_layer, AI_STATIC, 
  .outer_elems = 196, 
  .outer_elems_stride = 512, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_cv2_conv_Conv_output_0_output, &_model_6_m_0_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_cv2_act_Mul_output_0_layer, 69,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_cv2_act_Mul_output_0_chain,
  NULL, &_model_6_m_0_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_cv1_conv_Conv_output_0_output, &_model_6_m_0_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_cv1_act_Mul_output_0_layer, 52,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_cv1_act_Mul_output_0_chain,
  NULL, &_model_6_m_0_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_m_m_0_cv1_conv_Conv_output_0_output, &_model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_m_m_0_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv1_act_Mul_output_0_layer, 55,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_m_m_0_cv1_act_Mul_output_0_chain,
  NULL, &_model_6_m_0_m_m_0_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_m_m_0_cv2_conv_Conv_output_0_output, &_model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_m_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_m_m_0_cv2_act_Mul_output_0_layer, 58,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_m_m_0_cv2_act_Mul_output_0_chain,
  NULL, &_model_6_m_0_m_m_0_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_m_m_0_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_cv1_act_Mul_output_0_output, &_model_6_m_0_m_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_m_m_0_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_m_m_0_Add_output_0_layer, 59,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_m_m_0_Add_output_0_chain,
  NULL, &_model_6_m_0_m_m_0_Add_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_m_m_1_cv1_conv_Conv_output_0_output, &_model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_m_m_1_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv1_act_Mul_output_0_layer, 62,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_m_m_1_cv1_act_Mul_output_0_chain,
  NULL, &_model_6_m_0_m_m_1_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_m_m_1_cv2_conv_Conv_output_0_output, &_model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_m_m_1_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_m_m_1_cv2_act_Mul_output_0_layer, 65,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_m_m_1_cv2_act_Mul_output_0_chain,
  NULL, &_model_6_m_0_m_m_1_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_m_m_1_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_m_m_0_Add_output_0_output, &_model_6_m_0_m_m_1_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_m_m_1_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_m_m_1_Add_output_0_layer, 66,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_m_m_1_Add_output_0_chain,
  NULL, &_model_6_m_0_m_m_1_Add_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_Concat_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_m_m_1_Add_output_0_output, &_model_6_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_Concat_output_0_layer, 70,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &_model_6_m_0_Concat_output_0_chain,
  NULL, &_model_6_m_0_Concat_output_0_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_m_0_cv3_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_m_0_cv3_conv_Conv_output_0_output, &_model_6_m_0_cv3_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_m_0_cv3_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_m_0_cv3_act_Mul_output_0_layer, 73,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_m_0_cv3_act_Mul_output_0_chain,
  NULL, &_model_6_m_0_cv3_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_Concat_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_6_Split_output_0_output0, &_model_6_Split_output_0_output1, &_model_6_m_0_cv3_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_Concat_output_0_layer, 74,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &_model_6_Concat_output_0_chain,
  NULL, &_model_6_Concat_output_0_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_6_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_6_cv2_conv_Conv_output_0_output, &_model_6_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_6_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_6_cv2_act_Mul_output_0_layer, 77,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_6_cv2_act_Mul_output_0_chain,
  NULL, &_model_6_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_7_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_7_conv_Conv_output_0_output, &_model_7_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_7_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_7_act_Mul_output_0_layer, 80,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_7_act_Mul_output_0_chain,
  NULL, &_model_7_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_cv1_conv_Conv_output_0_output, &_model_8_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_cv1_act_Mul_output_0_layer, 83,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_cv1_act_Mul_output_0_chain,
  NULL, &_model_8_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_Split_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_Split_output_0_output0, &_model_8_Split_output_0_output1),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_Split_output_0_num_or_size_splits),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_Split_output_0_layer, 85,
  SPLIT_TYPE, 0x0, NULL,
  split, forward_split,
  &_model_8_Split_output_0_chain,
  NULL, &_model_8_Split_output_0_layer, AI_STATIC, 
  .outer_elems = 49, 
  .outer_elems_stride = 1024, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_cv2_conv_Conv_output_0_output, &_model_8_m_0_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_cv2_act_Mul_output_0_layer, 105,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_cv2_act_Mul_output_0_chain,
  NULL, &_model_8_m_0_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_cv1_conv_Conv_output_0_output, &_model_8_m_0_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_cv1_act_Mul_output_0_layer, 88,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_cv1_act_Mul_output_0_chain,
  NULL, &_model_8_m_0_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_m_m_0_cv1_conv_Conv_output_0_output, &_model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_m_m_0_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv1_act_Mul_output_0_layer, 91,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_m_m_0_cv1_act_Mul_output_0_chain,
  NULL, &_model_8_m_0_m_m_0_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_m_m_0_cv2_conv_Conv_output_0_output, &_model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_m_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_m_m_0_cv2_act_Mul_output_0_layer, 94,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_m_m_0_cv2_act_Mul_output_0_chain,
  NULL, &_model_8_m_0_m_m_0_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_m_m_0_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_cv1_act_Mul_output_0_output, &_model_8_m_0_m_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_m_m_0_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_m_m_0_Add_output_0_layer, 95,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_m_m_0_Add_output_0_chain,
  NULL, &_model_8_m_0_m_m_0_Add_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_m_m_1_cv1_conv_Conv_output_0_output, &_model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_m_m_1_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv1_act_Mul_output_0_layer, 98,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_m_m_1_cv1_act_Mul_output_0_chain,
  NULL, &_model_8_m_0_m_m_1_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_m_m_1_cv2_conv_Conv_output_0_output, &_model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_m_m_1_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_m_m_1_cv2_act_Mul_output_0_layer, 101,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_m_m_1_cv2_act_Mul_output_0_chain,
  NULL, &_model_8_m_0_m_m_1_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_m_m_1_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_m_m_0_Add_output_0_output, &_model_8_m_0_m_m_1_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_m_m_1_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_m_m_1_Add_output_0_layer, 102,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_m_m_1_Add_output_0_chain,
  NULL, &_model_8_m_0_m_m_1_Add_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_Concat_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_m_m_1_Add_output_0_output, &_model_8_m_0_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_Concat_output_0_layer, 106,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &_model_8_m_0_Concat_output_0_chain,
  NULL, &_model_8_m_0_Concat_output_0_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_m_0_cv3_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_m_0_cv3_conv_Conv_output_0_output, &_model_8_m_0_cv3_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_m_0_cv3_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_m_0_cv3_act_Mul_output_0_layer, 109,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_m_0_cv3_act_Mul_output_0_chain,
  NULL, &_model_8_m_0_cv3_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_Concat_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_8_Split_output_0_output0, &_model_8_Split_output_0_output1, &_model_8_m_0_cv3_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_Concat_output_0_layer, 110,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &_model_8_Concat_output_0_chain,
  NULL, &_model_8_Concat_output_0_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_8_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_8_cv2_conv_Conv_output_0_output, &_model_8_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_8_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_8_cv2_act_Mul_output_0_layer, 113,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_8_cv2_act_Mul_output_0_chain,
  NULL, &_model_8_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_cv1_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_9_cv1_conv_Conv_output_0_output, &_model_9_cv1_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_cv1_act_Mul_output_0_layer, 116,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_9_cv1_act_Mul_output_0_chain,
  NULL, &_model_9_cv1_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_Split_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_cv1_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_9_Split_output_0_output0, &_model_9_Split_output_0_output1),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_Split_output_0_num_or_size_splits),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_Split_output_0_layer, 117,
  SPLIT_TYPE, 0x0, NULL,
  split, forward_split,
  &_model_9_Split_output_0_chain,
  NULL, &_model_9_Split_output_0_layer, AI_STATIC, 
  .outer_elems = 49, 
  .outer_elems_stride = 1024, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_qkv_conv_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Reshape_output_0_to_chlast_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_layer, 120,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &_model_9_m_m_0_attn_Reshape_output_0_to_chlast_chain,
  NULL, &_model_9_m_m_0_attn_Reshape_output_0_to_chlast_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Reshape_output_0_to_chlast_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_layer, 120,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &_model_9_m_m_0_attn_Reshape_output_0_to_chfirst_chain,
  NULL, &_model_9_m_m_0_attn_Reshape_output_0_to_chfirst_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_9_m_m_0_attn_Split_output_0_output0, &_model_9_m_m_0_attn_Split_output_0_output1, &_model_9_m_m_0_attn_Split_output_0_output2),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Split_output_0_num_or_size_splits),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_Split_output_0_layer, 122,
  SPLIT_TYPE, 0x0, NULL,
  split, forward_split,
  &_model_9_m_m_0_attn_Split_output_0_chain,
  NULL, &_model_9_m_m_0_attn_Split_output_0_layer, AI_STATIC, 
  .outer_elems = 1, 
  .outer_elems_stride = 50176, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Split_output_0_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_layer, 124,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_chain,
  NULL, &transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Split_output_0_output2),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_layer, 129,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_chain,
  NULL, &transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Split_output_0_output1),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_layer, 124,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_chain,
  NULL, &transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output, &transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output, &_model_9_m_m_0_attn_MatMul_output_0_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_MatMul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_output_0_layer, 124,
  MATMUL_TYPE, 0x0, NULL,
  matmul, forward_matmul,
  &_model_9_m_m_0_attn_MatMul_output_0_chain,
  NULL, &_model_9_m_m_0_attn_MatMul_output_0_layer, AI_STATIC, 
  .alpha = 1.0, 
  .beta = 1.0, 
  .tA = 0, 
  .tB = 0, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_MatMul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_layer, 124,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_chain,
  NULL, &transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Softmax_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_layer, 129,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_chain,
  NULL, &transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_1_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output, &transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output, &_model_9_m_m_0_attn_MatMul_1_output_0_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_MatMul_1_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_MatMul_1_output_0_layer, 129,
  MATMUL_TYPE, 0x0, NULL,
  matmul, forward_matmul,
  &_model_9_m_m_0_attn_MatMul_1_output_0_chain,
  NULL, &_model_9_m_m_0_attn_MatMul_1_output_0_layer, AI_STATIC, 
  .alpha = 1.0, 
  .beta = 1.0, 
  .tA = 0, 
  .tB = 0, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_MatMul_1_output_0_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_layer, 132,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &_model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_chain,
  NULL, &_model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Split_output_0_output2),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_layer, 133,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &_model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_chain,
  NULL, &_model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_layer, 133,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &_model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_chain,
  NULL, &_model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_attn_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output, &_model_9_m_m_0_attn_pe_conv_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_attn_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_attn_Add_output_0_layer, 135,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_9_m_m_0_attn_Add_output_0_chain,
  NULL, &_model_9_m_m_0_attn_Add_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_9_Split_output_0_output1, &_model_9_m_m_0_attn_proj_conv_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_Add_output_0_layer, 137,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_9_m_m_0_Add_output_0_chain,
  NULL, &_model_9_m_m_0_Add_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_output, &_model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_layer, 140,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_chain,
  NULL, &_model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_m_m_0_Add_1_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_9_m_m_0_Add_output_0_output, &_model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_m_m_0_Add_1_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_m_m_0_Add_1_output_0_layer, 142,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_9_m_m_0_Add_1_output_0_chain,
  NULL, &_model_9_m_m_0_Add_1_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_Concat_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_9_Split_output_0_output0, &_model_9_m_m_0_Add_1_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_Concat_output_0_layer, 143,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &_model_9_Concat_output_0_chain,
  NULL, &_model_9_Concat_output_0_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_9_cv2_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_9_cv2_conv_Conv_output_0_output, &_model_9_cv2_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_9_cv2_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_9_cv2_act_Mul_output_0_layer, 146,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_9_cv2_act_Mul_output_0_chain,
  NULL, &_model_9_cv2_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_10_conv_act_Mul_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_10_conv_conv_Conv_output_0_output, &_model_10_conv_act_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_10_conv_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_10_conv_act_Mul_output_0_layer, 149,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_10_conv_act_Mul_output_0_chain,
  NULL, &_model_10_conv_act_Mul_output_0_layer, AI_STATIC, 
  .operation = ai_mul_f32, 
  .buffer_operation = ai_mul_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_10_pool_GlobalAveragePool_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_10_conv_act_Mul_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_10_pool_GlobalAveragePool_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_10_pool_GlobalAveragePool_output_0_layer, 150,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap,
  &_model_10_pool_GlobalAveragePool_output_0_chain,
  NULL, &_model_10_pool_GlobalAveragePool_output_0_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(7, 7), 
  .pool_stride = AI_SHAPE_2D_INIT(7, 7), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_transpose_images_Transpose(_stai_network_context* net_ctx)
{
  images_output_array.data = AI_PTR(net_ctx->_inputs[0] + 0);
  images_output_array.data_start = AI_PTR(net_ctx->_inputs[0] + 0);
  images_Transpose_output_array.data = AI_PTR(net_ctx->_activations[0] + 1015296);
  images_Transpose_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1015296);
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, { images_output.data->data});
  forward_transpose(&images_Transpose_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, { images_Transpose_output.data->data});
}
void forward_lite_eltwise__model_0_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_0_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_0_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_0_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  _model_0_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _model_0_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_0_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802816);
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 2, { _model_0_conv_Conv_output_0_output.data->data,_model_0_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_0_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, { _model_0_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 576);
  _model_1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 576);
  _model_1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 401984);
  _model_1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 401984);
  _model_1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803392);
  _model_1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803392);
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 2, { _model_1_conv_Conv_output_0_output.data->data,_model_1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, { _model_1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_2_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_2_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  _model_2_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  _model_2_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 401536);
  _model_2_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 401536);
  _model_2_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 802944);
  _model_2_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802944);
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 2, { _model_2_cv1_conv_Conv_output_0_output.data->data,_model_2_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_2_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, { _model_2_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_split__model_2_Split_output_0(_stai_network_context* net_ctx)
{
  _model_2_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 802944);
  _model_2_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802944);
  _model_2_Split_output_0_num_or_size_splits_array.data = AI_PTR(net_ctx->_weights[0] + 24584);
  _model_2_Split_output_0_num_or_size_splits_array.data_start = AI_PTR(net_ctx->_weights[0] + 24584);
  _model_2_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 200704);
  _model_2_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 200704);
  _model_2_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  _model_2_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, { _model_2_cv1_act_Mul_output_0_output.data->data});
  forward_split(&_model_2_Split_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 2, { _model_2_Split_output_0_output0.data->data,_model_2_Split_output_0_output1.data->data});
}
void forward_lite_eltwise__model_2_m_0_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_2_m_0_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 401984);
  _model_2_m_0_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 401984);
  _model_2_m_0_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 502336);
  _model_2_m_0_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 502336);
  _model_2_m_0_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602688);
  _model_2_m_0_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602688);
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 2, { _model_2_m_0_cv1_conv_Conv_output_0_output.data->data,_model_2_m_0_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_2_m_0_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, { _model_2_m_0_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_2_m_0_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_2_m_0_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 401696);
  _model_2_m_0_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 401696);
  _model_2_m_0_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602400);
  _model_2_m_0_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602400);
  _model_2_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803104);
  _model_2_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803104);
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 2, { _model_2_m_0_cv2_conv_Conv_output_0_output.data->data,_model_2_m_0_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_2_m_0_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, { _model_2_m_0_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_2_m_0_Add_output_0(_stai_network_context* net_ctx)
{
  _model_2_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 200704);
  _model_2_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 200704);
  _model_2_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803104);
  _model_2_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803104);
  _model_2_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 401408);
  _model_2_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 401408);
  _STAI_NETWORK_EVENT_NODE_START_CB(18, 2, { _model_2_Split_output_0_output1.data->data,_model_2_m_0_cv2_act_Mul_output_0_output.data->data});
  forward_eltwise(&_model_2_m_0_Add_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(18, 1, { _model_2_m_0_Add_output_0_output.data->data});
}
void forward_lite_concat__model_2_Concat_output_0(_stai_network_context* net_ctx)
{
  _model_2_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  _model_2_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _model_2_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 200704);
  _model_2_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 200704);
  _model_2_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 401408);
  _model_2_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 401408);
  _model_2_Concat_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_2_Concat_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602112);
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 3, { _model_2_Split_output_0_output0.data->data,_model_2_Split_output_0_output1.data->data,_model_2_m_0_Add_output_0_output.data->data});
  forward_concat(&_model_2_Concat_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, { _model_2_Concat_output_0_output.data->data});
}
void forward_lite_eltwise__model_2_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_2_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 1204224);
  _model_2_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1204224);
  _model_2_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 401408);
  _model_2_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 401408);
  _model_2_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 1204224);
  _model_2_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1204224);
  _STAI_NETWORK_EVENT_NODE_START_CB(22, 2, { _model_2_cv2_conv_Conv_output_0_output.data->data,_model_2_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_2_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(22, 1, { _model_2_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_3_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_3_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 403712);
  _model_3_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 403712);
  _model_3_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 604416);
  _model_3_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 604416);
  _model_3_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 805120);
  _model_3_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 805120);
  _STAI_NETWORK_EVENT_NODE_START_CB(25, 2, { _model_3_conv_Conv_output_0_output.data->data,_model_3_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_3_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(25, 1, { _model_3_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_4_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_4_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 401664);
  _model_4_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 401664);
  _model_4_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602368);
  _model_4_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602368);
  _model_4_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_4_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _STAI_NETWORK_EVENT_NODE_START_CB(28, 2, { _model_4_cv1_conv_Conv_output_0_output.data->data,_model_4_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_4_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(28, 1, { _model_4_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_split__model_4_Split_output_0(_stai_network_context* net_ctx)
{
  _model_4_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_4_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_4_Split_output_0_num_or_size_splits_array.data = AI_PTR(net_ctx->_weights[0] + 210796);
  _model_4_Split_output_0_num_or_size_splits_array.data_start = AI_PTR(net_ctx->_weights[0] + 210796);
  _model_4_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 501760);
  _model_4_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 501760);
  _model_4_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 401408);
  _model_4_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 401408);
  _STAI_NETWORK_EVENT_NODE_START_CB(30, 1, { _model_4_cv1_act_Mul_output_0_output.data->data});
  forward_split(&_model_4_Split_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(30, 2, { _model_4_Split_output_0_output0.data->data,_model_4_Split_output_0_output1.data->data});
}
void forward_lite_eltwise__model_4_m_0_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_4_m_0_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 603264);
  _model_4_m_0_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 603264);
  _model_4_m_0_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 653440);
  _model_4_m_0_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 653440);
  _model_4_m_0_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 703616);
  _model_4_m_0_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 703616);
  _STAI_NETWORK_EVENT_NODE_START_CB(33, 2, { _model_4_m_0_cv1_conv_Conv_output_0_output.data->data,_model_4_m_0_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_4_m_0_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(33, 1, { _model_4_m_0_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_4_m_0_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_4_m_0_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602688);
  _model_4_m_0_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602688);
  _model_4_m_0_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 703040);
  _model_4_m_0_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 703040);
  _model_4_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803392);
  _model_4_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803392);
  _STAI_NETWORK_EVENT_NODE_START_CB(36, 2, { _model_4_m_0_cv2_conv_Conv_output_0_output.data->data,_model_4_m_0_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_4_m_0_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(36, 1, { _model_4_m_0_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_4_m_0_Add_output_0(_stai_network_context* net_ctx)
{
  _model_4_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 501760);
  _model_4_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 501760);
  _model_4_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803392);
  _model_4_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803392);
  _model_4_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_4_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602112);
  _STAI_NETWORK_EVENT_NODE_START_CB(37, 2, { _model_4_Split_output_0_output1.data->data,_model_4_m_0_cv2_act_Mul_output_0_output.data->data});
  forward_eltwise(&_model_4_m_0_Add_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(37, 1, { _model_4_m_0_Add_output_0_output.data->data});
}
void forward_lite_concat__model_4_Concat_output_0(_stai_network_context* net_ctx)
{
  _model_4_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 401408);
  _model_4_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 401408);
  _model_4_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 501760);
  _model_4_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 501760);
  _model_4_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_4_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_4_Concat_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 702464);
  _model_4_Concat_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 702464);
  _STAI_NETWORK_EVENT_NODE_START_CB(38, 3, { _model_4_Split_output_0_output0.data->data,_model_4_Split_output_0_output1.data->data,_model_4_m_0_Add_output_0_output.data->data});
  forward_concat(&_model_4_Concat_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(38, 1, { _model_4_Concat_output_0_output.data->data});
}
void forward_lite_eltwise__model_4_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_4_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 1003520);
  _model_4_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1003520);
  _model_4_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_4_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_4_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 1404928);
  _model_4_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1404928);
  _STAI_NETWORK_EVENT_NODE_START_CB(41, 2, { _model_4_cv2_conv_Conv_output_0_output.data->data,_model_4_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_4_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(41, 1, { _model_4_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_5_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_5_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 606720);
  _model_5_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 606720);
  _model_5_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 707072);
  _model_5_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 707072);
  _model_5_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 807424);
  _model_5_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 807424);
  _STAI_NETWORK_EVENT_NODE_START_CB(44, 2, { _model_5_conv_Conv_output_0_output.data->data,_model_5_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_5_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(44, 1, { _model_5_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 602624);
  _model_6_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 602624);
  _model_6_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 702976);
  _model_6_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 702976);
  _model_6_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_6_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _STAI_NETWORK_EVENT_NODE_START_CB(47, 2, { _model_6_cv1_conv_Conv_output_0_output.data->data,_model_6_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(47, 1, { _model_6_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_split__model_6_Split_output_0(_stai_network_context* net_ctx)
{
  _model_6_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_6_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_6_Split_output_0_num_or_size_splits_array.data = AI_PTR(net_ctx->_weights[0] + 953904);
  _model_6_Split_output_0_num_or_size_splits_array.data_start = AI_PTR(net_ctx->_weights[0] + 953904);
  _model_6_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 652288);
  _model_6_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 652288);
  _model_6_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_6_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 602112);
  _STAI_NETWORK_EVENT_NODE_START_CB(49, 1, { _model_6_cv1_act_Mul_output_0_output.data->data});
  forward_split(&_model_6_Split_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(49, 2, { _model_6_Split_output_0_output0.data->data,_model_6_Split_output_0_output1.data->data});
}
void forward_lite_eltwise__model_6_m_0_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 702720);
  _model_6_m_0_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 702720);
  _model_6_m_0_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 727808);
  _model_6_m_0_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 727808);
  _model_6_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 752896);
  _model_6_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 752896);
  _STAI_NETWORK_EVENT_NODE_START_CB(69, 2, { _model_6_m_0_cv2_conv_Conv_output_0_output.data->data,_model_6_m_0_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(69, 1, { _model_6_m_0_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_m_0_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 702720);
  _model_6_m_0_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 702720);
  _model_6_m_0_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 727808);
  _model_6_m_0_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 727808);
  _model_6_m_0_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 777984);
  _model_6_m_0_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 777984);
  _STAI_NETWORK_EVENT_NODE_START_CB(52, 2, { _model_6_m_0_cv1_conv_Conv_output_0_output.data->data,_model_6_m_0_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(52, 1, { _model_6_m_0_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_m_0_m_m_0_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 703616);
  _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 703616);
  _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_m_m_0_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 828160);
  _model_6_m_0_m_m_0_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 828160);
  _STAI_NETWORK_EVENT_NODE_START_CB(55, 2, { _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_output.data->data,_model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_m_m_0_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(55, 1, { _model_6_m_0_m_m_0_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_m_0_m_m_0_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 703616);
  _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 703616);
  _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_m_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 828160);
  _model_6_m_0_m_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 828160);
  _STAI_NETWORK_EVENT_NODE_START_CB(58, 2, { _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_output.data->data,_model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_m_m_0_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(58, 1, { _model_6_m_0_m_m_0_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_m_0_m_m_0_Add_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 777984);
  _model_6_m_0_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 777984);
  _model_6_m_0_m_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 828160);
  _model_6_m_0_m_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 828160);
  _model_6_m_0_m_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 702464);
  _model_6_m_0_m_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 702464);
  _STAI_NETWORK_EVENT_NODE_START_CB(59, 2, { _model_6_m_0_cv1_act_Mul_output_0_output.data->data,_model_6_m_0_m_m_0_cv2_act_Mul_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_m_m_0_Add_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(59, 1, { _model_6_m_0_m_m_0_Add_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_m_0_m_m_1_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 777984);
  _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 777984);
  _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_6_m_0_m_m_1_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_m_m_1_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _STAI_NETWORK_EVENT_NODE_START_CB(62, 2, { _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_output.data->data,_model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_m_m_1_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(62, 1, { _model_6_m_0_m_m_1_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_m_0_m_m_1_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 777984);
  _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 777984);
  _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_6_m_0_m_m_1_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_m_m_1_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _STAI_NETWORK_EVENT_NODE_START_CB(65, 2, { _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_output.data->data,_model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_m_m_1_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(65, 1, { _model_6_m_0_m_m_1_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_m_0_m_m_1_Add_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_m_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 702464);
  _model_6_m_0_m_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 702464);
  _model_6_m_0_m_m_1_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_m_m_1_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_m_m_1_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_6_m_0_m_m_1_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 727552);
  _STAI_NETWORK_EVENT_NODE_START_CB(66, 2, { _model_6_m_0_m_m_0_Add_output_0_output.data->data,_model_6_m_0_m_m_1_cv2_act_Mul_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_m_m_1_Add_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(66, 1, { _model_6_m_0_m_m_1_Add_output_0_output.data->data});
}
void forward_lite_concat__model_6_m_0_Concat_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_m_m_1_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_6_m_0_m_m_1_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_6_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 752896);
  _model_6_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 752896);
  _model_6_m_0_Concat_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 777984);
  _model_6_m_0_Concat_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 777984);
  _STAI_NETWORK_EVENT_NODE_START_CB(70, 2, { _model_6_m_0_m_m_1_Add_output_0_output.data->data,_model_6_m_0_cv2_act_Mul_output_0_output.data->data});
  forward_concat(&_model_6_m_0_Concat_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(70, 1, { _model_6_m_0_Concat_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_m_0_cv3_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_m_0_cv3_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 702720);
  _model_6_m_0_cv3_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 702720);
  _model_6_m_0_cv3_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 752896);
  _model_6_m_0_cv3_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 752896);
  _model_6_m_0_cv3_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_cv3_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _STAI_NETWORK_EVENT_NODE_START_CB(73, 2, { _model_6_m_0_cv3_conv_Conv_output_0_output.data->data,_model_6_m_0_cv3_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_m_0_cv3_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(73, 1, { _model_6_m_0_cv3_act_Mul_output_0_output.data->data});
}
void forward_lite_concat__model_6_Concat_output_0(_stai_network_context* net_ctx)
{
  _model_6_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_6_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 602112);
  _model_6_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 652288);
  _model_6_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 652288);
  _model_6_m_0_cv3_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_m_0_cv3_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803072);
  _model_6_Concat_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 853248);
  _model_6_Concat_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 853248);
  _STAI_NETWORK_EVENT_NODE_START_CB(74, 3, { _model_6_Split_output_0_output0.data->data,_model_6_Split_output_0_output1.data->data,_model_6_m_0_cv3_act_Mul_output_0_output.data->data});
  forward_concat(&_model_6_Concat_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(74, 1, { _model_6_Concat_output_0_output.data->data});
}
void forward_lite_eltwise__model_6_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_6_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 703232);
  _model_6_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 703232);
  _model_6_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803584);
  _model_6_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803584);
  _model_6_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 903936);
  _model_6_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 903936);
  _STAI_NETWORK_EVENT_NODE_START_CB(77, 2, { _model_6_cv2_conv_Conv_output_0_output.data->data,_model_6_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_6_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(77, 1, { _model_6_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_7_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_7_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 707072);
  _model_7_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 707072);
  _model_7_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 757248);
  _model_7_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 757248);
  _model_7_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 807424);
  _model_7_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 807424);
  _STAI_NETWORK_EVENT_NODE_START_CB(80, 2, { _model_7_conv_Conv_output_0_output.data->data,_model_7_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_7_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(80, 1, { _model_7_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 703488);
  _model_8_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 703488);
  _model_8_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 753664);
  _model_8_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 753664);
  _model_8_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803840);
  _model_8_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803840);
  _STAI_NETWORK_EVENT_NODE_START_CB(83, 2, { _model_8_cv1_conv_Conv_output_0_output.data->data,_model_8_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(83, 1, { _model_8_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_split__model_8_Split_output_0(_stai_network_context* net_ctx)
{
  _model_8_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803840);
  _model_8_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803840);
  _model_8_Split_output_0_num_or_size_splits_array.data = AI_PTR(net_ctx->_weights[0] + 2677812);
  _model_8_Split_output_0_num_or_size_splits_array.data_start = AI_PTR(net_ctx->_weights[0] + 2677812);
  _model_8_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_8_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_8_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 702464);
  _model_8_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 702464);
  _STAI_NETWORK_EVENT_NODE_START_CB(85, 1, { _model_8_cv1_act_Mul_output_0_output.data->data});
  forward_split(&_model_8_Split_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(85, 2, { _model_8_Split_output_0_output0.data->data,_model_8_Split_output_0_output1.data->data});
}
void forward_lite_eltwise__model_8_m_0_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 753152);
  _model_8_m_0_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 753152);
  _model_8_m_0_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 765696);
  _model_8_m_0_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 765696);
  _model_8_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 778240);
  _model_8_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 778240);
  _STAI_NETWORK_EVENT_NODE_START_CB(105, 2, { _model_8_m_0_cv2_conv_Conv_output_0_output.data->data,_model_8_m_0_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(105, 1, { _model_8_m_0_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_m_0_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 753152);
  _model_8_m_0_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 753152);
  _model_8_m_0_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 765696);
  _model_8_m_0_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 765696);
  _model_8_m_0_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 790784);
  _model_8_m_0_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 790784);
  _STAI_NETWORK_EVENT_NODE_START_CB(88, 2, { _model_8_m_0_cv1_conv_Conv_output_0_output.data->data,_model_8_m_0_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(88, 1, { _model_8_m_0_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_m_0_m_m_0_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 754944);
  _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 754944);
  _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_m_m_0_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 815872);
  _model_8_m_0_m_m_0_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 815872);
  _STAI_NETWORK_EVENT_NODE_START_CB(91, 2, { _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_output.data->data,_model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_m_m_0_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(91, 1, { _model_8_m_0_m_m_0_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_m_0_m_m_0_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 754944);
  _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 754944);
  _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_m_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 815872);
  _model_8_m_0_m_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 815872);
  _STAI_NETWORK_EVENT_NODE_START_CB(94, 2, { _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_output.data->data,_model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_m_m_0_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(94, 1, { _model_8_m_0_m_m_0_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_m_0_m_m_0_Add_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 790784);
  _model_8_m_0_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 790784);
  _model_8_m_0_m_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 815872);
  _model_8_m_0_m_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 815872);
  _model_8_m_0_m_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_8_m_0_m_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 752640);
  _STAI_NETWORK_EVENT_NODE_START_CB(95, 2, { _model_8_m_0_cv1_act_Mul_output_0_output.data->data,_model_8_m_0_m_m_0_cv2_act_Mul_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_m_m_0_Add_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(95, 1, { _model_8_m_0_m_m_0_Add_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_m_0_m_m_1_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 790784);
  _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 790784);
  _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 765184);
  _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 765184);
  _model_8_m_0_m_m_1_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_m_m_1_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _STAI_NETWORK_EVENT_NODE_START_CB(98, 2, { _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_output.data->data,_model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_m_m_1_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(98, 1, { _model_8_m_0_m_m_1_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_m_0_m_m_1_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 790784);
  _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 790784);
  _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 765184);
  _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 765184);
  _model_8_m_0_m_m_1_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_m_m_1_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _STAI_NETWORK_EVENT_NODE_START_CB(101, 2, { _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_output.data->data,_model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_m_m_1_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(101, 1, { _model_8_m_0_m_m_1_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_m_0_m_m_1_Add_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_m_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_8_m_0_m_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_8_m_0_m_m_1_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_m_m_1_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_m_m_1_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 765184);
  _model_8_m_0_m_m_1_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 765184);
  _STAI_NETWORK_EVENT_NODE_START_CB(102, 2, { _model_8_m_0_m_m_0_Add_output_0_output.data->data,_model_8_m_0_m_m_1_cv2_act_Mul_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_m_m_1_Add_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(102, 1, { _model_8_m_0_m_m_1_Add_output_0_output.data->data});
}
void forward_lite_concat__model_8_m_0_Concat_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_m_m_1_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 765184);
  _model_8_m_0_m_m_1_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 765184);
  _model_8_m_0_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 778240);
  _model_8_m_0_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 778240);
  _model_8_m_0_Concat_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 790784);
  _model_8_m_0_Concat_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 790784);
  _STAI_NETWORK_EVENT_NODE_START_CB(106, 2, { _model_8_m_0_m_m_1_Add_output_0_output.data->data,_model_8_m_0_cv2_act_Mul_output_0_output.data->data});
  forward_concat(&_model_8_m_0_Concat_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(106, 1, { _model_8_m_0_Concat_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_m_0_cv3_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_m_0_cv3_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 753152);
  _model_8_m_0_cv3_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 753152);
  _model_8_m_0_cv3_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 778240);
  _model_8_m_0_cv3_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 778240);
  _model_8_m_0_cv3_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_cv3_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _STAI_NETWORK_EVENT_NODE_START_CB(109, 2, { _model_8_m_0_cv3_conv_Conv_output_0_output.data->data,_model_8_m_0_cv3_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_m_0_cv3_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(109, 1, { _model_8_m_0_cv3_act_Mul_output_0_output.data->data});
}
void forward_lite_concat__model_8_Concat_output_0(_stai_network_context* net_ctx)
{
  _model_8_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 702464);
  _model_8_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 702464);
  _model_8_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_8_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 727552);
  _model_8_m_0_cv3_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_m_0_cv3_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_8_Concat_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 828416);
  _model_8_Concat_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 828416);
  _STAI_NETWORK_EVENT_NODE_START_CB(110, 3, { _model_8_Split_output_0_output0.data->data,_model_8_Split_output_0_output1.data->data,_model_8_m_0_cv3_act_Mul_output_0_output.data->data});
  forward_concat(&_model_8_Concat_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(110, 1, { _model_8_Concat_output_0_output.data->data});
}
void forward_lite_eltwise__model_8_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_8_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 754176);
  _model_8_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 754176);
  _model_8_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 804352);
  _model_8_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 804352);
  _model_8_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 854528);
  _model_8_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 854528);
  _STAI_NETWORK_EVENT_NODE_START_CB(113, 2, { _model_8_cv2_conv_Conv_output_0_output.data->data,_model_8_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_8_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(113, 1, { _model_8_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_9_cv1_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_9_cv1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 753664);
  _model_9_cv1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 753664);
  _model_9_cv1_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803840);
  _model_9_cv1_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803840);
  _model_9_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 854016);
  _model_9_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 854016);
  _STAI_NETWORK_EVENT_NODE_START_CB(116, 2, { _model_9_cv1_conv_Conv_output_0_output.data->data,_model_9_cv1_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_9_cv1_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(116, 1, { _model_9_cv1_act_Mul_output_0_output.data->data});
}
void forward_lite_split__model_9_Split_output_0(_stai_network_context* net_ctx)
{
  _model_9_cv1_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 854016);
  _model_9_cv1_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 854016);
  _model_9_Split_output_0_num_or_size_splits_array.data = AI_PTR(net_ctx->_weights[0] + 4058168);
  _model_9_Split_output_0_num_or_size_splits_array.data_start = AI_PTR(net_ctx->_weights[0] + 4058168);
  _model_9_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 777728);
  _model_9_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 777728);
  _model_9_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_9_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 752640);
  _STAI_NETWORK_EVENT_NODE_START_CB(117, 1, { _model_9_cv1_act_Mul_output_0_output.data->data});
  forward_split(&_model_9_Split_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(117, 2, { _model_9_Split_output_0_output0.data->data,_model_9_Split_output_0_output1.data->data});
}
void forward_lite_transpose__model_9_m_m_0_attn_Reshape_output_0_to_chlast(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_qkv_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_9_m_m_0_attn_qkv_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output_array.data = AI_PTR(net_ctx->_activations[0] + 853504);
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 853504);
  _STAI_NETWORK_EVENT_NODE_START_CB(120, 1, { _model_9_m_m_0_attn_qkv_conv_Conv_output_0_output.data->data});
  forward_transpose(&_model_9_m_m_0_attn_Reshape_output_0_to_chlast_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(120, 1, { _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output.data->data});
}
void forward_lite_transpose__model_9_m_m_0_attn_Reshape_output_0_to_chfirst(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output_array.data = AI_PTR(net_ctx->_activations[0] + 853504);
  _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 853504);
  _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output_array.data = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802816);
  _STAI_NETWORK_EVENT_NODE_START_CB(120, 1, { _model_9_m_m_0_attn_Reshape_output_0_to_chlast_output0.data->data});
  forward_transpose(&_model_9_m_m_0_attn_Reshape_output_0_to_chfirst_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(120, 1, { _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output.data->data});
}
void forward_lite_split__model_9_m_m_0_attn_Split_output_0(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output_array.data = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_9_m_m_0_attn_Split_output_0_num_or_size_splits_array.data = AI_PTR(net_ctx->_weights[0] + 4190268);
  _model_9_m_m_0_attn_Split_output_0_num_or_size_splits_array.data_start = AI_PTR(net_ctx->_weights[0] + 4190268);
  _model_9_m_m_0_attn_Split_output_0_output2_array.data = AI_PTR(net_ctx->_activations[0] + 878080);
  _model_9_m_m_0_attn_Split_output_0_output2_array.data_start = AI_PTR(net_ctx->_activations[0] + 878080);
  _model_9_m_m_0_attn_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 865536);
  _model_9_m_m_0_attn_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 865536);
  _model_9_m_m_0_attn_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 852992);
  _model_9_m_m_0_attn_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 852992);
  _STAI_NETWORK_EVENT_NODE_START_CB(122, 1, { _model_9_m_m_0_attn_Reshape_output_0_to_chfirst_output.data->data});
  forward_split(&_model_9_m_m_0_attn_Split_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(122, 3, { _model_9_m_m_0_attn_Split_output_0_output0.data->data,_model_9_m_m_0_attn_Split_output_0_output1.data->data,_model_9_m_m_0_attn_Split_output_0_output2.data->data});
}
void forward_lite_transpose_transpose_a_model_9_m_m_0_attn_MatMul_output_0_out(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 852992);
  _model_9_m_m_0_attn_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 852992);
  transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 802816);
  transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802816);
  _STAI_NETWORK_EVENT_NODE_START_CB(124, 1, { _model_9_m_m_0_attn_Split_output_0_output0.data->data});
  forward_transpose(&transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(124, 1, { transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output.data->data});
}
void forward_lite_transpose_transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Split_output_0_output2_array.data = AI_PTR(net_ctx->_activations[0] + 878080);
  _model_9_m_m_0_attn_Split_output_0_output2_array.data_start = AI_PTR(net_ctx->_activations[0] + 878080);
  transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 815360);
  transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 815360);
  _STAI_NETWORK_EVENT_NODE_START_CB(129, 1, { _model_9_m_m_0_attn_Split_output_0_output2.data->data});
  forward_transpose(&transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(129, 1, { transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output.data->data});
}
void forward_lite_transpose_transpose_b_model_9_m_m_0_attn_MatMul_output_0_out(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 865536);
  _model_9_m_m_0_attn_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 865536);
  transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 840448);
  transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 840448);
  _STAI_NETWORK_EVENT_NODE_START_CB(124, 1, { _model_9_m_m_0_attn_Split_output_0_output1.data->data});
  forward_transpose(&transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(124, 1, { transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output.data->data});
}
void forward_lite_matmul__model_9_m_m_0_attn_MatMul_output_0(_stai_network_context* net_ctx)
{
  transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 802816);
  transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802816);
  transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 840448);
  transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 840448);
  _model_9_m_m_0_attn_MatMul_output_0_bias_array.data = AI_PTR(net_ctx->_weights[0] + 4);
  _model_9_m_m_0_attn_MatMul_output_0_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 4);
  _model_9_m_m_0_attn_MatMul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 852992);
  _model_9_m_m_0_attn_MatMul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 852992);
  _STAI_NETWORK_EVENT_NODE_START_CB(124, 3, { transpose_a_model_9_m_m_0_attn_MatMul_output_0_out_output.data->data,transpose_b_model_9_m_m_0_attn_MatMul_output_0_out_output.data->data,_model_9_m_m_0_attn_MatMul_output_0_bias.data->data});
  forward_matmul(&_model_9_m_m_0_attn_MatMul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(124, 1, { _model_9_m_m_0_attn_MatMul_output_0_output.data->data});
}
void forward_lite_transpose_transpose_out_model_9_m_m_0_attn_MatMul_output_0_out(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_MatMul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 852992);
  _model_9_m_m_0_attn_MatMul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 852992);
  transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 903168);
  transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 903168);
  _STAI_NETWORK_EVENT_NODE_START_CB(124, 1, { _model_9_m_m_0_attn_MatMul_output_0_output.data->data});
  forward_transpose(&transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(124, 1, { transpose_out_model_9_m_m_0_attn_MatMul_output_0_out_output.data->data});
}
void forward_lite_transpose_transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Softmax_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 903168);
  _model_9_m_m_0_attn_Softmax_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 903168);
  transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 840448);
  transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 840448);
  _STAI_NETWORK_EVENT_NODE_START_CB(129, 1, { _model_9_m_m_0_attn_Softmax_output_0_output.data->data});
  forward_transpose(&transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(129, 1, { transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output.data->data});
}
void forward_lite_matmul__model_9_m_m_0_attn_MatMul_1_output_0(_stai_network_context* net_ctx)
{
  transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 815360);
  transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 815360);
  transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array.data = AI_PTR(net_ctx->_activations[0] + 840448);
  transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 840448);
  _model_9_m_m_0_attn_MatMul_1_output_0_bias_array.data = AI_PTR(net_ctx->_weights[0] + 0);
  _model_9_m_m_0_attn_MatMul_1_output_0_bias_array.data_start = AI_PTR(net_ctx->_weights[0] + 0);
  _model_9_m_m_0_attn_MatMul_1_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 903168);
  _model_9_m_m_0_attn_MatMul_1_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 903168);
  _STAI_NETWORK_EVENT_NODE_START_CB(129, 3, { transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out_output.data->data,transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out_output.data->data,_model_9_m_m_0_attn_MatMul_1_output_0_bias.data->data});
  forward_matmul(&_model_9_m_m_0_attn_MatMul_1_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(129, 1, { _model_9_m_m_0_attn_MatMul_1_output_0_output.data->data});
}
void forward_lite_transpose__model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_MatMul_1_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 903168);
  _model_9_m_m_0_attn_MatMul_1_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 903168);
  _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output_array.data = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802816);
  _STAI_NETWORK_EVENT_NODE_START_CB(132, 1, { _model_9_m_m_0_attn_MatMul_1_output_0_output0.data->data});
  forward_transpose(&_model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(132, 1, { _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output.data->data});
}
void forward_lite_transpose__model_9_m_m_0_attn_Reshape_2_output_0_to_chlast(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Split_output_0_output2_array.data = AI_PTR(net_ctx->_activations[0] + 878080);
  _model_9_m_m_0_attn_Split_output_0_output2_array.data_start = AI_PTR(net_ctx->_activations[0] + 878080);
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output_array.data = AI_PTR(net_ctx->_activations[0] + 827904);
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 827904);
  _STAI_NETWORK_EVENT_NODE_START_CB(133, 1, { _model_9_m_m_0_attn_Split_output_0_output2.data->data});
  forward_transpose(&_model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(133, 1, { _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output.data->data});
}
void forward_lite_transpose__model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output_array.data = AI_PTR(net_ctx->_activations[0] + 827904);
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 827904);
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_output_array.data = AI_PTR(net_ctx->_activations[0] + 852992);
  _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 852992);
  _STAI_NETWORK_EVENT_NODE_START_CB(133, 1, { _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast_output0.data->data});
  forward_transpose(&_model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(133, 1, { _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst_output.data->data});
}
void forward_lite_eltwise__model_9_m_m_0_attn_Add_output_0(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output_array.data = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 802816);
  _model_9_m_m_0_attn_pe_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 827904);
  _model_9_m_m_0_attn_pe_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 827904);
  _model_9_m_m_0_attn_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 852992);
  _model_9_m_m_0_attn_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 852992);
  _STAI_NETWORK_EVENT_NODE_START_CB(135, 2, { _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst_output.data->data,_model_9_m_m_0_attn_pe_conv_Conv_output_0_output.data->data});
  forward_eltwise(&_model_9_m_m_0_attn_Add_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(135, 1, { _model_9_m_m_0_attn_Add_output_0_output.data->data});
}
void forward_lite_eltwise__model_9_m_m_0_Add_output_0(_stai_network_context* net_ctx)
{
  _model_9_Split_output_0_output1_array.data = AI_PTR(net_ctx->_activations[0] + 777728);
  _model_9_Split_output_0_output1_array.data_start = AI_PTR(net_ctx->_activations[0] + 777728);
  _model_9_m_m_0_attn_proj_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_9_m_m_0_attn_proj_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 803328);
  _model_9_m_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 828416);
  _model_9_m_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 828416);
  _STAI_NETWORK_EVENT_NODE_START_CB(137, 2, { _model_9_Split_output_0_output1.data->data,_model_9_m_m_0_attn_proj_conv_Conv_output_0_output.data->data});
  forward_eltwise(&_model_9_m_m_0_Add_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(137, 1, { _model_9_m_m_0_Add_output_0_output.data->data});
}
void forward_lite_eltwise__model_9_m_m_0_ffn_ffn_0_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 778240);
  _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 778240);
  _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 853504);
  _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 853504);
  _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 903680);
  _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 903680);
  _STAI_NETWORK_EVENT_NODE_START_CB(140, 2, { _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_output.data->data,_model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(140, 1, { _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_9_m_m_0_Add_1_output_0(_stai_network_context* net_ctx)
{
  _model_9_m_m_0_Add_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 828416);
  _model_9_m_m_0_Add_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 828416);
  _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 778752);
  _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 778752);
  _model_9_m_m_0_Add_1_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 853504);
  _model_9_m_m_0_Add_1_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 853504);
  _STAI_NETWORK_EVENT_NODE_START_CB(142, 2, { _model_9_m_m_0_Add_output_0_output.data->data,_model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_output.data->data});
  forward_eltwise(&_model_9_m_m_0_Add_1_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(142, 1, { _model_9_m_m_0_Add_1_output_0_output.data->data});
}
void forward_lite_concat__model_9_Concat_output_0(_stai_network_context* net_ctx)
{
  _model_9_Split_output_0_output0_array.data = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_9_Split_output_0_output0_array.data_start = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_9_m_m_0_Add_1_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 853504);
  _model_9_m_m_0_Add_1_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 853504);
  _model_9_Concat_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 777728);
  _model_9_Concat_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 777728);
  _STAI_NETWORK_EVENT_NODE_START_CB(143, 2, { _model_9_Split_output_0_output0.data->data,_model_9_m_m_0_Add_1_output_0_output.data->data});
  forward_concat(&_model_9_Concat_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(143, 1, { _model_9_Concat_output_0_output.data->data});
}
void forward_lite_eltwise__model_9_cv2_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_9_cv2_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 827904);
  _model_9_cv2_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 827904);
  _model_9_cv2_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_9_cv2_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_9_cv2_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 878080);
  _model_9_cv2_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 878080);
  _STAI_NETWORK_EVENT_NODE_START_CB(146, 2, { _model_9_cv2_conv_Conv_output_0_output.data->data,_model_9_cv2_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_9_cv2_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(146, 1, { _model_9_cv2_act_Mul_output_0_output.data->data});
}
void forward_lite_eltwise__model_10_conv_act_Mul_output_0(_stai_network_context* net_ctx)
{
  _model_10_conv_conv_Conv_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 928256);
  _model_10_conv_conv_Conv_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 928256);
  _model_10_conv_act_Sigmoid_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 1179136);
  _model_10_conv_act_Sigmoid_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1179136);
  _model_10_conv_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 1430016);
  _model_10_conv_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1430016);
  _STAI_NETWORK_EVENT_NODE_START_CB(149, 2, { _model_10_conv_conv_Conv_output_0_output.data->data,_model_10_conv_act_Sigmoid_output_0_output.data->data});
  forward_eltwise(&_model_10_conv_act_Mul_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(149, 1, { _model_10_conv_act_Mul_output_0_output.data->data});
}
void forward_lite_ap__model_10_pool_GlobalAveragePool_output_0(_stai_network_context* net_ctx)
{
  _model_10_conv_act_Mul_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 1430016);
  _model_10_conv_act_Mul_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1430016);
  _model_10_pool_GlobalAveragePool_output_0_output_array.data = AI_PTR(net_ctx->_activations[0] + 752640);
  _model_10_pool_GlobalAveragePool_output_0_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 752640);
  _STAI_NETWORK_EVENT_NODE_START_CB(150, 1, { _model_10_conv_act_Mul_output_0_output.data->data});
  forward_ap(&_model_10_pool_GlobalAveragePool_output_0_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(150, 1, { _model_10_pool_GlobalAveragePool_output_0_output.data->data});
}

/*****************************************************************************/



static const ai_u32 _model_0_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 3;
static const ai_u32 _model_0_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 16;
static const ai_u32 _model_0_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 224;
static const ai_u32 _model_0_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 224;
static const ai_u32 _model_0_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 112;
static const ai_u32 _model_0_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 112;
static const ai_u32 _model_0_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_0_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_0_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_0_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_0_conv_Conv_output_0_l_stride_1_const_u16 = 2;
static const ai_u16 _model_0_conv_Conv_output_0_l_stride_0_const_u16 = 2;
static const ai_u16 _model_0_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_0_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_0_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 200704;


static const ai_u32 _model_1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 16;
static const ai_u32 _model_1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 112;
static const ai_u32 _model_1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 112;
static const ai_u32 _model_1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 56;
static const ai_u32 _model_1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 56;
static const ai_u32 _model_1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_1_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_1_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_1_conv_Conv_output_0_l_stride_1_const_u16 = 2;
static const ai_u16 _model_1_conv_Conv_output_0_l_stride_0_const_u16 = 2;
static const ai_u16 _model_1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 100352;


static const ai_u32 _model_2_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_2_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_2_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 56;
static const ai_u32 _model_2_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 56;
static const ai_u32 _model_2_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 56;
static const ai_u32 _model_2_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 56;
static const ai_u32 _model_2_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_2_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_2_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_2_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_2_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_2_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_2_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_2_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_2_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 100352;



static const ai_u32 _model_2_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 16;
static const ai_u32 _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 8;
static const ai_u32 _model_2_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 56;
static const ai_u32 _model_2_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 56;
static const ai_u32 _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 56;
static const ai_u32 _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 56;
static const ai_u32 _model_2_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_2_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_2_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_2_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_2_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_2_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_2_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_2_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_2_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 25088;


static const ai_u32 _model_2_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 8;
static const ai_u32 _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 16;
static const ai_u32 _model_2_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 56;
static const ai_u32 _model_2_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 56;
static const ai_u32 _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 56;
static const ai_u32 _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 56;
static const ai_u32 _model_2_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_2_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_2_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_2_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_2_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_2_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_2_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_2_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_2_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 50176;




static const ai_u32 _model_2_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 48;
static const ai_u32 _model_2_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_2_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 56;
static const ai_u32 _model_2_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 56;
static const ai_u32 _model_2_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 56;
static const ai_u32 _model_2_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 56;
static const ai_u32 _model_2_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_2_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_2_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_2_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_2_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_2_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_2_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_2_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_2_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 200704;


static const ai_u32 _model_3_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_3_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_3_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 56;
static const ai_u32 _model_3_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 56;
static const ai_u32 _model_3_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 28;
static const ai_u32 _model_3_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 28;
static const ai_u32 _model_3_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_3_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_3_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_3_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_3_conv_Conv_output_0_l_stride_1_const_u16 = 2;
static const ai_u16 _model_3_conv_Conv_output_0_l_stride_0_const_u16 = 2;
static const ai_u16 _model_3_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_3_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_3_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 50176;


static const ai_u32 _model_4_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_4_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_4_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 28;
static const ai_u32 _model_4_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 28;
static const ai_u32 _model_4_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 28;
static const ai_u32 _model_4_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 28;
static const ai_u32 _model_4_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_4_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_4_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_4_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_4_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_4_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_4_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_4_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_4_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 50176;



static const ai_u32 _model_4_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 16;
static const ai_u32 _model_4_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 28;
static const ai_u32 _model_4_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 28;
static const ai_u32 _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 28;
static const ai_u32 _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 28;
static const ai_u32 _model_4_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_4_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_4_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_4_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_4_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_4_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_4_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_4_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_4_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 12544;


static const ai_u32 _model_4_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 16;
static const ai_u32 _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_4_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 28;
static const ai_u32 _model_4_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 28;
static const ai_u32 _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 28;
static const ai_u32 _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 28;
static const ai_u32 _model_4_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_4_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_4_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_4_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_4_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_4_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_4_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_4_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_4_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 25088;




static const ai_u32 _model_4_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 96;
static const ai_u32 _model_4_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_4_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 28;
static const ai_u32 _model_4_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 28;
static const ai_u32 _model_4_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 28;
static const ai_u32 _model_4_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 28;
static const ai_u32 _model_4_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_4_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_4_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_4_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_4_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_4_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_4_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_4_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_4_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 100352;


static const ai_u32 _model_5_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_5_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_5_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 28;
static const ai_u32 _model_5_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 28;
static const ai_u32 _model_5_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_5_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_5_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_5_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_5_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_5_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_5_conv_Conv_output_0_l_stride_1_const_u16 = 2;
static const ai_u16 _model_5_conv_Conv_output_0_l_stride_0_const_u16 = 2;
static const ai_u16 _model_5_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_5_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_5_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 25088;


static const ai_u32 _model_6_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_6_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_6_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_6_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_6_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_6_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_6_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 25088;



static const ai_u32 _model_6_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_6_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_6_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_6_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_6_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 6272;


static const ai_u32 _model_6_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_6_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_6_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_6_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_6_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 6272;


static const ai_u32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 6272;


static const ai_u32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 6272;



static const ai_u32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 6272;


static const ai_u32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 32;
static const ai_u32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 6272;




static const ai_u32 _model_6_m_0_cv3_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_6_m_0_cv3_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv3_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_m_0_cv3_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_6_m_0_cv3_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_6_m_0_cv3_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_6_m_0_cv3_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_6_m_0_cv3_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv3_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv3_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_m_0_cv3_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_m_0_cv3_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 12544;



static const ai_u32 _model_6_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 192;
static const ai_u32 _model_6_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_6_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 14;
static const ai_u32 _model_6_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 14;
static const ai_u32 _model_6_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_6_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_6_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_6_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_6_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_6_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_6_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_6_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_6_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 25088;


static const ai_u32 _model_7_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_7_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_7_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 14;
static const ai_u32 _model_7_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 14;
static const ai_u32 _model_7_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_7_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_7_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_7_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_7_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_7_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_7_conv_Conv_output_0_l_stride_1_const_u16 = 2;
static const ai_u16 _model_7_conv_Conv_output_0_l_stride_0_const_u16 = 2;
static const ai_u16 _model_7_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_7_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_7_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 12544;


static const ai_u32 _model_8_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_8_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_8_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_8_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_8_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_8_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_8_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 12544;



static const ai_u32 _model_8_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_8_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_8_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_8_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_8_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 3136;


static const ai_u32 _model_8_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_8_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_8_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_8_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_8_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 3136;


static const ai_u32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 3136;


static const ai_u32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 3136;



static const ai_u32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 3136;


static const ai_u32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 64;
static const ai_u32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 3136;




static const ai_u32 _model_8_m_0_cv3_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_8_m_0_cv3_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv3_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_m_0_cv3_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_8_m_0_cv3_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_8_m_0_cv3_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_8_m_0_cv3_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_8_m_0_cv3_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv3_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv3_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_m_0_cv3_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_m_0_cv3_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 6272;



static const ai_u32 _model_8_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 384;
static const ai_u32 _model_8_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_8_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_8_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_8_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_8_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_8_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_8_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_8_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_8_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_8_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_8_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_8_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 12544;


static const ai_u32 _model_9_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_9_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_9_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_9_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_9_cv1_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_9_cv1_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_9_cv1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_9_cv1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_9_cv1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_9_cv1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_9_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 12544;



static const ai_u32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_dilation_W_const_u16 = 1;










static const ai_i32 _model_9_m_m_0_attn_Softmax_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 4802;






static const ai_u32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 3;
static const ai_u32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 3;
static const ai_i32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_pad_W_0_const_s32 = 1;
static const ai_i32 _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_pad_H_0_const_s32 = 1;
static const ai_u16 _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_dilation_W_const_u16 = 1;


static const ai_u32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_dilation_W_const_u16 = 1;


static const ai_u32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 12544;


static const ai_u32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 128;
static const ai_u32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_dilation_W_const_u16 = 1;



static const ai_u32 _model_9_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_9_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_9_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_9_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_9_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_9_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_9_cv2_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_9_cv2_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_9_cv2_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_9_cv2_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_9_cv2_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_9_cv2_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_9_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 12544;


static const ai_u32 _model_10_conv_conv_Conv_output_0_t_in_0_shape_ch_const_u32 = 256;
static const ai_u32 _model_10_conv_conv_Conv_output_0_t_out_0_shape_ch_const_u32 = 1280;
static const ai_u32 _model_10_conv_conv_Conv_output_0_t_in_0_shape_w_const_u32 = 7;
static const ai_u32 _model_10_conv_conv_Conv_output_0_t_in_0_shape_h_const_u32 = 7;
static const ai_u32 _model_10_conv_conv_Conv_output_0_t_out_0_shape_w_const_u32 = 7;
static const ai_u32 _model_10_conv_conv_Conv_output_0_t_out_0_shape_h_const_u32 = 7;
static const ai_u32 _model_10_conv_conv_Conv_output_0_t_weight_0_shape_w_const_u32 = 1;
static const ai_u32 _model_10_conv_conv_Conv_output_0_t_weight_0_shape_h_const_u32 = 1;
static const ai_i32 _model_10_conv_conv_Conv_output_0_l_pad_W_0_const_s32 = 0;
static const ai_i32 _model_10_conv_conv_Conv_output_0_l_pad_H_0_const_s32 = 0;
static const ai_u16 _model_10_conv_conv_Conv_output_0_l_stride_1_const_u16 = 1;
static const ai_u16 _model_10_conv_conv_Conv_output_0_l_stride_0_const_u16 = 1;
static const ai_u16 _model_10_conv_conv_Conv_output_0_l_dilation_H_const_u16 = 1;
static const ai_u16 _model_10_conv_conv_Conv_output_0_l_dilation_W_const_u16 = 1;

static const ai_i32 _model_10_conv_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32 = 62720;




static const ai_i32 output0_t_in_0_shape_ch_prod_const_s32 = 8;
STAI_API_ENTRY
stai_return_code stai_network_run(
  stai_network* network,
  const stai_run_mode mode)
{
   STAI_UNUSED(mode)
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_ACTIVATIONS) != STAI_FLAG_ACTIVATIONS,
        STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_INPUTS) != STAI_FLAG_INPUTS,
                  STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_OUTPUTS) != STAI_FLAG_OUTPUTS,
                  STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_WEIGHTS) != STAI_FLAG_WEIGHTS,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)


  /* LITE_KERNEL_SECTION BEGIN images_Transpose */
  {
    
  forward_lite_transpose_images_Transpose(net_ctx);
  }
  /* LITE_KERNEL_SECTION END images_Transpose */
  /* LITE_KERNEL_SECTION BEGIN _model_0_conv_Conv_output_0 */
  {
      const ai_float* _model_0_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 1015296);
    ai_float* _model_0_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 802816);
    const ai_u8* _model_0_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 8);
    const ai_u8* _model_0_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1736);
    ai_float* _model_0_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 413184);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) _model_0_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_0_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_0_conv_Conv_output_0_t_out_0_ptr_f32, _model_0_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_0_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_0_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_0_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_0_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_0_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_0_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_0_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_0_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_0_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_0_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_0_conv_Conv_output_0_l_pad_W_0_const_s32, _model_0_conv_Conv_output_0_l_pad_H_0_const_s32, _model_0_conv_Conv_output_0_l_stride_1_const_u16, _model_0_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_0_conv_Conv_output_0_l_dilation_H_const_u16, _model_0_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) _model_0_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_0_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_0_act_Sigmoid_output_0 */
  {
      ai_handle _model_0_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 0);
    const ai_handle _model_0_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 802816);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) _model_0_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_0_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_0_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_0_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) _model_0_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_0_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_0_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_0_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_0_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_1_conv_Conv_output_0 */
  {
      const ai_float* _model_1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 802816);
    ai_float* _model_1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 576);
    const ai_u8* _model_1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1800);
    const ai_u8* _model_1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 20232);
    ai_float* _model_1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) _model_1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_1_conv_Conv_output_0_t_out_0_ptr_f32, _model_1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_1_conv_Conv_output_0_l_stride_1_const_u16, _model_1_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_1_conv_Conv_output_0_l_dilation_H_const_u16, _model_1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) _model_1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_1_act_Sigmoid_output_0 */
  {
      ai_handle _model_1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 401984);
    const ai_handle _model_1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 576);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) _model_1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) _model_1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_2_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 803392);
    ai_float* _model_2_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 128);
    const ai_u8* _model_2_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 20360);
    const ai_u8* _model_2_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 24456);
    ai_float* _model_2_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) _model_2_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_2_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_2_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_2_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_2_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_2_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_2_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_2_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_2_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_2_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_2_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_2_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_2_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_2_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_2_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_2_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_2_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_2_cv1_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_2_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_2_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) _model_2_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_2_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_2_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 401536);
    const ai_handle _model_2_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 128);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) _model_2_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_2_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_2_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_2_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) _model_2_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_2_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_2_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_2_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_Split_output_0 */
  {
    
  forward_lite_split__model_2_Split_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_2_Split_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_m_0_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_2_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 200704);
    ai_float* _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 401984);
    const ai_u8* _model_2_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 24588);
    const ai_u8* _model_2_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 29196);
    ai_float* _model_2_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 401408);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) _model_2_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_2_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_2_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_2_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_2_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_2_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_2_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_2_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_2_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_2_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_2_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_2_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_2_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_2_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_2_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_2_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) _model_2_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_2_m_0_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_m_0_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_2_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 502336);
    const ai_handle _model_2_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 401984);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) _model_2_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_2_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_2_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_2_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) _model_2_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_2_m_0_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_m_0_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_2_m_0_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_2_m_0_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_m_0_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_2_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 602688);
    ai_float* _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 401696);
    const ai_u8* _model_2_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 29228);
    const ai_u8* _model_2_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 33836);
    ai_float* _model_2_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 401408);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) _model_2_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_2_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_2_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_2_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_2_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_2_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_2_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_2_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_2_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_2_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_2_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_2_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_2_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_2_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_2_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_2_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) _model_2_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_2_m_0_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_m_0_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_2_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 602400);
    const ai_handle _model_2_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 401696);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(16, 1, {(stai_ptr) _model_2_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_2_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_2_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_2_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(16, 1, {(stai_ptr) _model_2_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_2_m_0_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_m_0_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_2_m_0_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_2_m_0_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_m_0_Add_output_0 */
  {
    
  forward_lite_eltwise__model_2_m_0_Add_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_2_m_0_Add_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_Concat_output_0 */
  {
    
  forward_lite_concat__model_2_Concat_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_2_Concat_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_2_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 602112);
    ai_float* _model_2_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 1204224);
    const ai_u8* _model_2_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 33900);
    const ai_u8* _model_2_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 46188);
    ai_float* _model_2_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 401408);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(20, 1, {(stai_ptr) _model_2_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_2_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_2_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_2_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_2_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_2_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_2_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_2_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_2_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_2_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_2_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_2_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_2_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_2_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_2_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_2_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_2_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_2_cv2_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_2_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_2_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(20, 1, {(stai_ptr) _model_2_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_2_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_2_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 401408);
    const ai_handle _model_2_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 1204224);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, {(stai_ptr) _model_2_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_2_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_2_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_2_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, {(stai_ptr) _model_2_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_2_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_2_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_2_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_2_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_3_conv_Conv_output_0 */
  {
      const ai_float* _model_3_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 1204224);
    ai_float* _model_3_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 403712);
    const ai_u8* _model_3_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 46444);
    const ai_u8* _model_3_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 193900);
    ai_float* _model_3_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 401408);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(23, 1, {(stai_ptr) _model_3_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_3_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_3_conv_Conv_output_0_t_out_0_ptr_f32, _model_3_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_3_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_3_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_3_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_3_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_3_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_3_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_3_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_3_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_3_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_3_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_3_conv_Conv_output_0_l_pad_W_0_const_s32, _model_3_conv_Conv_output_0_l_pad_H_0_const_s32, _model_3_conv_Conv_output_0_l_stride_1_const_u16, _model_3_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_3_conv_Conv_output_0_l_dilation_H_const_u16, _model_3_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(23, 1, {(stai_ptr) _model_3_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_3_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_3_act_Sigmoid_output_0 */
  {
      ai_handle _model_3_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 604416);
    const ai_handle _model_3_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 403712);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(24, 1, {(stai_ptr) _model_3_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_3_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_3_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_3_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(24, 1, {(stai_ptr) _model_3_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_3_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_3_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_3_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_3_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_4_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 805120);
    ai_float* _model_4_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 401664);
    const ai_u8* _model_4_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 194156);
    const ai_u8* _model_4_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 210540);
    ai_float* _model_4_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 401408);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(26, 1, {(stai_ptr) _model_4_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_4_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_4_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_4_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_4_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_4_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_4_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_4_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_4_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_4_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_4_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_4_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_4_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_4_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_4_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_4_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_4_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_4_cv1_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_4_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_4_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(26, 1, {(stai_ptr) _model_4_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_4_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_4_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 602368);
    const ai_handle _model_4_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 401664);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(27, 1, {(stai_ptr) _model_4_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_4_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_4_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_4_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(27, 1, {(stai_ptr) _model_4_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_4_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_4_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_4_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_Split_output_0 */
  {
    
  forward_lite_split__model_4_Split_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_4_Split_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_m_0_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_4_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 501760);
    ai_float* _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 603264);
    const ai_u8* _model_4_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 210800);
    const ai_u8* _model_4_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 229232);
    ai_float* _model_4_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 602112);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(31, 1, {(stai_ptr) _model_4_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_4_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_4_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_4_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_4_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_4_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_4_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_4_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_4_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_4_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_4_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_4_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_4_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_4_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_4_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_4_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(31, 1, {(stai_ptr) _model_4_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_4_m_0_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_m_0_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_4_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 653440);
    const ai_handle _model_4_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 603264);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(32, 1, {(stai_ptr) _model_4_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_4_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_4_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_4_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(32, 1, {(stai_ptr) _model_4_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_4_m_0_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_m_0_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_4_m_0_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_4_m_0_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_m_0_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_4_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 703616);
    ai_float* _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 602688);
    const ai_u8* _model_4_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 229296);
    const ai_u8* _model_4_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 247728);
    ai_float* _model_4_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 602112);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(34, 1, {(stai_ptr) _model_4_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_4_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_4_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_4_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_4_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_4_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_4_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_4_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_4_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_4_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_4_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_4_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_4_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_4_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_4_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_4_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(34, 1, {(stai_ptr) _model_4_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_4_m_0_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_m_0_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_4_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 703040);
    const ai_handle _model_4_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 602688);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(35, 1, {(stai_ptr) _model_4_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_4_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_4_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_4_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(35, 1, {(stai_ptr) _model_4_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_4_m_0_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_m_0_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_4_m_0_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_4_m_0_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_m_0_Add_output_0 */
  {
    
  forward_lite_eltwise__model_4_m_0_Add_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_4_m_0_Add_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_Concat_output_0 */
  {
    
  forward_lite_concat__model_4_Concat_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_4_Concat_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_4_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
    ai_float* _model_4_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 1003520);
    const ai_u8* _model_4_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 247856);
    const ai_u8* _model_4_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 297008);
    ai_float* _model_4_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 602112);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(39, 1, {(stai_ptr) _model_4_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_4_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_4_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_4_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_4_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_4_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_4_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_4_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_4_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_4_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_4_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_4_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_4_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_4_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_4_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_4_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_4_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_4_cv2_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_4_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_4_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(39, 1, {(stai_ptr) _model_4_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_4_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_4_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 602112);
    const ai_handle _model_4_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 1003520);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(40, 1, {(stai_ptr) _model_4_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_4_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_4_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_4_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(40, 1, {(stai_ptr) _model_4_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_4_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_4_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_4_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_4_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_5_conv_Conv_output_0 */
  {
      const ai_float* _model_5_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 1404928);
    ai_float* _model_5_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 606720);
    const ai_u8* _model_5_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 297520);
    const ai_u8* _model_5_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 887344);
    ai_float* _model_5_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 602112);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(42, 1, {(stai_ptr) _model_5_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_5_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_5_conv_Conv_output_0_t_out_0_ptr_f32, _model_5_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_5_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_5_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_5_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_5_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_5_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_5_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_5_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_5_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_5_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_5_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_5_conv_Conv_output_0_l_pad_W_0_const_s32, _model_5_conv_Conv_output_0_l_pad_H_0_const_s32, _model_5_conv_Conv_output_0_l_stride_1_const_u16, _model_5_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_5_conv_Conv_output_0_l_dilation_H_const_u16, _model_5_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(42, 1, {(stai_ptr) _model_5_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_5_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_5_act_Sigmoid_output_0 */
  {
      ai_handle _model_5_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 707072);
    const ai_handle _model_5_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 606720);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(43, 1, {(stai_ptr) _model_5_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_5_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_5_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_5_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(43, 1, {(stai_ptr) _model_5_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_5_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_5_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_5_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_5_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_6_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 807424);
    ai_float* _model_6_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 602624);
    const ai_u8* _model_6_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 887856);
    const ai_u8* _model_6_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 953392);
    ai_float* _model_6_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 602112);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(45, 1, {(stai_ptr) _model_6_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_6_cv1_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_6_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(45, 1, {(stai_ptr) _model_6_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 702976);
    const ai_handle _model_6_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 602624);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(46, 1, {(stai_ptr) _model_6_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(46, 1, {(stai_ptr) _model_6_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_Split_output_0 */
  {
    
  forward_lite_split__model_6_Split_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_Split_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_6_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 652288);
    ai_float* _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702720);
    const ai_u8* _model_6_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 953908);
    const ai_u8* _model_6_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 962100);
    ai_float* _model_6_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(67, 1, {(stai_ptr) _model_6_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_6_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_6_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(67, 1, {(stai_ptr) _model_6_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 727808);
    const ai_handle _model_6_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 702720);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(68, 1, {(stai_ptr) _model_6_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(68, 1, {(stai_ptr) _model_6_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_6_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 652288);
    ai_float* _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702720);
    const ai_u8* _model_6_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 962228);
    const ai_u8* _model_6_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 970420);
    ai_float* _model_6_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(50, 1, {(stai_ptr) _model_6_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_6_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_6_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(50, 1, {(stai_ptr) _model_6_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 727808);
    const ai_handle _model_6_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 702720);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(51, 1, {(stai_ptr) _model_6_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(51, 1, {(stai_ptr) _model_6_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_0_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 777984);
    ai_float* _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 703616);
    const ai_u8* _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 970548);
    const ai_u8* _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1007412);
    ai_float* _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(53, 1, {(stai_ptr) _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(53, 1, {(stai_ptr) _model_6_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_0_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 803072);
    const ai_handle _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 703616);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(54, 1, {(stai_ptr) _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(54, 1, {(stai_ptr) _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_0_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_0_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_m_m_0_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_0_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_0_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 828160);
    ai_float* _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 703616);
    const ai_u8* _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1007540);
    const ai_u8* _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1044404);
    ai_float* _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(56, 1, {(stai_ptr) _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(56, 1, {(stai_ptr) _model_6_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_0_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 803072);
    const ai_handle _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 703616);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(57, 1, {(stai_ptr) _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(57, 1, {(stai_ptr) _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_0_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_0_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_m_m_0_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_0_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_0_Add_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_m_m_0_Add_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_0_Add_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_1_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
    ai_float* _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 777984);
    const ai_u8* _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1044532);
    const ai_u8* _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1081396);
    ai_float* _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 727552);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(60, 1, {(stai_ptr) _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(60, 1, {(stai_ptr) _model_6_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_1_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 727552);
    const ai_handle _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 777984);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(61, 1, {(stai_ptr) _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(61, 1, {(stai_ptr) _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_1_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_1_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_m_m_1_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_1_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_1_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 803072);
    ai_float* _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 777984);
    const ai_u8* _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1081524);
    const ai_u8* _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1118388);
    ai_float* _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 727552);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(63, 1, {(stai_ptr) _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(63, 1, {(stai_ptr) _model_6_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_1_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 727552);
    const ai_handle _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 777984);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(64, 1, {(stai_ptr) _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(64, 1, {(stai_ptr) _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_1_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_1_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_m_m_1_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_1_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_m_m_1_Add_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_m_m_1_Add_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_m_m_1_Add_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_Concat_output_0 */
  {
    
  forward_lite_concat__model_6_m_0_Concat_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_Concat_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv3_conv_Conv_output_0 */
  {
      const ai_float* _model_6_m_0_cv3_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 777984);
    ai_float* _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702720);
    const ai_u8* _model_6_m_0_cv3_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1118516);
    const ai_u8* _model_6_m_0_cv3_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1134900);
    ai_float* _model_6_m_0_cv3_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(71, 1, {(stai_ptr) _model_6_m_0_cv3_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_m_0_cv3_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_m_0_cv3_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_m_0_cv3_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_m_0_cv3_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_m_0_cv3_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_m_0_cv3_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_m_0_cv3_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_m_0_cv3_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_m_0_cv3_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_m_0_cv3_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_m_0_cv3_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_m_0_cv3_conv_Conv_output_0_l_stride_1_const_u16, _model_6_m_0_cv3_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_6_m_0_cv3_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_m_0_cv3_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(71, 1, {(stai_ptr) _model_6_m_0_cv3_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv3_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv3_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_m_0_cv3_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 752896);
    const ai_handle _model_6_m_0_cv3_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 702720);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(72, 1, {(stai_ptr) _model_6_m_0_cv3_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_m_0_cv3_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_m_0_cv3_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_m_0_cv3_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(72, 1, {(stai_ptr) _model_6_m_0_cv3_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv3_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_m_0_cv3_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_m_0_cv3_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_m_0_cv3_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_Concat_output_0 */
  {
    
  forward_lite_concat__model_6_Concat_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_Concat_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_6_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 853248);
    ai_float* _model_6_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 703232);
    const ai_u8* _model_6_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1135156);
    const ai_u8* _model_6_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1233460);
    ai_float* _model_6_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(75, 1, {(stai_ptr) _model_6_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_6_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_6_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_6_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_6_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_6_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_6_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_6_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_6_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_6_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_6_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_6_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_6_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_6_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_6_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_6_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_6_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_6_cv2_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_6_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_6_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(75, 1, {(stai_ptr) _model_6_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_6_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_6_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 803584);
    const ai_handle _model_6_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 703232);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(76, 1, {(stai_ptr) _model_6_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_6_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_6_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_6_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(76, 1, {(stai_ptr) _model_6_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_6_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_6_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_6_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_6_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_7_conv_Conv_output_0 */
  {
      const ai_float* _model_7_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 903936);
    ai_float* _model_7_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 707072);
    const ai_u8* _model_7_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 1233972);
    const ai_u8* _model_7_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2413620);
    ai_float* _model_7_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(78, 1, {(stai_ptr) _model_7_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_7_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_7_conv_Conv_output_0_t_out_0_ptr_f32, _model_7_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_7_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_7_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_7_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_7_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_7_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_7_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_7_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_7_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_7_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_7_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_7_conv_Conv_output_0_l_pad_W_0_const_s32, _model_7_conv_Conv_output_0_l_pad_H_0_const_s32, _model_7_conv_Conv_output_0_l_stride_1_const_u16, _model_7_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_7_conv_Conv_output_0_l_dilation_H_const_u16, _model_7_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(78, 1, {(stai_ptr) _model_7_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_7_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_7_act_Sigmoid_output_0 */
  {
      ai_handle _model_7_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 757248);
    const ai_handle _model_7_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 707072);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(79, 1, {(stai_ptr) _model_7_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_7_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_7_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_7_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(79, 1, {(stai_ptr) _model_7_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_7_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_7_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_7_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_7_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_8_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 807424);
    ai_float* _model_8_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 703488);
    const ai_u8* _model_8_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2414644);
    const ai_u8* _model_8_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2676788);
    ai_float* _model_8_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 702464);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(81, 1, {(stai_ptr) _model_8_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_8_cv1_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_8_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(81, 1, {(stai_ptr) _model_8_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 753664);
    const ai_handle _model_8_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 703488);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(82, 1, {(stai_ptr) _model_8_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(82, 1, {(stai_ptr) _model_8_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_Split_output_0 */
  {
    
  forward_lite_split__model_8_Split_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_Split_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_8_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 727552);
    ai_float* _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 753152);
    const ai_u8* _model_8_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2677816);
    const ai_u8* _model_8_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2710584);
    ai_float* _model_8_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(103, 1, {(stai_ptr) _model_8_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_8_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_8_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(103, 1, {(stai_ptr) _model_8_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 765696);
    const ai_handle _model_8_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 753152);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(104, 1, {(stai_ptr) _model_8_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(104, 1, {(stai_ptr) _model_8_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_8_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 727552);
    ai_float* _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 753152);
    const ai_u8* _model_8_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2710840);
    const ai_u8* _model_8_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2743608);
    ai_float* _model_8_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(86, 1, {(stai_ptr) _model_8_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_8_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_8_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(86, 1, {(stai_ptr) _model_8_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 765696);
    const ai_handle _model_8_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 753152);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(87, 1, {(stai_ptr) _model_8_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(87, 1, {(stai_ptr) _model_8_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_0_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 790784);
    ai_float* _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 754944);
    const ai_u8* _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2743864);
    const ai_u8* _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2891320);
    ai_float* _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(89, 1, {(stai_ptr) _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(89, 1, {(stai_ptr) _model_8_m_0_m_m_0_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_0_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 803328);
    const ai_handle _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 754944);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(90, 1, {(stai_ptr) _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(90, 1, {(stai_ptr) _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_0_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_0_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_m_m_0_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_0_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_0_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 815872);
    ai_float* _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 754944);
    const ai_u8* _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 2891576);
    const ai_u8* _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3039032);
    ai_float* _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(92, 1, {(stai_ptr) _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(92, 1, {(stai_ptr) _model_8_m_0_m_m_0_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_0_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 803328);
    const ai_handle _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 754944);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(93, 1, {(stai_ptr) _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(93, 1, {(stai_ptr) _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_0_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_0_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_m_m_0_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_0_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_0_Add_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_m_m_0_Add_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_0_Add_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_1_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
    ai_float* _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 790784);
    const ai_u8* _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3039288);
    const ai_u8* _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3186744);
    ai_float* _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 765184);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(96, 1, {(stai_ptr) _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(96, 1, {(stai_ptr) _model_8_m_0_m_m_1_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_1_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 765184);
    const ai_handle _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 790784);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(97, 1, {(stai_ptr) _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(97, 1, {(stai_ptr) _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_1_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_1_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_m_m_1_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_1_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_1_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 803328);
    ai_float* _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 790784);
    const ai_u8* _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3187000);
    const ai_u8* _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3334456);
    ai_float* _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 765184);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(99, 1, {(stai_ptr) _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(99, 1, {(stai_ptr) _model_8_m_0_m_m_1_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_1_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 765184);
    const ai_handle _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 790784);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(100, 1, {(stai_ptr) _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(100, 1, {(stai_ptr) _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_1_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_1_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_m_m_1_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_1_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_m_m_1_Add_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_m_m_1_Add_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_m_m_1_Add_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_Concat_output_0 */
  {
    
  forward_lite_concat__model_8_m_0_Concat_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_Concat_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv3_conv_Conv_output_0 */
  {
      const ai_float* _model_8_m_0_cv3_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 790784);
    ai_float* _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 753152);
    const ai_u8* _model_8_m_0_cv3_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3334712);
    const ai_u8* _model_8_m_0_cv3_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3400248);
    ai_float* _model_8_m_0_cv3_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(107, 1, {(stai_ptr) _model_8_m_0_cv3_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_m_0_cv3_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_m_0_cv3_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_m_0_cv3_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_m_0_cv3_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_m_0_cv3_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_m_0_cv3_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_m_0_cv3_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_m_0_cv3_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_m_0_cv3_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_m_0_cv3_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_m_0_cv3_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_m_0_cv3_conv_Conv_output_0_l_stride_1_const_u16, _model_8_m_0_cv3_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_8_m_0_cv3_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_m_0_cv3_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(107, 1, {(stai_ptr) _model_8_m_0_cv3_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv3_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv3_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_m_0_cv3_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 778240);
    const ai_handle _model_8_m_0_cv3_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 753152);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(108, 1, {(stai_ptr) _model_8_m_0_cv3_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_m_0_cv3_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_m_0_cv3_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_m_0_cv3_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(108, 1, {(stai_ptr) _model_8_m_0_cv3_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv3_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_m_0_cv3_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_m_0_cv3_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_m_0_cv3_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_Concat_output_0 */
  {
    
  forward_lite_concat__model_8_Concat_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_Concat_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_8_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 828416);
    ai_float* _model_8_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 754176);
    const ai_u8* _model_8_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3400760);
    const ai_u8* _model_8_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3793976);
    ai_float* _model_8_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(111, 1, {(stai_ptr) _model_8_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_8_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_8_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_8_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_8_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_8_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_8_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_8_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_8_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_8_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_8_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_8_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_8_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_8_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_8_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_8_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_8_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_8_cv2_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_8_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_8_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(111, 1, {(stai_ptr) _model_8_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_8_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_8_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 804352);
    const ai_handle _model_8_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 754176);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(112, 1, {(stai_ptr) _model_8_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_8_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_8_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_8_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(112, 1, {(stai_ptr) _model_8_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_8_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_8_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_8_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_8_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_cv1_conv_Conv_output_0 */
  {
      const ai_float* _model_9_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 854528);
    ai_float* _model_9_cv1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 753664);
    const ai_u8* _model_9_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 3795000);
    const ai_u8* _model_9_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4057144);
    ai_float* _model_9_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(114, 1, {(stai_ptr) _model_9_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_9_cv1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_9_cv1_conv_Conv_output_0_t_out_0_ptr_f32, _model_9_cv1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_9_cv1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_9_cv1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_9_cv1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_9_cv1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_9_cv1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_9_cv1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_9_cv1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_9_cv1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_9_cv1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_9_cv1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_9_cv1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_9_cv1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_9_cv1_conv_Conv_output_0_l_stride_1_const_u16, _model_9_cv1_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_9_cv1_conv_Conv_output_0_l_dilation_H_const_u16, _model_9_cv1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(114, 1, {(stai_ptr) _model_9_cv1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_9_cv1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_cv1_act_Sigmoid_output_0 */
  {
      ai_handle _model_9_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 803840);
    const ai_handle _model_9_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 753664);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(115, 1, {(stai_ptr) _model_9_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_9_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_9_cv1_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_9_cv1_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(115, 1, {(stai_ptr) _model_9_cv1_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_9_cv1_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_cv1_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_9_cv1_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_cv1_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_Split_output_0 */
  {
    
  forward_lite_split__model_9_Split_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_Split_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_qkv_conv_Conv_output_0 */
  {
      const ai_float* _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 777728);
    ai_float* _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 803328);
    const ai_u8* _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4058172);
    const ai_u8* _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4189244);
    ai_float* _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 802816);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(118, 1, {(stai_ptr) _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_ptr_f32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_pad_W_0_const_s32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_pad_H_0_const_s32, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_stride_1_const_u16, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_dilation_H_const_u16, _model_9_m_m_0_attn_qkv_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(118, 1, {(stai_ptr) _model_9_m_m_0_attn_qkv_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_qkv_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Reshape_output_0_to_chlast */
  {
    
  forward_lite_transpose__model_9_m_m_0_attn_Reshape_output_0_to_chlast(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Reshape_output_0_to_chlast */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Reshape_output_0_to_chfirst */
  {
    
  forward_lite_transpose__model_9_m_m_0_attn_Reshape_output_0_to_chfirst(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Reshape_output_0_to_chfirst */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Split_output_0 */
  {
    
  forward_lite_split__model_9_m_m_0_attn_Split_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Split_output_0 */
  /* LITE_KERNEL_SECTION BEGIN transpose_a_model_9_m_m_0_attn_MatMul_output_0_out */
  {
    
  forward_lite_transpose_transpose_a_model_9_m_m_0_attn_MatMul_output_0_out(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_a_model_9_m_m_0_attn_MatMul_output_0_out */
  /* LITE_KERNEL_SECTION BEGIN transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out */
  {
    
  forward_lite_transpose_transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_a_model_9_m_m_0_attn_MatMul_1_output_0_out */
  /* LITE_KERNEL_SECTION BEGIN transpose_b_model_9_m_m_0_attn_MatMul_output_0_out */
  {
    
  forward_lite_transpose_transpose_b_model_9_m_m_0_attn_MatMul_output_0_out(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_b_model_9_m_m_0_attn_MatMul_output_0_out */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_MatMul_output_0 */
  {
    
  forward_lite_matmul__model_9_m_m_0_attn_MatMul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_MatMul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN transpose_out_model_9_m_m_0_attn_MatMul_output_0_out */
  {
    
  forward_lite_transpose_transpose_out_model_9_m_m_0_attn_MatMul_output_0_out(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_out_model_9_m_m_0_attn_MatMul_output_0_out */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Mul_output_0 */
  {
      ai_float* _model_9_m_m_0_attn_Mul_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 840448);
    const ai_float* _model_9_m_m_0_attn_Mul_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 903168);
    const ai_float* _model_9_m_m_0_attn_Mul_output_0_t_weight_0_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 4190280);
    const ai_float* _model_9_m_m_0_attn_Mul_output_0_t_weight_1_ptr_const_f32 = (ai_float*)(net_ctx->_weights[0] + 4190288);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(126, 1, {(stai_ptr) _model_9_m_m_0_attn_Mul_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_bn_if32of32wf32(_model_9_m_m_0_attn_Mul_output_0_t_out_0_ptr_f32, _model_9_m_m_0_attn_Mul_output_0_t_in_0_ptr_const_f32, _model_9_m_m_0_attn_Mul_output_0_t_weight_0_ptr_const_f32, _model_9_m_m_0_attn_Mul_output_0_t_weight_1_ptr_const_f32, (ai_u32)(4802), (ai_size)(2));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(126, 1, {(stai_ptr) _model_9_m_m_0_attn_Mul_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Softmax_output_0 */
  {
      ai_handle _model_9_m_m_0_attn_Softmax_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 903168);
    const ai_handle _model_9_m_m_0_attn_Softmax_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 840448);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(127, 1, {(stai_ptr) _model_9_m_m_0_attn_Softmax_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_softmax_if32of32(_model_9_m_m_0_attn_Softmax_output_0_t_out_0_ptr_handle, _model_9_m_m_0_attn_Softmax_output_0_t_in_0_ptr_const_handle, _model_9_m_m_0_attn_Softmax_output_0_t_in_0_shape_ch_h_w_prod_const_s32, 2, 49);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(127, 1, {(stai_ptr) _model_9_m_m_0_attn_Softmax_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Softmax_output_0 */
  /* LITE_KERNEL_SECTION BEGIN transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out */
  {
    
  forward_lite_transpose_transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out(net_ctx);
  }
  /* LITE_KERNEL_SECTION END transpose_b_model_9_m_m_0_attn_MatMul_1_output_0_out */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_MatMul_1_output_0 */
  {
    
  forward_lite_matmul__model_9_m_m_0_attn_MatMul_1_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_MatMul_1_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst */
  {
    
  forward_lite_transpose__model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Reshape_1_output_0_to_chfirst */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast */
  {
    
  forward_lite_transpose__model_9_m_m_0_attn_Reshape_2_output_0_to_chlast(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Reshape_2_output_0_to_chlast */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst */
  {
    
  forward_lite_transpose__model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Reshape_2_output_0_to_chfirst */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_pe_conv_Conv_output_0 */
  {
      const ai_float* _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 852992);
    ai_float* _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 827904);
    const ai_u8* _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4190296);
    const ai_u8* _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4194904);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(134, 1, {(stai_ptr) _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_dw_if32of32wf32(_model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_ptr_f32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_pad_W_0_const_s32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_pad_H_0_const_s32, _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_stride_1_const_u16, _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_stride_0_const_u16, 3, 3, _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_dilation_H_const_u16, _model_9_m_m_0_attn_pe_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(128));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(134, 1, {(stai_ptr) _model_9_m_m_0_attn_pe_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_pe_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_Add_output_0 */
  {
    
  forward_lite_eltwise__model_9_m_m_0_attn_Add_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_Add_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_attn_proj_conv_Conv_output_0 */
  {
      const ai_float* _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 852992);
    ai_float* _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 803328);
    const ai_u8* _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4195416);
    const ai_u8* _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4260952);
    ai_float* _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 802816);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(136, 1, {(stai_ptr) _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_ptr_f32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_pad_W_0_const_s32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_pad_H_0_const_s32, _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_stride_1_const_u16, _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_dilation_H_const_u16, _model_9_m_m_0_attn_proj_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(136, 1, {(stai_ptr) _model_9_m_m_0_attn_proj_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_attn_proj_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_Add_output_0 */
  {
    
  forward_lite_eltwise__model_9_m_m_0_Add_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_Add_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0 */
  {
      const ai_float* _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 828416);
    ai_float* _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 778240);
    const ai_u8* _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4261464);
    const ai_u8* _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4392536);
    ai_float* _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 777728);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(138, 1, {(stai_ptr) _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_ptr_f32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_pad_W_0_const_s32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_pad_H_0_const_s32, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_stride_1_const_u16, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_dilation_H_const_u16, _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(138, 1, {(stai_ptr) _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_ffn_ffn_0_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0 */
  {
      ai_handle _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 853504);
    const ai_handle _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 778240);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(139, 1, {(stai_ptr) _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(139, 1, {(stai_ptr) _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_ffn_ffn_0_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_9_m_m_0_ffn_ffn_0_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_ffn_ffn_0_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0 */
  {
      const ai_float* _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 903680);
    ai_float* _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 778752);
    const ai_u8* _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4393560);
    const ai_u8* _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4524632);
    ai_float* _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 777728);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(141, 1, {(stai_ptr) _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_ptr_f32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_pad_W_0_const_s32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_pad_H_0_const_s32, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_stride_1_const_u16, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_dilation_H_const_u16, _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(141, 1, {(stai_ptr) _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_ffn_ffn_1_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_m_m_0_Add_1_output_0 */
  {
    
  forward_lite_eltwise__model_9_m_m_0_Add_1_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_m_m_0_Add_1_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_Concat_output_0 */
  {
    
  forward_lite_concat__model_9_Concat_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_Concat_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_cv2_conv_Conv_output_0 */
  {
      const ai_float* _model_9_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 777728);
    ai_float* _model_9_cv2_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 827904);
    const ai_u8* _model_9_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4525144);
    const ai_u8* _model_9_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4787288);
    ai_float* _model_9_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(144, 1, {(stai_ptr) _model_9_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_9_cv2_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_9_cv2_conv_Conv_output_0_t_out_0_ptr_f32, _model_9_cv2_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_9_cv2_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_9_cv2_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_9_cv2_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_9_cv2_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_9_cv2_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_9_cv2_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_9_cv2_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_9_cv2_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_9_cv2_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_9_cv2_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_9_cv2_conv_Conv_output_0_l_pad_W_0_const_s32, _model_9_cv2_conv_Conv_output_0_l_pad_H_0_const_s32, _model_9_cv2_conv_Conv_output_0_l_stride_1_const_u16, _model_9_cv2_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_9_cv2_conv_Conv_output_0_l_dilation_H_const_u16, _model_9_cv2_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(144, 1, {(stai_ptr) _model_9_cv2_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_9_cv2_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_cv2_act_Sigmoid_output_0 */
  {
      ai_handle _model_9_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 752640);
    const ai_handle _model_9_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 827904);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(145, 1, {(stai_ptr) _model_9_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_9_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_9_cv2_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_9_cv2_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(145, 1, {(stai_ptr) _model_9_cv2_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_9_cv2_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_9_cv2_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_9_cv2_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_9_cv2_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_10_conv_conv_Conv_output_0 */
  {
      const ai_float* _model_10_conv_conv_Conv_output_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_activations[0] + 878080);
    ai_float* _model_10_conv_conv_Conv_output_0_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 928256);
    const ai_u8* _model_10_conv_conv_Conv_output_0_t_weight_0_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 4788312);
    const ai_u8* _model_10_conv_conv_Conv_output_0_t_weight_1_ptr_const_u8 = (ai_u8*)(net_ctx->_weights[0] + 6099032);
    ai_float* _model_10_conv_conv_Conv_output_0_t_scratch_0_ptr_f32 = (ai_float*)(net_ctx->_activations[0] + 752640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(147, 1, {(stai_ptr) _model_10_conv_conv_Conv_output_0_t_in_0_ptr_const_f32});
    
  forward_lite_conv2d_if32of32wf32(_model_10_conv_conv_Conv_output_0_t_in_0_ptr_const_f32, _model_10_conv_conv_Conv_output_0_t_out_0_ptr_f32, _model_10_conv_conv_Conv_output_0_t_weight_0_ptr_const_u8, _model_10_conv_conv_Conv_output_0_t_weight_1_ptr_const_u8, _model_10_conv_conv_Conv_output_0_t_scratch_0_ptr_f32, _model_10_conv_conv_Conv_output_0_t_in_0_shape_ch_const_u32, _model_10_conv_conv_Conv_output_0_t_out_0_shape_ch_const_u32, _model_10_conv_conv_Conv_output_0_t_in_0_shape_w_const_u32, _model_10_conv_conv_Conv_output_0_t_in_0_shape_h_const_u32, _model_10_conv_conv_Conv_output_0_t_out_0_shape_w_const_u32, _model_10_conv_conv_Conv_output_0_t_out_0_shape_h_const_u32, _model_10_conv_conv_Conv_output_0_t_weight_0_shape_w_const_u32, _model_10_conv_conv_Conv_output_0_t_weight_0_shape_h_const_u32, _model_10_conv_conv_Conv_output_0_l_pad_W_0_const_s32, _model_10_conv_conv_Conv_output_0_l_pad_H_0_const_s32, _model_10_conv_conv_Conv_output_0_l_stride_1_const_u16, _model_10_conv_conv_Conv_output_0_l_stride_0_const_u16, 1, 1, _model_10_conv_conv_Conv_output_0_l_dilation_H_const_u16, _model_10_conv_conv_Conv_output_0_l_dilation_W_const_u16, (ai_size)(1));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(147, 1, {(stai_ptr) _model_10_conv_conv_Conv_output_0_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END _model_10_conv_conv_Conv_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_10_conv_act_Sigmoid_output_0 */
  {
      ai_handle _model_10_conv_act_Sigmoid_output_0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_activations[0] + 1179136);
    const ai_handle _model_10_conv_act_Sigmoid_output_0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 928256);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(148, 1, {(stai_ptr) _model_10_conv_act_Sigmoid_output_0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_sigmoid_if32of32(_model_10_conv_act_Sigmoid_output_0_t_out_0_ptr_handle, _model_10_conv_act_Sigmoid_output_0_t_in_0_ptr_const_handle, _model_10_conv_act_Sigmoid_output_0_t_in_0_shape_ch_h_w_prod_const_s32, NULL);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(148, 1, {(stai_ptr) _model_10_conv_act_Sigmoid_output_0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END _model_10_conv_act_Sigmoid_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_10_conv_act_Mul_output_0 */
  {
    
  forward_lite_eltwise__model_10_conv_act_Mul_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_10_conv_act_Mul_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_10_pool_GlobalAveragePool_output_0 */
  {
    
  forward_lite_ap__model_10_pool_GlobalAveragePool_output_0(net_ctx);
  }
  /* LITE_KERNEL_SECTION END _model_10_pool_GlobalAveragePool_output_0 */
  /* LITE_KERNEL_SECTION BEGIN _model_10_linear_Gemm_output_0 */
  {
      forward_lite_dense_if32of32wf32_args arg_30f51e = {
      .output = (float*)(net_ctx->_activations[0] + 757760),
      .input = (float*)(net_ctx->_activations[0] + 752640),
      .weights = (float*)(net_ctx->_weights[0] + 6104152),
      .bias = (float*)(net_ctx->_weights[0] + 6145112),
      .n_channel_in = 1280,
      .n_channel_out = 8,
      .n_elements = 1,
    };
  
  _STAI_NETWORK_EVENT_NODE_START_CB(152, 1, {(stai_ptr) (float*)(net_ctx->_activations[0] + 752640)});
    
  forward_lite_dense_if32of32wf32((forward_lite_dense_if32of32wf32_args*)&arg_30f51e);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(152, 1, {(stai_ptr) (float*)(net_ctx->_activations[0] + 757760)});
  }
  /* LITE_KERNEL_SECTION END _model_10_linear_Gemm_output_0 */
  /* LITE_KERNEL_SECTION BEGIN output0 */
  {
      ai_handle output0_t_out_0_ptr_handle = (ai_handle)(net_ctx->_outputs[0] + 0);
    const ai_handle output0_t_in_0_ptr_const_handle = (ai_handle)(net_ctx->_activations[0] + 757760);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(153, 1, {(stai_ptr) output0_t_in_0_ptr_const_handle});
    
  forward_lite_nl_softmax_if32of32(output0_t_out_0_ptr_handle, output0_t_in_0_ptr_const_handle, output0_t_in_0_shape_ch_prod_const_s32, 1, 8);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(153, 1, {(stai_ptr) output0_t_out_0_ptr_handle});
  }
  /* LITE_KERNEL_SECTION END output0 */
  return net_ctx->_return_code;
}

/*****************************************************************************/
/*  Getters APIs Section  */
STAI_API_ENTRY
stai_size stai_network_get_context_size()
{
  return (stai_size)STAI_NETWORK_CONTEXT_SIZE;
}

#if defined(HAVE_NETWORK_INFO)
STAI_API_ENTRY
stai_return_code stai_network_get_info(
  stai_network* network,
  stai_network_info* info)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, info==NULL, STAI_ERROR_NETWORK_INVALID_INFO, net_ctx->_return_code)

  // Copy of network info struct
  *info = g_network_info;

  return STAI_SUCCESS;
}
#endif


STAI_API_ENTRY
stai_return_code stai_network_get_activations(
  stai_network* network, stai_ptr* activations, stai_size* n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, !n_activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_activations = STAI_NETWORK_ACTIVATIONS_NUM;
for (stai_size idx=0; activations && (idx<STAI_NETWORK_ACTIVATIONS_NUM); idx++) {
    // get address of the activations buffers
    activations[idx] = net_ctx->_activations[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_weights(
  stai_network* network, stai_ptr* weights, stai_size* n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_weights = STAI_NETWORK_WEIGHTS_NUM;
for (stai_size idx=0; weights && (idx<STAI_NETWORK_WEIGHTS_NUM); idx++) {
    // get address of the weights buffers
    weights[idx] = net_ctx->_weights[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_inputs(
  stai_network* network, stai_ptr* inputs, stai_size* n_inputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_inputs = STAI_NETWORK_IN_NUM;
  for (stai_size idx=0; inputs && (idx<STAI_NETWORK_IN_NUM); idx++) {
    inputs[idx] = net_ctx->_inputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_outputs(
  stai_network* network, stai_ptr* outputs, stai_size* n_outputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_outputs = STAI_NETWORK_OUT_NUM;
  for (stai_size idx=0; outputs && (idx<STAI_NETWORK_OUT_NUM); idx++) {
    outputs[idx] = net_ctx->_outputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_error(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /* return 1st generated error or STAI_SUCCESS if no errors so far */
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_states(
  stai_network* network, stai_ptr* states, stai_size* n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_states, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  /* get the number of internals states (supporting multi-heap also for internal states) */
  *n_states = STAI_NETWORK_STATES_NUM;

  STAI_UNUSED(states)
return net_ctx->_return_code;
}


/*****************************************************************************/
/*  Setters APIs Section  */

STAI_API_ENTRY
stai_return_code stai_network_set_activations(
  stai_network* network,
  const stai_ptr* activations,
  const stai_size n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _activations_alignment[] = STAI_NETWORK_ACTIVATIONS_ALIGNMENTS;
  STAI_PRINT("  [stai_network_set_activations] network(%p) activations[%d]: %p\n\n", net_ctx, n_activations, activations)
  _STAI_SET_ERROR(net_ctx, !activations,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_activations!=STAI_NETWORK_ACTIVATIONS_NUM,
                  STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_NUM, net_ctx->_return_code)

  for (stai_size idx=0; activations && idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    STAI_PRINT("  activation[%d]: %p\n", idx, activations[idx])
    _STAI_SET_ERROR(net_ctx, activations[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)activations[idx]) & (_activations_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_activations[idx] = activations[idx];
  }
  net_ctx->_inputs[0] = activations[0] + 413184;

  net_ctx->_outputs[0] = activations[0] + 752640;
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_weights(
  stai_network* network,
  const stai_ptr* weights,
  const stai_size n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _weights_alignment[] = STAI_NETWORK_WEIGHTS_ALIGNMENTS;
  _STAI_SET_ERROR(net_ctx, !weights,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_weights!=STAI_NETWORK_WEIGHTS_NUM,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_NUM, net_ctx->_return_code)
  for (stai_size idx=0; weights && idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    STAI_PRINT("  weight[%d]: %p\n", idx, weights[idx])
    _STAI_SET_ERROR(net_ctx, weights[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)weights[idx]) & (_weights_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_weights[idx] = weights[idx];
  }_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_inputs(
  stai_network* network,
  const stai_ptr* inputs,
  const stai_size n_inputs)
{
  const uintptr_t _inputs_alignment[] = STAI_NETWORK_IN_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !inputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_inputs!=STAI_NETWORK_IN_NUM,
                  STAI_ERROR_NETWORK_INVALID_IN_NUM, net_ctx->_return_code)

  for (stai_size idx=0; inputs && idx<STAI_NETWORK_IN_NUM; idx++) {
    STAI_PRINT("  input[%d]: %p\n", idx, inputs[idx])
    _STAI_SET_ERROR(net_ctx, inputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)inputs[idx]) & (_inputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_inputs[idx] = inputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_outputs(
  stai_network* network,
  const stai_ptr* outputs,
  const stai_size n_outputs)
{
  const uintptr_t _outputs_alignment[] = STAI_NETWORK_OUT_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !outputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_outputs!=STAI_NETWORK_OUT_NUM,
                  STAI_ERROR_NETWORK_INVALID_OUT_NUM, net_ctx->_return_code)

  for (stai_size idx=0; outputs && idx<n_outputs; idx++) {
    STAI_PRINT("  output[%d]: %p\n", idx, outputs[idx])
    _STAI_SET_ERROR(net_ctx, outputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)outputs[idx]) & (_outputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_outputs[idx] = outputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_states(
  stai_network* network,
  const stai_ptr* states,
  const stai_size n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  STAI_UNUSED(states)
  STAI_UNUSED(n_states)
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}

STAI_API_ENTRY
stai_return_code stai_network_set_callback(
  stai_network* network, const stai_event_cb cb, void* cb_cookie)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  STAI_PRINT("  set_callback %p cb %p cookie %p\n", net_ctx, cb, cb_cookie)
  // _STAI_SET_ERROR(net_ctx, cb==NULL, STAI_ERROR_NETWORK_INVALID_CALLBACK, net_ctx->_return_code)
  net_ctx->_callback = cb;
  net_ctx->_callback_cookie = cb_cookie;
  return net_ctx->_return_code;
}

#undef _STAI_SET_ERROR
#undef _STAI_CONTEXT_ALIGNMENT
#undef _STAI_CONTEXT_ACQUIRE
#undef _STAI_NETWORK_EVENT_NODE_START_CB
#undef _STAI_NETWORK_EVENT_NODE_STOP_CB
#undef _STAI_NETWORK_MODEL_SIGNATURE
#undef _STAI_NETWORK_DATETIME
#undef _STAI_NETWORK_COMPILE_DATETIME

