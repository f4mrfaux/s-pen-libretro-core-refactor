# S-Pen Libretro Core Refactor

## Problem Statement

Building on RetroArch's S-Pen improvements (android_input.c PR pending), this project extends **RETRO_DEVICE_POINTER support to individual libretro cores** for proper absolute tracking. While RetroArch now handles S-Pen hardware correctly (hover guard, tool detection, contact states), most cores still lack direct pointer positioning for games that originally supported mouse/lightgun input. This project adds **comprehensive S-Pen integration with full UI-based configuration** - users can customize every S-Pen interaction (tap, barrel button, hover) through familiar RetroArch menus for precise absolute positioning in drawing games, arcade shooters, and lightgun titles.

## Status

| Core | Status | Mouse Support | Lightgun Support | Notes |
|------|--------|---------------|------------------|-------|
| **SNES9x** | ✅ **Tested** | ✅ Absolute mode | ✅ Pointer mode | Mario Paint, Super Scope 6 |
| **MAME 2016** | ✅ **Tested** | ✅ Absolute mode | ✅ Lightgun mode | Area 51, arcade games |
| **SwanStation** | ✅ **Tested** | - | ✅ GunCon mode | Point Blank, PSX lightgun |
| **Genesis Plus GX** | ✅ **Available** | ✅ Built-in | ✅ Built-in | Already excellent |
| **DeSmuME** | 📋 **Planned** | - | ✅ Touchscreen | NDS lower screen |
| **MelonDS** | 📋 **Planned** | - | ✅ Touchscreen | NDS lower screen |
| **Beetle PSX** | 🔬 **Prototyping** | - | ⚠️ Research needed | Follow SwanStation pattern |

## 📚 Documentation

Complete documentation is available in the [docs/](docs/) directory:

- **[📋 Compliance Reports](docs/compliance/)** - Official compliance verification and audit reports
- **[🔧 Implementation Guides](docs/implementation/)** - Technical implementation details and testing procedures  
- **[👥 User Guides](docs/user-guides/)** - End-user setup and configuration documentation

See [docs/README.md](docs/README.md) for a complete documentation index.

## How to Try It

### Quick Start (Reference Implementation)
```bash
# Build the reference S-Pen adapter
cd adapter/
make

# Test with the sample stub core
./spen_test_harness
```

### Core Integration
Each core provides comprehensive S-Pen support through core options:

**SNES9x:**
- Set `snes9x_mouse_mode = "Absolute"` for drawing/painting game stylus input
- Set `snes9x_lightgun_mode = "Absolute"` for Super Scope direct aiming
- Configure `snes9x_spen_tap_action` - what stylus tap does (left_click, right_click, trigger, etc.)
- Configure `snes9x_spen_barrel_action` - what barrel button does (fully customizable)
- Configure `snes9x_spen_hover_behavior` - hover tracking for precision aiming

**MAME 2016:**
- Set `mame2016_mouse_mode = "absolute"` for arcade lightgun games
- Configure `mame2016_spen_tap_action` - fully mappable tap behavior
- Configure `mame2016_spen_barrel_action` - fully mappable barrel button

**SwanStation:**
- Set `swanstation_Controller_GunconInputMode = "Pointer"` for PSX lightgun games
- Configure `swanstation_spen_tap_action` - customizable tap behavior
- Configure `swanstation_spen_barrel_action` - customizable barrel button

### Code-Less UI Configuration  
**Zero coding required!** Configure everything through RetroArch menus:

1. Load your core and game
2. **Quick Menu → Options → Input** (find S-Pen options here!)
3. Set **S-Pen Tap Action** - what stylus contact does
4. Set **S-Pen Barrel Action** - what side button does  
5. Set **S-Pen Hover Behavior** - hover tracking preferences
6. **Quick Menu → Overrides → Save Game Overrides** (remembers per-game!)

**Complete visual guide:** See `UI_CONFIGURATION_GUIDE.md`

## Reference Implementation

The `adapter/` directory contains a minimal S-Pen integration library:

