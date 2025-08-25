# SNES Mouse Pointer Support Design

## Current Implementation Analysis
- SNES Mouse uses `RETRO_DEVICE_MOUSE` with relative coordinates
- Accumulates deltas in `snes_mouse_state[port][0/1] += _x/_y`
- This works with touchscreen via relative motion
- Separate from lightgun touchscreen support (which already exists)

## Proposed Solution (Safe Addition)

### 1. New Core Option
```c
{
   "snes9x_mouse_mode",
   "SNES Mouse Mode", 
   "Mode",
   "Use relative mouse movement or absolute pointer coordinates for SNES Mouse. 'Relative' preserves classic behavior and touchscreen compatibility. 'Pointer' enables direct stylus/touch positioning for games like Mario Paint.",
   NULL,
   "input",
   {
      { "Relative", "Relative (Classic)" },
      { "Pointer", "Absolute Pointer" }, 
      { NULL, NULL },
   },
   "Relative"
}
```

### 2. Implementation Logic
```c
case RETRO_DEVICE_MOUSE:
    if (setting_snes_mouse_pointer) {
        // NEW: Absolute pointer mode for S-Pen/stylus
        if (input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)) {
            // Convert -32768..32767 to SNES screen coordinates (0..255)
            int pointer_x = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
            int pointer_y = input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
            snes_mouse_state[port][0] = ((pointer_x + 32768) * 256) / 65536;
            snes_mouse_state[port][1] = ((pointer_y + 32768) * 224) / 65536; // SNES height
        }
        // Handle mouse buttons normally
        for (int i = MOUSE_LEFT; i <= MOUSE_LAST; i++)
            S9xReportButton(MAKE_BUTTON(port + 1, i), 
                input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED) ||
                input_state_cb(port, RETRO_DEVICE_MOUSE, 0, i));
    } else {
        // EXISTING: Relative mode (default) - preserves all current behavior
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

### 3. Safety Features
- **Default to "Relative"**: Existing users see no behavior change
- **Fallback support**: If pointer not pressed, still accept regular mouse input  
- **Separate from lightgun**: Does not interfere with existing touchscreen lightgun support
- **Opt-in only**: Must explicitly choose "Pointer" mode

### 4. Benefits for S-Pen Users
- Direct tap-to-point positioning in Mario Paint
- No cursor jumping from accumulated deltas
- Natural stylus behavior
- Works with RetroArch's S-Pen hover guard system

### 5. Backwards Compatibility
- All existing touchscreen mouse users continue working normally
- No changes to default behavior
- No changes to existing core options
- Existing save states remain compatible

This approach adds S-Pen support while being completely safe for existing users.