#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

#define DM_SCREEN_SHAPE_MIN 1.3
#define DM_SCREEN_SHAPE_SWITCH (16.0 / 9.0)
#define DM_SCREEN_SHAPE_MAX (19.5 / 9.0)

static inline double DmMatchScreenShape(double width, double height) {
    if (!isfinite(width) || !isfinite(height) || width <= 0 || height <= 0) return 0;
    double ratio = fmax(width, height) / fmin(width, height);
    return fmin(fmax(ratio, DM_SCREEN_SHAPE_MIN), DM_SCREEN_SHAPE_MAX);
}

static inline double DmWideScreenShape(double width, double height) {
    if (!isfinite(width) || !isfinite(height) || width <= 0 || height <= 0) return 0;
    double ratio = width / height;
    return ratio > DM_SCREEN_SHAPE_SWITCH ? fmin(ratio, DM_SCREEN_SHAPE_MAX) : 0;
}

static inline unsigned DmTallerScreenHeight(double ratio) {
    unsigned height = (unsigned)(1920.0 / ratio / 48.0 + 0.5) * 48;
    if (height > 1920.0 / DM_SCREEN_SHAPE_MIN)
        height = (unsigned)(1920.0 / DM_SCREEN_SHAPE_MIN / 12.0) * 12;
    return height;
}

static inline uint64_t DmScreenSizeForAspect(double aspect) {
    if (!isfinite(aspect) || aspect < DM_SCREEN_SHAPE_MIN || aspect > DM_SCREEN_SHAPE_MAX || fabs(aspect - DM_SCREEN_SHAPE_SWITCH) <= 0.001) return 0;
    uint32_t width = 1920, height = 1080;
    if (aspect > DM_SCREEN_SHAPE_SWITCH) width = (uint32_t)(aspect * 1080.0 / 16.0 + 0.5) * 16;
    else height = DmTallerScreenHeight(aspect);
    return (uint64_t)height << 32 | width;
}

typedef struct {
    double taller;
    int automatic;
    int widescreen;
} DmScreenShapeChoice;

static inline DmScreenShapeChoice DmResolveScreenShape(const char *choice, double width, double height) {
    DmScreenShapeChoice result = {0, 0, 0};
    if (!choice || strcmp(choice, "auto")) return result;
    result.automatic = 1;
    double ratio = DmMatchScreenShape(width, height);
    if (ratio > 0 && ratio < DM_SCREEN_SHAPE_SWITCH) result.taller = ratio;
    else if (ratio >= DM_SCREEN_SHAPE_SWITCH) result.widescreen = 1;
    return result;
}