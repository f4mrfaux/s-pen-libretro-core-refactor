# SNES9x S-Pen Pointer Support Implementation

## Following Libretro Style Guide

Based on analysis of existing libretro cores and established patterns:

### 1. Update Existing Lightgun Mode (for consistency)
```c
// In libretro_core_options.h - Update existing option for consistency
{
   "snes9x_lightgun_mode",
   "Light Gun Mode", 
   "Mode",
   "Use relative light gun tracking or absolute pointer positioning.",
   NULL,
   "lightgun",
   {
      { "lightgun",   "Light Gun" },        // Keep existing default
      { "absolute",   "Absolute" },         // Change from "Touchscreen" 
      { NULL, NULL },
   },
   "lightgun"  // Default unchanged for compatibility
}
```

### 2. Add New Mouse Mode Option  
```c
// In libretro_core_options.h - Add new option
{
   "snes9x_mouse_mode",
   "SNES Mouse Mode",
   "Mode", 
   "Use relative mouse movement or absolute pointer positioning for SNES Mouse input.",
   NULL,
   "input",           // Could also use "lightgun" category
   {
      { "relative",   "Relative" },        // Default - preserves existing behavior
      { "absolute",   "Absolute" },        // New S-Pen mode
      { NULL, NULL },
   },
   "relative"
}
```

### 3. Implementation Constants (following Genesis Plus GX pattern)
```c
// Consistent enum-based approach
enum RetroInputModes { 
    RetroRelative = 0, 
    RetroAbsolute = 1 
};

static enum RetroInputModes snes_lightgun_mode = RetroRelative;
static enum RetroInputModes snes_mouse_mode = RetroRelative;
```

### 4. Core Option Processing
```c
// Update existing lightgun processing (maintain compatibility)
var.key = "snes9x_lightgun_mode";
if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
    if (var.value && !strcmp(var.value, "absolute")) {
        snes_lightgun_mode = RetroAbsolute; 
    } else {
        snes_lightgun_mode = RetroRelative;  // Default + legacy "lightgun" value
    }
}

// Add new mouse option processing  
var.key = "snes9x_mouse_mode";
if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
    if (var.value && !strcmp(var.value, "absolute")) {
        snes_mouse_mode = RetroAbsolute;
    } else {
        snes_mouse_mode = RetroRelative;  // Default
    }
}
```

### 5. Input Handling Updates
```c
// Update lightgun logic (change comparison)
case RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE:
    if (snes_lightgun_mode == RetroAbsolute) {
        input_handle_pointer_lightgun(port, RETRO_DEVICE_LIGHTGUN_SUPER_SCOPE, BTN_POINTER);
    } else {
        // Existing lightgun logic...
    }

// Add mouse pointer support
case RETRO_DEVICE_MOUSE:
    if (snes_mouse_mode == RetroAbsolute) {
        // NEW: S-Pen absolute mode
        if (input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)) {
            // Convert -32768..32767 to SNES coordinates
            int pointer_x = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
            int pointer_y = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
            snes_mouse_state[port][0] = ((pointer_x + 32768) * 256) / 65536;  
            snes_mouse_state[port][1] = ((pointer_y + 32768) * 224) / 65536;  
        }
        // Handle buttons - stylus tip = left click, fallback to mouse buttons
        for (int i = MOUSE_LEFT; i <= MOUSE_LAST; i++) {
            bool pressed = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, i);
            if (i == MOUSE_LEFT) {
                pressed |= input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
            }
            S9xReportButton(MAKE_BUTTON(port + 1, i), pressed);
        }
    } else {
        // EXISTING: Relative mode (unchanged - preserves all current behavior)
        _x = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
        _y = input_state_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
        snes_mouse_state[port][0] += _x;
        snes_mouse_state[port][1] += _y;
        for (int i = MOUSE_LEFT; i <= MOUSE_LAST; i++)
            S9xReportButton(MAKE_BUTTON(port + 1, i), input_state_cb(port, RETRO_DEVICE_MOUSE, 0, i));
    }
    S9xReportPointer(BTN_POINTER + port, snes_mouse_state[port][0], snes_mouse_state[port][1]);
    break;
```

## Benefits
- **Consistent terminology**: "Absolute" vs "Relative" matches libretro conventions
- **Backwards compatible**: Existing users see no changes
- **Proper categorization**: Mouse option in "input" category, separate from lightgun
- **Safe defaults**: Both default to existing behavior
- **S-Pen optimized**: Direct tap-to-point for Mario Paint and other mouse games

## Usage for S-Pen Users
1. **For lightguns**: Set device to Super Scope + `snes9x_lightgun_mode = "absolute"`  
2. **For mouse games**: Set device to SNES Mouse + `snes9x_mouse_mode = "absolute"`