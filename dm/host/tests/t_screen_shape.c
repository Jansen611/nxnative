#include "../screen_shape.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    assert(DmScreenSizeForAspect(16.0 / 9.0) == 0);
    assert(DmScreenSizeForAspect(0) == 0);
    assert(DmScreenSizeForAspect(NAN) == 0);
    assert(DmScreenSizeForAspect(INFINITY) == 0);
    assert(DmScreenSizeForAspect(1.29) == 0);
    assert(DmScreenSizeForAspect(2.17) == 0);
    assert(DmScreenSizeForAspect(4.0 / 3.0) == ((uint64_t)1440 << 32 | 1920));
    assert(DmScreenSizeForAspect(1.3) == ((uint64_t)1476 << 32 | 1920));
    assert(DmScreenSizeForAspect(19.5 / 9.0) == ((uint64_t)1080 << 32 | 2336));
    puts("screen shape: 9 checks passed");
    return 0;
}