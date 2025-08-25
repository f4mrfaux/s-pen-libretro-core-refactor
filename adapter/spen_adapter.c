#include "spen_adapter.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef __linux__
#define _GNU_SOURCE
#endif

/* Internal S-Pen context structure */
struct spen_context {
    spen_state_t current_state;
    spen_state_t previous_state;
    
    /* Hover guard state */
    bool hover_guard_active;
    uint64_t hover_guard_until;
    float hover_guard_x, hover_guard_y;
    int hover_guard_time_ms;
    float hover_guard_radius_px;
    
    /* Configuration */
    bool require_contact_for_click;
    
    /* Comprehensive input mapping configuration */
    spen_action_t tap_action;
    spen_action_t barrel_action;
    spen_hover_behavior_t hover_behavior;
    float pressure_threshold;
    
    /* Coordinate transformation */
    spen_coordinate_transform_t transform_func;
    void* transform_user_data;
};

/* Helper function to get current time in milliseconds */
static uint64_t spen_get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* Helper function to calculate distance between two points */
static float spen_distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

spen_context_t* spen_init(void) {
    spen_context_t* ctx = calloc(1, sizeof(spen_context_t));
    if (!ctx) {
        return NULL;
    }
    
    /* Initialize default configuration */
    ctx->require_contact_for_click = true;
    ctx->hover_guard_time_ms = 100;
    ctx->hover_guard_radius_px = 12.0f;
    
    /* Initialize input mapping defaults */
    ctx->tap_action = SPEN_ACTION_LEFT_CLICK;
    ctx->barrel_action = SPEN_ACTION_RIGHT_CLICK;
    ctx->hover_behavior = SPEN_HOVER_CURSOR;
    ctx->pressure_threshold = 0.1f;
    
    /* Initialize state */
    ctx->current_state.tool_type = SPEN_TOOL_UNKNOWN;
    ctx->previous_state.tool_type = SPEN_TOOL_UNKNOWN;
    
    return ctx;
}

void spen_cleanup(spen_context_t* ctx) {
    if (ctx) {
        free(ctx);
    }
}

void spen_on_hover(spen_context_t* ctx, float x, float y, float pressure) {
    if (!ctx) return;
    
    uint64_t now = spen_get_time_ms();
    
    /* Update state */
    ctx->previous_state = ctx->current_state;
    ctx->current_state.x = x;
    ctx->current_state.y = y;
    ctx->current_state.pressure = pressure;
    ctx->current_state.contact = false;
    ctx->current_state.hover = true;
    ctx->current_state.timestamp = now;
    
    /* Arm hover guard */
    ctx->hover_guard_active = true;
    ctx->hover_guard_until = now + ctx->hover_guard_time_ms;
    ctx->hover_guard_x = x;
    ctx->hover_guard_y = y;
}

void spen_on_contact(spen_context_t* ctx, float x, float y, float pressure) {
    if (!ctx) return;
    
    uint64_t now = spen_get_time_ms();
    
    /* Update state */
    ctx->previous_state = ctx->current_state;
    ctx->current_state.x = x;
    ctx->current_state.y = y;
    ctx->current_state.pressure = pressure;
    ctx->current_state.contact = true;
    ctx->current_state.hover = false;
    ctx->current_state.timestamp = now;
    
    /* Disable hover guard on actual contact */
    ctx->hover_guard_active = false;
}

void spen_on_button(spen_context_t* ctx, spen_button_t button, bool pressed) {
    if (!ctx || button < 0 || button >= 32) return;
    
    if (pressed) {
        ctx->current_state.button_state |= (1U << button);
    } else {
        ctx->current_state.button_state &= ~(1U << button);
    }
    
    ctx->current_state.timestamp = spen_get_time_ms();
}

void spen_on_tool_type(spen_context_t* ctx, spen_tool_type_t tool_type) {
    if (!ctx) return;
    
    ctx->current_state.tool_type = tool_type;
    ctx->current_state.timestamp = spen_get_time_ms();
}

const spen_state_t* spen_get_state(spen_context_t* ctx) {
    if (!ctx) return NULL;
    return &ctx->current_state;
}

bool spen_is_active(spen_context_t* ctx) {
    if (!ctx) return false;
    return ctx->current_state.contact || ctx->current_state.hover;
}

bool spen_require_contact(spen_context_t* ctx) {
    if (!ctx) return true;
    return ctx->require_contact_for_click;
}

