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