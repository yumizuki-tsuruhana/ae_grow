#include "Grow.h"
#include "GrowKernel.h"

static PF_Err About(
    PF_InData  *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    PF_SPRINTF(out_data->return_msg,
        "%s v%d.%d\r%s",
        GROW_PLUGIN_NAME,
        GROW_MAJOR_VERSION, GROW_MINOR_VERSION,
        GROW_DESCRIPTION);
    return PF_Err_NONE;
}

static PF_Err GlobalSetup(
    PF_InData  *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    out_data->my_version = PF_VERSION(
        GROW_MAJOR_VERSION,
        GROW_MINOR_VERSION,
        GROW_BUG_VERSION,
        PF_Stage_DEVELOP,
        GROW_BUILD_VERSION);

    out_data->out_flags =
        PF_OutFlag_DEEP_COLOR_AWARE |
        PF_OutFlag_PIX_INDEPENDENT  |
        PF_OutFlag_I_EXPAND_BUFFER;

    out_data->out_flags2 =
        PF_OutFlag2_SUPPORTS_SMART_RENDER |
        PF_OutFlag2_FLOAT_COLOR_AWARE     |
        PF_OutFlag2_SUPPORTS_THREADED_RENDERING;

    return PF_Err_NONE;
}

static PF_Err ParamsSetup(
    PF_InData  *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    PF_ParamDef def;
    PF_Err      err = PF_Err_NONE;

    // Radius
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(
        "Radius",
        GROW_RADIUS_MIN, GROW_RADIUS_MAX,
        GROW_RADIUS_MIN, GROW_RADIUS_MAX,
        GROW_RADIUS_DEFAULT,
        PF_Precision_TENTHS,
        0, 0,
        RADIUS_DISK_ID);

    // Softness
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(
        "Softness",
        0.0, 100.0,
        0.0, 100.0,
        50.0,
        PF_Precision_ONES,
        0, 0,
        SOFTNESS_DISK_ID);

    // Mode
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(
        "Mode",
        MODE_NUM - 1,
        MODE_GROW,
        "Grow|Shrink|Edge Only",
        MODE_DISK_ID);

    // Shape
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(
        "Shape",
        SHAPE_NUM - 1,
        SHAPE_CIRCLE,
        "Circle|Square|Diamond",
        SHAPE_DISK_ID);

    // Channel
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(
        "Channel",
        CHAN_NUM - 1,
        CHAN_ALPHA,
        "Alpha|Luminance|RGB Max",
        CHANNEL_DISK_ID);

    // Threshold
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(
        "Threshold",
        0.0, 1.0,
        0.0, 1.0,
        0.5,
        PF_Precision_HUNDREDTHS,
        0, 0,
        THRESHOLD_DISK_ID);

    // Invert
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOXX(
        "Invert",
        FALSE,
        0,
        INVERT_DISK_ID);

    // Blend with Original
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(
        "Blend Original",
        0.0, 100.0,
        0.0, 100.0,
        0.0,
        PF_Precision_ONES,
        0, 0,
        BLEND_DISK_ID);

    out_data->num_effect_params = GROW_NUM_PARAMS;

    return err;
}

static void read_params(PF_ParamDef *params[], GrowParams &gp)
{
    gp.radius    = params[GROW_RADIUS]->u.fs_d.value;
    gp.softness  = params[GROW_SOFTNESS]->u.fs_d.value;
    gp.mode      = params[GROW_MODE]->u.pd.value;
    gp.shape     = params[GROW_SHAPE]->u.pd.value;
    gp.channel   = params[GROW_CHANNEL]->u.pd.value;
    gp.threshold = params[GROW_THRESHOLD]->u.fs_d.value;
    gp.invert    = params[GROW_INVERT]->u.bd.value != 0;
    gp.blend     = params[GROW_BLEND_ORIGINAL]->u.fs_d.value;
}

static PF_Err PreRender(
    PF_InData        *in_data,
    PF_OutData       *out_data,
    PF_PreRenderExtra *extra)
{
    PF_Err err = PF_Err_NONE;

    PF_RenderRequest req = extra->input->output_request;
    PF_CheckoutResult cr;

    // Expand the input rect by radius for border pixels
    PF_ParamDef radius_param;
    AEFX_CLR_STRUCT(radius_param);
    ERR(PF_CHECKOUT_PARAM(in_data, GROW_RADIUS, in_data->current_time,
                           in_data->time_step, in_data->time_scale, &radius_param));

    A_long rad = (A_long)std::ceil(radius_param.u.fs_d.value) + 1;

    req.rect.left   -= rad;
    req.rect.top    -= rad;
    req.rect.right  += rad;
    req.rect.bottom += rad;

    req.field = PF_Field_FRAME;

    ERR(extra->cb->checkout_layer(in_data->effect_ref, GROW_INPUT,
                                   GROW_INPUT, &req,
                                   in_data->current_time,
                                   in_data->time_step,
                                   in_data->time_scale, &cr));

    if (!err) {
        extra->output->result_rect   = cr.result_rect;
        extra->output->max_result_rect = cr.max_result_rect;
    }

    PF_CHECKIN_PARAM(in_data, &radius_param);
    return err;
}

