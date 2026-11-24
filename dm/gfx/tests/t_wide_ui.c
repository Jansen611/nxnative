#include "../nvn/nvn_wide_ui.h"
#include <stdio.h>

static int failures;

static void Check(int result, const char *message) {
    printf("%s: %s\n", message, result ? "PASS" : "FAIL");
    failures += !result;
}

int main(void) {
    const uint64_t SELECTED = (uint64_t)1080 << 32 | 2336;
    Check(DmWideUiEnabled("verified", "verified", 1, SELECTED, 2336, 1080, 1920, 1080), "verified wide output");
    Check(!DmWideUiEnabled("verified", "other", 1, SELECTED, 2336, 1080, 1920, 1080), "other build refused");
    Check(!DmWideUiEnabled("verified", "verified", 0, SELECTED, 2336, 1080, 1920, 1080), "disabled capability refused");
    Check(!DmWideUiEnabled("verified", "verified", 1, SELECTED, 1920, 1080, 1920, 1080), "other target refused");
    Check(!DmWideUiEnabled("verified", "verified", 1, (uint64_t)1080 << 32 | 1920, 1920, 1080, 1920, 1080), "16:9 refused");
    Check(DmWideUiShader("0123456789abcdef,fedcba9876543210", "fedcba9876543210"), "listed shader");
    Check(!DmWideUiShader("0123456789abcdef,fedcba9876543210", "0123456789abcdee"), "other shader refused");
    Check(!DmWideUiShader("0123456789abcdef,fedcba9876543210", "123456789abcdef"), "partial shader refused");
    float mapProjection[4] = {2.0f / 2336, -2.0f / 1080, -1, 1};
    Check(DmWideMapProjection(mapProjection, 2336, 1080), "wide map projection");
    Check(!DmWideMapProjection(mapProjection, 2160, 1080), "other map target refused");
    mapProjection[2] = 0;
    Check(!DmWideMapProjection(mapProjection, 2336, 1080), "translated map projection refused");
    Check(DmWideMapMargin(2336, 1080, 1920, 1080) == 208, "19.5:9 map margin");
    Check(DmWideMapMargin(2160, 1080, 1920, 1080) == 120, "18:9 map margin");
    Check(DmWideMapMargin(1920, 1080, 1920, 1080) == 0, "16:9 map unchanged");
    Check(DmWideMapMargin(1440, 1080, 1920, 1080) == 0, "narrow output unchanged");
    float maskPositions[4][2] = {{0, 0}, {0, 1080}, {1920, 1080}, {1920, 0}};
    uint16_t maskIndices[6] = {0, 1, 2, 0, 2, 3};
    Check(DmWideMapMaskQuad(maskPositions, maskIndices, 1920, 1080), "full map mask");
    maskPositions[2][0] = 1900;
    Check(!DmWideMapMaskQuad(maskPositions, maskIndices, 1920, 1080), "partial map mask refused");
    maskPositions[2][0] = 1920; maskIndices[5] = 2;
    Check(!DmWideMapMaskQuad(maskPositions, maskIndices, 1920, 1080), "degenerate mask refused");
    maskIndices[5] = 4;
    Check(!DmWideMapMaskQuad(maskPositions, maskIndices, 1920, 1080), "invalid mask index refused");
    Check(fabs(DmWidePreviewScale(2336, 1080, 1920, 1080) - (1920.0 / 2336) * (1920.0 / 2336)) < 0.000001, "19.5:9 preview alignment");
    Check(fabs(DmWidePreviewScale(2160, 1080, 1920, 1080) - (1920.0 / 2160) * (1920.0 / 2160)) < 0.000001, "18:9 preview alignment");
    Check(DmWidePreviewScale(1920, 1080, 1920, 1080) == 1, "16:9 preview unchanged");
    Check(DmWidePreviewScale(0, 1080, 1920, 1080) == 1, "invalid preview unchanged");
    float iconPositions[4][2] = {{916.8f, 496.8f}, {916.8f, 583.2f}, {1003.2f, 583.2f}, {1003.2f, 496.8f}};
    float iconUvs[4][2] = {{66.0f / 2048, 344.0f / 512}, {66.0f / 2048, 408.0f / 512},
                            {130.0f / 2048, 408.0f / 512}, {130.0f / 2048, 344.0f / 512}};
    const float ICON_SIZES[] = {64, 68, 92};
    Check(DmWideWorldIcon(iconPositions, iconUvs, 2048, 512, ICON_SIZES, 3), "world-map icon");
    iconUvs[2][0] = 140.0f / 2048;
    Check(!DmWideWorldIcon(iconPositions, iconUvs, 2048, 512, ICON_SIZES, 3), "wrong icon UV refused");
    iconUvs[2][0] = 130.0f / 2048; iconPositions[1][0] += 1;
    Check(!DmWideWorldIcon(iconPositions, iconUvs, 2048, 512, ICON_SIZES, 3), "non-rectangular icon refused");
    iconPositions[1][0] -= 1;
    float batchPositions[2][4][2], batchUvs[2][4][2];
    memcpy(batchPositions[0], iconPositions, sizeof iconPositions); memcpy(batchPositions[1], iconPositions, sizeof iconPositions);
    memcpy(batchUvs[0], iconUvs, sizeof iconUvs); memcpy(batchUvs[1], iconUvs, sizeof iconUvs);
    uint16_t batchIndices[12] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
    Check(DmWideWorldIconBatch(batchPositions, batchUvs, batchIndices, 1, 2048, 512, ICON_SIZES, 3), "single world-map icon batch");
    Check(DmWideWorldIconBatch(batchPositions, batchUvs, batchIndices, 2, 2048, 512, ICON_SIZES, 3), "two world-map icons batched");
    batchUvs[1][2][0] += 1.0f / 2048;
    Check(!DmWideWorldIconBatch(batchPositions, batchUvs, batchIndices, 2, 2048, 512, ICON_SIZES, 3), "mixed batch refused");
    batchUvs[1][2][0] -= 1.0f / 2048; batchIndices[11] = 6;
    Check(!DmWideWorldIconBatch(batchPositions, batchUvs, batchIndices, 2, 2048, 512, ICON_SIZES, 3), "degenerate icon batch refused");
    batchIndices[11] = 3;
    Check(!DmWideWorldIconBatch(batchPositions, batchUvs, batchIndices, 2, 2048, 512, ICON_SIZES, 3), "cross-quad index refused");
    batchIndices[11] = 7;
    float pulseUvs[2][4][2] = {{{0, 0}, {0, 60.0f / 64}, {60.0f / 64, 60.0f / 64}, {60.0f / 64, 0}},
                                {{0, 0}, {0, 60.0f / 64}, {60.0f / 64, 60.0f / 64}, {60.0f / 64, 0}}};
    float pulsePositions[2][4][2] = {{{932.031f, 137.659f}, {862.460f, 263.169f}, {987.969f, 332.740f}, {1057.540f, 207.230f}},
                                     {{862.460f, 207.230f}, {932.031f, 332.740f}, {1057.540f, 263.169f}, {987.969f, 137.659f}}};
    Check(DmWideWorldPulse(pulsePositions, pulseUvs, batchIndices, 64, 64, 60), "rotating world-map pulse");
    for (unsigned index = 0; index < 4; index++) pulsePositions[1][index][0] += 1;
    Check(!DmWideWorldPulse(pulsePositions, pulseUvs, batchIndices, 64, 64, 60), "different pulse geometry refused");
    for (unsigned index = 0; index < 4; index++) pulsePositions[1][index][0] -= 1;
    pulseUvs[1][0][0] += 1.0f / 64;
    Check(!DmWideWorldPulse(pulsePositions, pulseUvs, batchIndices, 64, 64, 60), "different pulse UV refused");
    float values[36] = {0};
    values[8] = 2.0f / 1920; values[13] = 2.0f / 1080; values[23] = 1;
    values[24] = values[29] = values[34] = 1;
    DmWideUiVertex vertices[6] = {
        {-960, 540, 0, 0}, {960, 540, 0, 0}, {-960, -540, 0, 0},
        {960, 540, 0, 0}, {960, -540, 0, 0}, {-960, -540, 0, 0}
    };
    Check(DmWideUiProjection(values, 1920, 1080), "orthographic projection");
    Check(DmWideUiFullOverlay(values, vertices, 1920, 1080), "full-screen quad");
    vertices[0].objectIndex = 1;
    Check(!DmWideUiFullOverlay(values, vertices, 1920, 1080), "other object matrix refused");
    vertices[0].objectIndex = 0; values[27] = 5;
    Check(!DmWideUiFullOverlay(values, vertices, 1920, 1080), "translated object refused");
    values[27] = 0; vertices[0].x = -500;
    Check(!DmWideUiFullOverlay(values, vertices, 1920, 1080), "partial rectangle refused");
    vertices[0].x = -960; vertices[4] = vertices[3];
    Check(!DmWideUiFullOverlay(values, vertices, 1920, 1080), "degenerate triangle refused");
    values[20] = 1;
    Check(!DmWideUiProjection(values, 1920, 1080), "translated projection refused");
    values[20] = 0; values[13] = NAN;
    Check(!DmWideUiProjection(values, 1920, 1080), "non-finite projection refused");
    return failures ? 1 : 0;
}
