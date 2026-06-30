#pragma once

#include "Grow.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

// Felzenszwalb & Huttenlocher style 1D distance transform — O(n) per row/col.
// Total cost: O(width * height) regardless of radius.
void dt_1d(const float *f, float *d, int n);

// Full 2D Euclidean distance transform (separable, two 1D passes).
void edt_2d(float *grid, int width, int height);

// Chebyshev (L-inf) distance transform — produces square-shaped grow.
void dt_chebyshev(float *grid, int width, int height);

// Manhattan (L1) distance transform — produces diamond-shaped grow.
void dt_manhattan(float *grid, int width, int height);

// High-level: run the full grow pipeline on a src→dst pair.
void process_grow(
    const PF_EffectWorld *src,
    PF_EffectWorld       *dst,
    const GrowParams     &p,
    int                   max_val);

// Build a binary mask from the source layer based on channel/threshold settings.
void build_mask(
    const PF_EffectWorld *src,
    float                *mask,
    const GrowParams     &p,
    int                   max_val);

// Apply the grow result back to the output layer.
void apply_grow(
    const PF_EffectWorld *src,
    PF_EffectWorld       *dst,
    const float          *dist,
    const GrowParams     &p,
    int                   max_val);