```c
#include "spen_adapter.h"

// Initialize S-Pen context
spen_context_t* ctx = spen_init();

// Handle hover events (no phantom touches)
spen_on_hover(ctx, x, y, pressure);

// Handle barrel button for drag operations  
spen_on_button(ctx, SPEN_BUTTON_BARREL, pressed);

// Emit libretro pointer events
spen_emit_libretro_pointer(ctx, &retro_input_callback);
```

This proves the concept before patching large cores and provides a clean integration path.

## Architecture

### Core Design Principles
- **Builds on RetroArch S-Pen Foundation**: Leverages your android_input.c improvements (hover guard, tool detection, contact states)
- **RETRO_DEVICE_POINTER Extension**: Adds proper absolute positioning to cores that previously only supported relative input
- **Additive-Only**: No existing functionality removed or modified
- **Mode-Based**: Legacy and S-Pen operate as separate code paths  
- **Architecture-Aware**: Each core uses appropriate coordinate systems
- **UI-First**: Complete configuration through RetroArch menus

### Integration Pattern
```c
if (spen_mode_enabled) {
    // S-Pen absolute positioning path
    if (input_state_cb(port, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED)) {
        // Direct stylus positioning with proper coordinate transformation
        transform_stylus_coordinates(pointer_x, pointer_y);
    }
} else {
    // Traditional input path (unchanged legacy behavior)
    handle_traditional_input();
}
```

## Game Compatibility

| Core | Game Examples | Input Type | S-Pen Experience |
|------|---------------|------------|------------------|
| SNES9x | Drawing/painting games | SNES Mouse | Direct stylus input |
| SNES9x | Super Scope 6 | Super Scope | Direct aim targeting |
| MAME 2016 | Area 51, Lethal Enforcers | Arcade Lightgun | Direct lightgun aiming |
| SwanStation | Point Blank, Time Crisis | PSX GunCon | Direct aiming with trigger |
| Genesis Plus GX | Sega Pico games | Drawing Tablet | Educational stylus games |

## Development

### Prerequisites
- Android NDK for core building
- RetroArch Android development environment
- Samsung S-Pen compatible device for testing

### Building Cores
```bash
# Build SNES9x with S-Pen support
cd cores/snes9x/
make -f Makefile.libretro

# Build MAME 2016 with S-Pen support  
cd cores/mame2016-libretro/
make -f Makefile.libretro

# Build reference S-Pen adapter
cd adapter/
make && ./spen_test_harness
```

### S-Pen Button Mapping System
The adapter provides comprehensive input mapping:

**Mouse Mode:**
- Tap → Left click
- Barrel button → Right/Middle click (configurable)
- Hover → Cursor movement
- Pressure threshold → Click sensitivity

**Lightgun Mode:**
- Tap → Quick trigger shot
- Barrel button → Trigger/Reload/Offscreen (configurable)  
- Hover → **Aim tracking without shooting** (precision aiming)
- Hover + barrel → Manual trigger control

**Core Integration API:**
```c
// Configure mapping behavior
spen_configure_mapping(ctx, SPEN_BARREL_TRIGGER, SPEN_HOVER_LIGHTGUN_TRACKING, 0.3f);

// Get mapped button state  
bool trigger = spen_get_mapped_button(ctx, RETRO_DEVICE_LIGHTGUN, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER);
```

### Testing Framework
Comprehensive test cases available in `spen_testing_plan.md` covering:
- Direct positioning accuracy
- Multi-touch gesture support
- Button mapping verification (barrel button, hover states)
- Hover-to-lightgun tracking functionality
- Legacy compatibility validation

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Build toolchain setup
- Code style guidelines  
- Testing procedures with RetroArch Android
- How to add S-Pen support to new cores

## Integration Strategy

**Preferred Path**: Per-core small patches calling the S-Pen adapter (Option A - lowest risk to upstream). This allows individual cores to adopt S-Pen support incrementally without requiring large RetroArch frontend changes, while still benefiting from RetroArch's existing hover guard and tool type detection improvements.

## License

This project maintains the same licenses as the respective upstream libretro cores. See individual core directories for specific license information.