int16_t spen_emit_libretro_pointer(spen_context_t* ctx, 
                                   retro_input_state_t input_state_cb,
                                   unsigned port, unsigned device,
                                   unsigned index, unsigned id) {
    if (!ctx) return 0;
    
    const spen_state_t* state = &ctx->current_state;
    
    /* Handle libretro pointer device queries */
    if (device == 6) { /* RETRO_DEVICE_POINTER */
        switch (id) {
            case 0: /* RETRO_DEVICE_ID_POINTER_X */
                if (ctx->transform_func && (state->contact || state->hover)) {
                    int transformed_x, transformed_y;
                    ctx->transform_func(state->x, state->y, &transformed_x, &transformed_y,
                                        ctx->transform_user_data);
                    return (int16_t)transformed_x;
                }
                return (int16_t)state->x;
                
            case 1: /* RETRO_DEVICE_ID_POINTER_Y */
                if (ctx->transform_func && (state->contact || state->hover)) {
                    int transformed_x, transformed_y;
                    ctx->transform_func(state->x, state->y, &transformed_x, &transformed_y,
                                        ctx->transform_user_data);
                    return (int16_t)transformed_y;
                }
                return (int16_t)state->y;
                
            case 2: /* RETRO_DEVICE_ID_POINTER_PRESSED */
                if (ctx->require_contact_for_click) {
                    return state->contact ? 1 : 0;
                } else {
                    /* Allow barrel button or contact */
                    return (state->contact || 
                            (state->button_state & (1U << SPEN_BUTTON_BARREL))) ? 1 : 0;
                }
                
            case 3: /* RETRO_DEVICE_ID_POINTER_COUNT */
                return (state->contact || state->hover) ? 1 : 0;
                
            default:
                break;
        }
    }
    
    /* Check hover guard for phantom touch suppression */
    if (ctx->hover_guard_active) {
        uint64_t now = spen_get_time_ms();
        if (now < ctx->hover_guard_until) {
            /* Check if this might be a phantom touch near the hover location */
            float distance = spen_distance(state->x, state->y, 
                                           ctx->hover_guard_x, ctx->hover_guard_y);
            if (distance <= ctx->hover_guard_radius_px) {
                /* Suppress phantom touch */
                return 0;
            }
        } else {
            /* Hover guard expired */
            ctx->hover_guard_active = false;
        }
    }
    
    /* Fall back to original input callback for non-pointer devices */
    return input_state_cb ? input_state_cb(port, device, index, id) : 0;
}

void spen_set_coordinate_transform(spen_context_t* ctx, 
                                   spen_coordinate_transform_t transform_func,
                                   void* user_data) {
    if (!ctx) return;
    
    ctx->transform_func = transform_func;
    ctx->transform_user_data = user_data;
}

void spen_configure_hover_guard(spen_context_t* ctx, 
                                int guard_time_ms, float guard_radius_px) {
    if (!ctx) return;
    
    ctx->hover_guard_time_ms = guard_time_ms;
    ctx->hover_guard_radius_px = guard_radius_px;
}

void spen_configure_mapping(spen_context_t* ctx,
                           spen_action_t tap_action,
                           spen_action_t barrel_action,
                           spen_hover_behavior_t hover_behavior, 
                           float pressure_threshold) {
    if (!ctx) return;
    
    ctx->tap_action = tap_action;
    ctx->barrel_action = barrel_action;
    ctx->hover_behavior = hover_behavior;
    ctx->pressure_threshold = pressure_threshold;
}

/* Helper function to check if an action is triggered by current state */
static bool spen_action_triggered(spen_context_t* ctx, spen_action_t action) {
    const spen_state_t* state = &ctx->current_state;
    bool barrel_pressed = state->button_state & (1U << SPEN_BUTTON_BARREL);
    bool contact = state->contact;
    bool hover_active = state->hover && (ctx->hover_behavior != SPEN_HOVER_DISABLED);
    
    switch (action) {
        case SPEN_ACTION_DISABLED:
            return false;
        case SPEN_ACTION_LEFT_CLICK:
        case SPEN_ACTION_RIGHT_CLICK:  
        case SPEN_ACTION_MIDDLE_CLICK:
        case SPEN_ACTION_TRIGGER:
        case SPEN_ACTION_RELOAD:
        case SPEN_ACTION_OFFSCREEN:
            /* Check if tap or barrel triggers this action */
            if (contact && ctx->tap_action == action) return true;
            if (barrel_pressed && ctx->barrel_action == action) return true;
            /* For cursor/hover actions, check hover with pressure threshold */
            if (hover_active && ctx->tap_action == action && state->pressure >= ctx->pressure_threshold) return true;
            return false;
        case SPEN_ACTION_CURSOR:
            return hover_active;
        default:
            return false;
    }
}

bool spen_get_mapped_button(spen_context_t* ctx, int device_type, int button_id) {
    if (!ctx) return false;
    
    /* Mouse device button mapping */
    if (device_type == 1) { /* RETRO_DEVICE_MOUSE equivalent */
        switch (button_id) {
            case 0: /* Left click */
                return spen_action_triggered(ctx, SPEN_ACTION_LEFT_CLICK);
            case 1: /* Right click */
                return spen_action_triggered(ctx, SPEN_ACTION_RIGHT_CLICK);
            case 2: /* Middle click */  
                return spen_action_triggered(ctx, SPEN_ACTION_MIDDLE_CLICK);
        }
    }
    
    /* Lightgun device button mapping */
    if (device_type == 6) { /* RETRO_DEVICE_LIGHTGUN equivalent */
        switch (button_id) {
            case 2: /* Trigger */
                return spen_action_triggered(ctx, SPEN_ACTION_TRIGGER);
            case 16: /* Reload (offscreen shot) */
                return spen_action_triggered(ctx, SPEN_ACTION_RELOAD) ||
                       spen_action_triggered(ctx, SPEN_ACTION_OFFSCREEN);
            case 3: /* Cursor (for lightgun cursor visibility) */
                /* Special case: hover tracking for lightgun */
                if (ctx->hover_behavior == SPEN_HOVER_LIGHTGUN_TRACKING) {
                    const spen_state_t* state = &ctx->current_state;
                    return state->hover && !state->contact;
                }
                return spen_action_triggered(ctx, SPEN_ACTION_CURSOR);
        }
    }
    
    return false;
}