#pragma once

#ifndef GROW_H
#define GROW_H

typedef unsigned char       u_char;
typedef unsigned short      u_short;
typedef unsigned short      u_int16;
typedef unsigned long       u_long;
typedef short int           int16;

#define PF_TABLE_BITS   12
#define PF_TABLE_SZ_16  4096

#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"

#ifdef AE_OS_WIN
    typedef unsigned short PixelType;
    #include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AEFX_SuiteHelper.h"

#define GROW_MAJOR_VERSION    1
#define GROW_MINOR_VERSION    0
#define GROW_BUG_VERSION      0
#define GROW_BUILD_VERSION    0

#define GROW_MATCH_NAME       "YT Grow"
#define GROW_PLUGIN_NAME      "YT Grow"
#define GROW_CATEGORY         "YT Effects"
#define GROW_DESCRIPTION      "Lightweight & powerful morphological grow/shrink"

enum {
    GROW_INPUT = 0,
    GROW_RADIUS,
    GROW_SOFTNESS,
    GROW_MODE,
    GROW_SHAPE,
    GROW_CHANNEL,
    GROW_THRESHOLD,
    GROW_INVERT,
    GROW_BLEND_ORIGINAL,
    GROW_NUM_PARAMS
};

enum {
    RADIUS_DISK_ID = 1,
    SOFTNESS_DISK_ID,
    MODE_DISK_ID,
    SHAPE_DISK_ID,
    CHANNEL_DISK_ID,
    THRESHOLD_DISK_ID,
    INVERT_DISK_ID,
    BLEND_DISK_ID,
};

enum GrowMode {
    MODE_GROW = 1,
    MODE_SHRINK,
    MODE_EDGE,
    MODE_NUM
};

enum GrowShape {
    SHAPE_CIRCLE = 1,
    SHAPE_SQUARE,
    SHAPE_DIAMOND,
    SHAPE_NUM
};

enum GrowChannel {
    CHAN_ALPHA = 1,
    CHAN_LUMINANCE,
    CHAN_ALL_RGB,
    CHAN_NUM
};

#define GROW_RADIUS_MIN     0.0
#define GROW_RADIUS_MAX     500.0
#define GROW_RADIUS_DEFAULT 10.0

struct GrowParams {
    double radius;
    double softness;
    int    mode;
    int    shape;
    int    channel;
    double threshold;
    bool   invert;
    double blend;
};

#ifdef __cplusplus
extern "C" {
#endif

DllExport PF_Err PluginDataEntryFunction(
    PF_PluginDataPtr   inPtr,
    PF_PluginDataCB    inPluginDataCallBackPtr,
    SPBasicSuite      *inSPBasicSuitePtr,
    const char        *inHostName,
    const char        *inHostVersion);

DllExport PF_Err EffectMain(
    PF_Cmd       cmd,
    PF_InData   *in_data,
    PF_OutData  *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output,
    void        *extra);

#ifdef __cplusplus
}
#endif

#endif // GROW_H
