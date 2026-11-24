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

static inline int DmWideMapMaskQuad(const float positions[4][2], const uint16_t indices[6], float width, float height) {
    if (width <= 0 || height <= 0) return 0;
    unsigned triangles[2] = {0, 0};
    for (unsigned index = 0; index < 6; index++) {
        if (indices[index] >= 4) return 0;
        const float *position = positions[indices[index]];
        if ((position[0] != 0 && position[0] != width) || (position[1] != 0 && position[1] != height)) return 0;
        unsigned corner = (position[0] == width ? 1u : 0u) | (position[1] == height ? 2u : 0u);
        unsigned bit = 1u << corner;
        if (triangles[index / 3] & bit) return 0;
        triangles[index / 3] |= bit;
    }
    unsigned shared = triangles[0] & triangles[1];
    return (triangles[0] | triangles[1]) == 15 && (shared == 9 || shared == 6);
}

// Align the observed wide-to-fixed-to-wide preview map with the source map.
static inline double DmWidePreviewScale(uint32_t width, uint32_t height, uint32_t originalWidth, uint32_t originalHeight) {
    if (!width || !height || !originalWidth || !originalHeight ||
        (uint64_t)width * originalHeight <= (uint64_t)height * originalWidth) return 1;
    double ratio = (double)originalWidth * height / ((double)originalHeight * width);
    return ratio * ratio;
}

// Recognise the observed rectangular world-map icon geometry and atlas extents.
static inline int DmWideWorldIcon(const float positions[4][2], const float uvs[4][2], float atlasWidth, float atlasHeight,
                                  const float *sizes, unsigned count) {
    float width = fabsf(positions[2][0] - positions[0][0]);
    float height = fabsf(positions[2][1] - positions[0][1]);
    float uvWidth = fabsf(uvs[2][0] - uvs[0][0]) * atlasWidth;
    float uvHeight = fabsf(uvs[2][1] - uvs[0][1]) * atlasHeight;
        int matchingSize = 0;
        for (unsigned index = 0; index < count; index++) if (fabsf(uvWidth - sizes[index]) < 0.01f) matchingSize = 1;
        if (!(width > 0 && isfinite(width) && fabsf(width - height) < 0.01f &&
            fabsf(uvWidth - uvHeight) < 0.01f && matchingSize)) return 0;
    return positions[0][0] == positions[1][0] && positions[1][1] == positions[2][1] &&
        positions[2][0] == positions[3][0] && positions[3][1] == positions[0][1] &&
        uvs[0][0] == uvs[1][0] && uvs[1][1] == uvs[2][1] &&
        uvs[2][0] == uvs[3][0] && uvs[3][1] == uvs[0][1];
}

// Check the triangles of each observed four-vertex quad in a batch.
static inline int DmWideWorldQuadIndices(const uint16_t *indices, unsigned quadCount) {
    if (!quadCount || quadCount > 2) return 0;
    for (unsigned quadIndex = 0; quadIndex < quadCount; quadIndex++) {
        unsigned triangles[2] = {0, 0};
        for (unsigned index = 0; index < 6; index++) {
            unsigned vertex = indices[quadIndex * 6 + index];
            if (vertex < quadIndex * 4 || vertex >= quadIndex * 4 + 4) return 0;
            unsigned bit = 1u << (vertex - quadIndex * 4);
            if (triangles[index / 3] & bit) return 0;
            triangles[index / 3] |= bit;
        }
        unsigned shared = triangles[0] & triangles[1];
        if ((triangles[0] | triangles[1]) != 15 || (shared != 5 && shared != 10)) return 0;
    }
    return 1;
}

// Recognise a batch only when every indexed quad is an observed world-map icon.
static inline int DmWideWorldIconBatch(const float positions[][4][2], const float uvs[][4][2], const uint16_t *indices,
                                       unsigned quadCount, float atlasWidth, float atlasHeight,
                                       const float *sizes, unsigned sizeCount) {
    if (!DmWideWorldQuadIndices(indices, quadCount)) return 0;
    for (unsigned quadIndex = 0; quadIndex < quadCount; quadIndex++)
        if (!DmWideWorldIcon(positions[quadIndex], uvs[quadIndex], atlasWidth, atlasHeight, sizes, sizeCount)) return 0;
    return 1;
}

// Recognise the observed pair of rotating, concentric pulse texture squares.
static inline int DmWideWorldPulse(const float positions[][4][2], const float uvs[][4][2], const uint16_t *indices,
                                   float textureWidth, float textureHeight, float extent) {
    if (textureWidth <= 0 || textureHeight <= 0 || extent <= 0 || !DmWideWorldQuadIndices(indices, 2)) return 0;
    float lengths[2], centresX[2], centresY[2];
    for (unsigned quadIndex = 0; quadIndex < 2; quadIndex++) {
        const float (*position)[2] = positions[quadIndex], (*uv)[2] = uvs[quadIndex];
        float edge1X = position[1][0] - position[0][0], edge1Y = position[1][1] - position[0][1];
        float edge2X = position[3][0] - position[0][0], edge2Y = position[3][1] - position[0][1];
        lengths[quadIndex] = edge1X * edge1X + edge1Y * edge1Y;
        float otherLength = edge2X * edge2X + edge2Y * edge2Y;
        centresX[quadIndex] = (position[0][0] + position[2][0]) / 2;
        centresY[quadIndex] = (position[0][1] + position[2][1]) / 2;
        if (!(lengths[quadIndex] > 0 && isfinite(lengths[quadIndex]) && isfinite(otherLength) &&
              isfinite(centresX[quadIndex]) && isfinite(centresY[quadIndex])) ||
            fabsf(lengths[quadIndex] - otherLength) > lengths[quadIndex] * 0.001f ||
            fabsf(edge1X * edge2X + edge1Y * edge2Y) > lengths[quadIndex] * 0.001f ||
            fabsf(position[0][0] + position[2][0] - position[1][0] - position[3][0]) > 0.01f ||
            fabsf(position[0][1] + position[2][1] - position[1][1] - position[3][1]) > 0.01f ||
            uv[0][0] != 0 || uv[0][1] != 0 || uv[1][0] != 0 || uv[3][1] != 0 ||
            uv[1][1] != uv[2][1] || uv[2][0] != uv[3][0] ||
            !(fabsf(uv[2][0] * textureWidth - extent) < 0.01f) || !(fabsf(uv[2][1] * textureHeight - extent) < 0.01f)) return 0;
    }
    return fabsf(centresX[0] - centresX[1]) < 0.01f && fabsf(centresY[0] - centresY[1]) < 0.01f &&
        fabsf(lengths[0] - lengths[1]) < lengths[0] * 0.001f;
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
