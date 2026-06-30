#include "GrowKernel.h"
#include <limits>
#include <cmath>

static constexpr float INF = 1e20f;

// 1D squared-distance transform (Felzenszwalb & Huttenlocher 2012).
// Input f[i] = 0 for foreground, INF for background.
// Output d[i] = squared Euclidean distance to nearest foreground pixel.
void dt_1d(const float *f, float *d, int n)
{
    std::vector<int>   v(n);
    std::vector<float> z(n + 1);
    int k = 0;
    v[0] = 0;
    z[0] = -INF;
    z[1] =  INF;

    for (int q = 1; q < n; ++q) {
        float s = ((f[q] + (float)(q * q)) - (f[v[k]] + (float)(v[k] * v[k])))
                  / (2.0f * q - 2.0f * v[k]);
        while (s <= z[k]) {
            --k;
            s = ((f[q] + (float)(q * q)) - (f[v[k]] + (float)(v[k] * v[k])))
                / (2.0f * q - 2.0f * v[k]);
        }
        ++k;
        v[k] = q;
        z[k] = s;
        z[k + 1] = INF;
    }

    k = 0;
    for (int q = 0; q < n; ++q) {
        while (z[k + 1] < (float)q) ++k;
        int dq = q - v[k];
        d[q] = (float)(dq * dq) + f[v[k]];
    }
}

void edt_2d(float *grid, int width, int height)
{
    std::vector<float> f(std::max(width, height));
    std::vector<float> d(std::max(width, height));

    // Transform along rows
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x)
            f[x] = grid[y * width + x];
        dt_1d(f.data(), d.data(), width);
        for (int x = 0; x < width; ++x)
            grid[y * width + x] = d[x];
    }

    // Transform along columns
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y)
            f[y] = grid[y * width + x];
        dt_1d(f.data(), d.data(), height);
        for (int y = 0; y < height; ++y)
            grid[y * width + x] = d[y];
    }
}

void dt_chebyshev(float *grid, int width, int height)
{
    // Forward pass
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid[y * width + x] > 0.0f) {
                float top  = (y > 0) ? grid[(y - 1) * width + x] : INF;
                float left = (x > 0) ? grid[y * width + (x - 1)] : INF;
                float tl   = (y > 0 && x > 0) ? grid[(y - 1) * width + (x - 1)] : INF;
                float tr   = (y > 0 && x < width - 1) ? grid[(y - 1) * width + (x + 1)] : INF;
                float m = std::min({top, left, tl, tr});
                grid[y * width + x] = std::min(grid[y * width + x], m + 1.0f);
            }
        }
    }
    // Backward pass
    for (int y = height - 1; y >= 0; --y) {
        for (int x = width - 1; x >= 0; --x) {
            if (grid[y * width + x] > 0.0f) {
                float bot   = (y < height - 1) ? grid[(y + 1) * width + x] : INF;
                float right = (x < width - 1)  ? grid[y * width + (x + 1)] : INF;
                float br    = (y < height - 1 && x < width - 1) ? grid[(y + 1) * width + (x + 1)] : INF;
                float bl    = (y < height - 1 && x > 0) ? grid[(y + 1) * width + (x - 1)] : INF;
                float m = std::min({bot, right, br, bl});
                grid[y * width + x] = std::min(grid[y * width + x], m + 1.0f);
            }
        }
    }
}

void dt_manhattan(float *grid, int width, int height)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid[y * width + x] > 0.0f) {
                float top  = (y > 0) ? grid[(y - 1) * width + x] : INF;
                float left = (x > 0) ? grid[y * width + (x - 1)] : INF;
                grid[y * width + x] = std::min(grid[y * width + x], std::min(top, left) + 1.0f);
            }
        }
    }
    for (int y = height - 1; y >= 0; --y) {
        for (int x = width - 1; x >= 0; --x) {
            if (grid[y * width + x] > 0.0f) {
                float bot   = (y < height - 1) ? grid[(y + 1) * width + x] : INF;
                float right = (x < width - 1)  ? grid[y * width + (x + 1)] : INF;
                grid[y * width + x] = std::min(grid[y * width + x], std::min(bot, right) + 1.0f);
            }
        }
    }
}

static inline float pixel_luminance_8(PF_Pixel8 px)
{
    return 0.2126f * px.red + 0.7152f * px.green + 0.0722f * px.blue;
}

static inline float pixel_luminance_16(PF_Pixel16 px)
{
    return 0.2126f * px.red + 0.7152f * px.green + 0.0722f * px.blue;
}

void build_mask(
    const PF_EffectWorld *src,
    float                *mask,
    const GrowParams     &p,
    int                   max_val)
{
    int w = src->width;
    int h = src->height;
    float thresh = (float)(p.threshold * max_val);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float val = 0.0f;

            if (max_val <= 255) {
                PF_Pixel8 *px = (PF_Pixel8 *)((char *)src->data + y * src->rowbytes) + x;
                switch (p.channel) {
                    case CHAN_ALPHA:
                        val = (float)px->alpha;
                        break;
                    case CHAN_LUMINANCE:
                        val = pixel_luminance_8(*px);
                        break;
                    case CHAN_ALL_RGB:
                        val = (float)std::max({px->red, px->green, px->blue});
                        break;
                }
            } else {
                PF_Pixel16 *px = (PF_Pixel16 *)((char *)src->data + y * src->rowbytes) + x;
                switch (p.channel) {
                    case CHAN_ALPHA:
                        val = (float)px->alpha;
                        break;
                    case CHAN_LUMINANCE:
                        val = pixel_luminance_16(*px);
                        break;
                    case CHAN_ALL_RGB:
                        val = (float)std::max({px->red, px->green, px->blue});
                        break;
                }
            }

            bool fg = val >= thresh;
            if (p.invert) fg = !fg;
            mask[y * w + x] = fg ? 0.0f : INF;
        }
    }
}

