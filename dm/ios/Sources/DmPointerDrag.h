#ifndef DM_POINTER_DRAG_H
#define DM_POINTER_DRAG_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool active, dragging;
    double originX, originY, previousX, previousY, deltaX, deltaY, sampleTime;
} DmPointerDrag;

// Discard both an uncommitted click and any accumulated camera movement.
static inline void PointerDragReset(DmPointerDrag *state) {
    *state = (DmPointerDrag){0};
}

// Defer the primary click until release so a drag never also touches the game.
static inline void PointerDragBegin(DmPointerDrag *state, double positionX, double positionY, double timestamp) {
    *state = (DmPointerDrag){.active = true, .originX = positionX, .originY = positionY,
                            .previousX = positionX, .previousY = positionY, .sampleTime = timestamp};
}

// Cross a small point-based slop once, then accumulate relative movement only.
static inline void PointerDragMove(DmPointerDrag *state, double positionX, double positionY) {
    if (!state->active) return;
    if (!state->dragging && hypot(positionX - state->originX, positionY - state->originY) >= 5.0)
        state->dragging = true;
    if (state->dragging) {
        state->deltaX += positionX - state->previousX;
        state->deltaY += positionY - state->previousY;
    }
    state->previousX = positionX; state->previousY = positionY;
}

// Convert point velocity to a circular right stick; a stationary interval is zero.
static inline void PointerDragSample(DmPointerDrag *state, double timestamp, int32_t *rightX, int32_t *rightY) {
    *rightX = *rightY = 0;
    double elapsed = timestamp - state->sampleTime;
    if (!state->active || elapsed <= 0) return;
    double horizontal = state->deltaX / (elapsed * 50.0), vertical = -state->deltaY / (elapsed * 50.0);
    double magnitude = hypot(horizontal, vertical);
    if (magnitude > 1) { horizontal /= magnitude; vertical /= magnitude; }
    *rightX = (int32_t)lrint(horizontal * 32767); *rightY = (int32_t)lrint(vertical * 32767);
    state->deltaX = state->deltaY = 0; state->sampleTime = timestamp;
}

// Commit a click only when the gesture ended normally without becoming a drag.
static inline bool PointerDragEnd(DmPointerDrag *state, bool cancelled) {
    bool click = state->active && !state->dragging && !cancelled;
    PointerDragReset(state);
    return click;
}

#endif