static PF_Err SmartRender(
    PF_InData           *in_data,
    PF_OutData          *out_data,
    PF_SmartRenderExtra *extra)
{
    PF_Err err = PF_Err_NONE;

    PF_EffectWorld *input_world  = nullptr;
    PF_EffectWorld *output_world = nullptr;

    ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, GROW_INPUT, &input_world));
    ERR(extra->cb->checkout_output(in_data->effect_ref, &output_world));

    if (err || !input_world || !output_world)
        return err;

    // Fetch all parameters
    PF_ParamDef p_radius, p_soft, p_mode, p_shape, p_chan, p_thresh, p_inv, p_blend;
    AEFX_CLR_STRUCT(p_radius); AEFX_CLR_STRUCT(p_soft);
    AEFX_CLR_STRUCT(p_mode);   AEFX_CLR_STRUCT(p_shape);
    AEFX_CLR_STRUCT(p_chan);    AEFX_CLR_STRUCT(p_thresh);
    AEFX_CLR_STRUCT(p_inv);    AEFX_CLR_STRUCT(p_blend);

    ERR(PF_CHECKOUT_PARAM(in_data, GROW_RADIUS,         in_data->current_time, in_data->time_step, in_data->time_scale, &p_radius));
    ERR(PF_CHECKOUT_PARAM(in_data, GROW_SOFTNESS,       in_data->current_time, in_data->time_step, in_data->time_scale, &p_soft));
    ERR(PF_CHECKOUT_PARAM(in_data, GROW_MODE,            in_data->current_time, in_data->time_step, in_data->time_scale, &p_mode));
    ERR(PF_CHECKOUT_PARAM(in_data, GROW_SHAPE,           in_data->current_time, in_data->time_step, in_data->time_scale, &p_shape));
    ERR(PF_CHECKOUT_PARAM(in_data, GROW_CHANNEL,         in_data->current_time, in_data->time_step, in_data->time_scale, &p_chan));
    ERR(PF_CHECKOUT_PARAM(in_data, GROW_THRESHOLD,       in_data->current_time, in_data->time_step, in_data->time_scale, &p_thresh));
    ERR(PF_CHECKOUT_PARAM(in_data, GROW_INVERT,          in_data->current_time, in_data->time_step, in_data->time_scale, &p_inv));
    ERR(PF_CHECKOUT_PARAM(in_data, GROW_BLEND_ORIGINAL,  in_data->current_time, in_data->time_step, in_data->time_scale, &p_blend));

    if (!err) {
        GrowParams gp;
        gp.radius    = p_radius.u.fs_d.value;
        gp.softness  = p_soft.u.fs_d.value;
        gp.mode      = p_mode.u.pd.value;
        gp.shape     = p_shape.u.pd.value;
        gp.channel   = p_chan.u.pd.value;
        gp.threshold = p_thresh.u.fs_d.value;
        gp.invert    = p_inv.u.bd.value != 0;
        gp.blend     = p_blend.u.fs_d.value;

        bool is_16bit = PF_WORLD_IS_DEEP(output_world);
        int max_val = is_16bit ? 32768 : 255;

        if (gp.radius < 0.01) {
            int w = input_world->width;
            int h = input_world->height;
            for (int y = 0; y < h; ++y) {
                std::memcpy(
                    (char *)output_world->data + y * output_world->rowbytes,
                    (char *)input_world->data  + y * input_world->rowbytes,
                    w * (is_16bit ? sizeof(PF_Pixel16) : sizeof(PF_Pixel8)));
            }
        } else {
            process_grow(input_world, output_world, gp, max_val);
        }
    }

    PF_CHECKIN_PARAM(in_data, &p_radius);
    PF_CHECKIN_PARAM(in_data, &p_soft);
    PF_CHECKIN_PARAM(in_data, &p_mode);
    PF_CHECKIN_PARAM(in_data, &p_shape);
    PF_CHECKIN_PARAM(in_data, &p_chan);
    PF_CHECKIN_PARAM(in_data, &p_thresh);
    PF_CHECKIN_PARAM(in_data, &p_inv);
    PF_CHECKIN_PARAM(in_data, &p_blend);

    return err;
}

static PF_Err Render(
    PF_InData   *in_data,
    PF_OutData  *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    PF_Err err = PF_Err_NONE;
    PF_EffectWorld *input_world = &params[GROW_INPUT]->u.ld;

    GrowParams gp;
    read_params(params, gp);

    bool is_16bit = PF_WORLD_IS_DEEP(output);
    int max_val = is_16bit ? 32768 : 255;

    if (gp.radius < 0.01) {
        ERR(PF_COPY(input_world, output, NULL, NULL));
        return err;
    }

    process_grow(input_world, output, gp, max_val);

    return err;
}

static PF_Err QueryDynamicFlags(
    PF_InData   *in_data,
    PF_OutData  *out_data,
    PF_ParamDef *params[],
    void        *extra)
{
    return PF_Err_NONE;
}

DllExport PF_Err PluginDataEntryFunction(
    PF_PluginDataPtr   inPtr,
    PF_PluginDataCB    inPluginDataCallBackPtr,
    SPBasicSuite      *inSPBasicSuitePtr,
    const char        *inHostName,
    const char        *inHostVersion)
{
    return PF_REGISTER_EFFECT(
        inPtr,
        inPluginDataCallBackPtr,
        GROW_PLUGIN_NAME,
        GROW_MATCH_NAME,
        GROW_CATEGORY,
        AE_RESERVED_INFO);
}

DllExport PF_Err EffectMain(
    PF_Cmd       cmd,
    PF_InData   *in_data,
    PF_OutData  *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output,
    void        *extra)
{
    PF_Err err = PF_Err_NONE;

    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data, out_data, params, output);
                break;
            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output);
                break;
            case PF_Cmd_SMART_PRE_RENDER:
                err = PreRender(in_data, out_data, (PF_PreRenderExtra *)extra);
                break;
            case PF_Cmd_SMART_RENDER:
                err = SmartRender(in_data, out_data, (PF_SmartRenderExtra *)extra);
                break;
            case PF_Cmd_QUERY_DYNAMIC_FLAGS:
                err = QueryDynamicFlags(in_data, out_data, params, extra);
                break;
            default:
                break;
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }

    return err;
}