void apply_grow(
    const PF_EffectWorld *src,
    PF_EffectWorld       *dst,
    const float          *dist_sq,
    const GrowParams     &p,
    int                   max_val)
{
    int w = src->width;
    int h = src->height;
    float radius = (float)p.radius;
    float soft   = (float)(p.softness * 0.01);
    float blend  = (float)(p.blend * 0.01);

    float soft_width = radius * soft;
    if (soft_width < 0.5f) soft_width = 0.5f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float d = std::sqrt(dist_sq[y * w + x]);

            // Compute grow factor: 1.0 = fully inside, 0.0 = fully outside
            float factor;
            if (p.mode == MODE_GROW) {
                if (d <= radius - soft_width)
                    factor = 1.0f;
                else if (d >= radius)
                    factor = 0.0f;
                else
                    factor = 1.0f - (d - (radius - soft_width)) / soft_width;
            } else if (p.mode == MODE_SHRINK) {
                if (d <= 0.0f)
                    factor = 1.0f;
                else if (d >= radius)
                    factor = 0.0f;
                else
                    factor = 1.0f - d / radius;
                factor = 1.0f - factor;
            } else { // MODE_EDGE
                float edge_inner = std::max(0.0f, radius - soft_width);
                float edge_outer = radius + soft_width;
                if (d < edge_inner || d > edge_outer)
                    factor = 0.0f;
                else if (d < radius)
                    factor = (d - edge_inner) / (radius - edge_inner);
                else
                    factor = 1.0f - (d - radius) / (edge_outer - radius);
            }

            factor = std::clamp(factor, 0.0f, 1.0f);

            if (max_val <= 255) {
                PF_Pixel8 *sp = (PF_Pixel8 *)((char *)src->data + y * src->rowbytes) + x;
                PF_Pixel8 *dp = (PF_Pixel8 *)((char *)dst->data + y * dst->rowbytes) + x;

                if (p.channel == CHAN_ALPHA) {
                    dp->red   = sp->red;
                    dp->green = sp->green;
                    dp->blue  = sp->blue;
                    A_u_char grown_a = (A_u_char)std::clamp((int)(factor * max_val), 0, max_val);
                    dp->alpha = (A_u_char)std::clamp(
                        (int)(sp->alpha + (grown_a - sp->alpha) * (1.0f - blend)),
                        0, max_val);
                } else {
                    A_u_char f8 = (A_u_char)std::clamp((int)(factor * max_val), 0, max_val);
                    dp->alpha = sp->alpha;
                    dp->red   = (A_u_char)std::clamp((int)(sp->red   + (f8 - sp->red)   * (1.0f - blend)), 0, max_val);
                    dp->green = (A_u_char)std::clamp((int)(sp->green + (f8 - sp->green) * (1.0f - blend)), 0, max_val);
                    dp->blue  = (A_u_char)std::clamp((int)(sp->blue  + (f8 - sp->blue)  * (1.0f - blend)), 0, max_val);
                }
            } else {
                PF_Pixel16 *sp = (PF_Pixel16 *)((char *)src->data + y * src->rowbytes) + x;
                PF_Pixel16 *dp = (PF_Pixel16 *)((char *)dst->data + y * dst->rowbytes) + x;

                if (p.channel == CHAN_ALPHA) {
                    dp->red   = sp->red;
                    dp->green = sp->green;
                    dp->blue  = sp->blue;
                    int grown_a = std::clamp((int)(factor * max_val), 0, max_val);
                    dp->alpha = (A_u_short)std::clamp(
                        (int)(sp->alpha + (grown_a - sp->alpha) * (1.0f - blend)),
                        0, max_val);
                } else {
                    int f16 = std::clamp((int)(factor * max_val), 0, max_val);
                    dp->alpha = sp->alpha;
                    dp->red   = (A_u_short)std::clamp((int)(sp->red   + (f16 - sp->red)   * (1.0f - blend)), 0, max_val);
                    dp->green = (A_u_short)std::clamp((int)(sp->green + (f16 - sp->green) * (1.0f - blend)), 0, max_val);
                    dp->blue  = (A_u_short)std::clamp((int)(sp->blue  + (f16 - sp->blue)  * (1.0f - blend)), 0, max_val);
                }
            }
        }
    }
}

void process_grow(
    const PF_EffectWorld *src,
    PF_EffectWorld       *dst,
    const GrowParams     &p,
    int                   max_val)
{
    int w = src->width;
    int h = src->height;

    std::vector<float> mask(w * h);
    build_mask(src, mask.data(), p, max_val);

    if (p.mode == MODE_SHRINK) {
        for (int i = 0; i < w * h; ++i)
            mask[i] = (mask[i] < 1.0f) ? INF : 0.0f;
    }

    if (p.shape == SHAPE_CIRCLE) {
        edt_2d(mask.data(), w, h);
    } else if (p.shape == SHAPE_SQUARE) {
        dt_chebyshev(mask.data(), w, h);
        for (int i = 0; i < w * h; ++i)
            mask[i] = mask[i] * mask[i];
    } else {
        dt_manhattan(mask.data(), w, h);
        for (int i = 0; i < w * h; ++i)
            mask[i] = mask[i] * mask[i];
    }

    apply_grow(src, dst, mask.data(), p, max_val);
}
