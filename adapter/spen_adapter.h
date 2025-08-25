#ifndef SPEN_ADAPTER_H
#define SPEN_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* S-Pen button definitions */
typedef enum {
    SPEN_BUTTON_TIP = 0,
    SPEN_BUTTON_BARREL = 1,
    SPEN_BUTTON_ERASER = 2
} spen_button_t;

/* S-Pen tool types */
typedef enum {
    SPEN_TOOL_STYLUS = 0,
    SPEN_TOOL_FINGER = 1,
    SPEN_TOOL_UNKNOWN = 2
} spen_tool_type_t;

/* S-Pen state */
typedef struct {
    float x, y;                    /* Current position */
    float pressure;                /* 0.0 - 1.0 */
    float distance;                /* Hover distance */
    spen_tool_type_t tool_type;    /* Tool type */
    bool contact;                  /* Tip touching screen */
    bool hover;                    /* Hovering near screen */
    uint32_t button_state;         /* Button bitmask */
    uint64_t timestamp;            /* Event timestamp */
} spen_state_t;

/* S-Pen context */
typedef struct spen_context spen_context_t;

/* Libretro input callback type */
typedef int16_t (*retro_input_state_t)(unsigned port, unsigned device, 
                                       unsigned index, unsigned id);

/**
 * Initialize S-Pen adapter context
 * @return S-Pen context or NULL on failure
 */
spen_context_t* spen_init(void);

/**
 * Cleanup S-Pen adapter context
 * @param ctx S-Pen context to cleanup
 */
void spen_cleanup(spen_context_t* ctx);

/**
 * Handle hover events (cursor movement without contact)
 * @param ctx S-Pen context
 * @param x X coordinate (-32768 to 32767)
 * @param y Y coordinate (-32768 to 32767) 
 * @param pressure Pressure value (0.0 to 1.0)
 */
void spen_on_hover(spen_context_t* ctx, float x, float y, float pressure);

/**
 * Handle contact events (tip touching screen)
 * @param ctx S-Pen context
 * @param x X coordinate (-32768 to 32767)
 * @param y Y coordinate (-32768 to 32767)
 * @param pressure Pressure value (0.0 to 1.0)
 */
void spen_on_contact(spen_context_t* ctx, float x, float y, float pressure);

/**
 * Handle button events (barrel button, eraser, etc.)
 * @param ctx S-Pen context
 * @param button Button type
 * @param pressed True if pressed, false if released
 */
void spen_on_button(spen_context_t* ctx, spen_button_t button, bool pressed);

/**
 * Handle tool type detection
 * @param ctx S-Pen context
 * @param tool_type Tool type (stylus, finger, etc.)
 */
void spen_on_tool_type(spen_context_t* ctx, spen_tool_type_t tool_type);

/**
 * Get current S-Pen state
 * @param ctx S-Pen context
 * @return Current S-Pen state
 */
const spen_state_t* spen_get_state(spen_context_t* ctx);

/**
 * Check if S-Pen is currently active (contact or hover)
 * @param ctx S-Pen context
 * @return True if S-Pen is active
 */
bool spen_is_active(spen_context_t* ctx);

/**
 * Check if contact detection should be required for clicks
 * @param ctx S-Pen context
 * @return True if contact is required
 */
bool spen_require_contact(spen_context_t* ctx);

/**
 * Emit libretro pointer events based on current S-Pen state
 * @param ctx S-Pen context
 * @param input_state_cb Libretro input state callback
 * @param port Controller port
 * @return Simulated input state for libretro
 */
int16_t spen_emit_libretro_pointer(spen_context_t* ctx, 
                                   retro_input_state_t input_state_cb,
                                   unsigned port, unsigned device,
                                   unsigned index, unsigned id);

/**
 * Set coordinate transformation function
 * @param ctx S-Pen context
 * @param transform_func Function to transform coordinates for specific core
 */
typedef void (*spen_coordinate_transform_t)(float in_x, float in_y, 
                                            int* out_x, int* out_y, 
                                            void* user_data);
void spen_set_coordinate_transform(spen_context_t* ctx, 
                                   spen_coordinate_transform_t transform_func,
                                   void* user_data);

/**
 * Configure hover guard settings
 * @param ctx S-Pen context
 * @param guard_time_ms Time to guard against phantom touches after hover
 * @param guard_radius_px Spatial radius for phantom touch detection
 */
void spen_configure_hover_guard(spen_context_t* ctx, 
                                int guard_time_ms, float guard_radius_px);

/**
 * Comprehensive S-Pen input mapping configuration
 */
typedef enum {
    SPEN_ACTION_DISABLED = 0,
    SPEN_ACTION_LEFT_CLICK = 1,
    SPEN_ACTION_RIGHT_CLICK = 2,
    SPEN_ACTION_MIDDLE_CLICK = 3,
    SPEN_ACTION_TRIGGER = 4,
    SPEN_ACTION_RELOAD = 5,
    SPEN_ACTION_OFFSCREEN = 6,
    SPEN_ACTION_CURSOR = 7
} spen_action_t;

typedef enum {
    SPEN_HOVER_DISABLED = 0,
    SPEN_HOVER_CURSOR = 1,
    SPEN_HOVER_LIGHTGUN_TRACKING = 2
} spen_hover_behavior_t;

/**
 * Configure comprehensive S-Pen input mapping
 * @param ctx S-Pen context
 * @param tap_action What stylus tap does (contact)
 * @param barrel_action What barrel button does
 * @param hover_behavior How hover state is handled
 * @param pressure_threshold Minimum pressure for contact detection
 */
void spen_configure_mapping(spen_context_t* ctx,
                           spen_action_t tap_action,
                           spen_action_t barrel_action,
                           spen_hover_behavior_t hover_behavior, 
                           float pressure_threshold);

/**
 * Get enhanced input state with button mapping applied
 * @param ctx S-Pen context
 * @param device_type RETRO_DEVICE_MOUSE or RETRO_DEVICE_LIGHTGUN equivalent
 * @param button_id Which button to query
 * @return Mapped button state
 */
bool spen_get_mapped_button(spen_context_t* ctx, int device_type, int button_id);

#ifdef __cplusplus
}
#endif

#endif /* SPEN_ADAPTER_H */