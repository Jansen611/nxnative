#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    float x, y, z, objectIndex;
} DmWideUiVertex;

static inline int DmWideUiEnabled(const char *expectedBuild, const char *actualBuild, int enabled,
                                  uint64_t selected, uint32_t width, uint32_t height,
                                  uint32_t originalWidth, uint32_t originalHeight) {
    return enabled && expectedBuild && actualBuild && !strcmp(expectedBuild, actualBuild) && width && height &&
        originalWidth && originalHeight && (uint32_t)selected == width && (uint32_t)(selected >> 32) == height &&
        (uint64_t)width * originalHeight > (uint64_t)height * originalWidth;
}

static inline int DmWideUiShader(const char *keys, const char *key) {
    if (!keys || !key || strlen(key) != 16) return 0;
    for (const char *entry = keys; *entry;) {
        const char *end = strchr(entry, ',');
        size_t length = end ? (size_t)(end - entry) : strlen(entry);
        if (length == 16 && !memcmp(entry, key, 16)) return 1;
        if (!end) break;
        entry = end + 1;
    }
    return 0;
}

static inline int DmWideUiProjection(const float *values, float width, float height) {
    return width > 0 && height > 0 &&
        fabsf(values[8] - 2.0f / width) < 0.0000001f &&
        fabsf(values[13] - 2.0f / height) < 0.0000001f &&
        values[9] == 0 && values[11] == 0 && values[12] == 0 && values[15] == 0 &&
        values[16] == 0 && values[17] == 0 && values[19] == 0 &&
        values[20] == 0 && values[21] == 0 && values[23] == 1;
}

static inline int DmWideMapProjection(const float *values, float width, float height) {
    return width > 0 && height > 0 && fabsf(values[0] - 2.0f / width) < 0.0000001f &&
        fabsf(values[1] + 2.0f / height) < 0.0000001f && values[2] == -1 && values[3] == 1;
}

static inline double DmWideMapMargin(uint32_t width, uint32_t height, uint32_t originalWidth, uint32_t originalHeight) {
    if (!width || !height || !originalWidth || !originalHeight ||
        (uint64_t)width * originalHeight <= (uint64_t)height * originalWidth) return 0;
    return (width - (double)height * originalWidth / originalHeight) / 2;
}

static inline int DmWideUiFullOverlay(const float *values, const DmWideUiVertex *vertices, float width, float height) {
    const float IDENTITY[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    unsigned triangles[2] = {0, 0};
    if (!DmWideUiProjection(values, width, height) || memcmp(values + 24, IDENTITY, sizeof IDENTITY)) return 0;
    for (int i = 0; i < 6; i++) {
        const DmWideUiVertex *vertex = &vertices[i];
        if (vertex->objectIndex != 0 || vertex->z != 0 ||
            (vertex->x != -width / 2 && vertex->x != width / 2) ||
            (vertex->y != -height / 2 && vertex->y != height / 2)) return 0;
        unsigned corner = (vertex->x > 0 ? 1u : 0u) | (vertex->y > 0 ? 2u : 0u);
        unsigned bit = 1u << corner;
        if (triangles[i / 3] & bit) return 0;
        triangles[i / 3] |= bit;
    }
    unsigned shared = triangles[0] & triangles[1];
    return (triangles[0] | triangles[1]) == 15 && (shared == 9 || shared == 6);
}