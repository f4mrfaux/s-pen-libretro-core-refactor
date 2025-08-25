# S-Pen User Configuration Guide

## 🎯 **Complete S-Pen Input Control**

Every S-Pen interaction is now fully mappable! You have complete control over:
- **Stylus tap** (screen contact) behavior
- **Barrel button** (side button) behavior  
- **Hover** (proximity) behavior

## 🖱️ **Mouse Mode Configuration**

### **Available Tap Actions:**
- `left_click` - Standard left mouse click
- `right_click` - Right mouse click (context menus)
- `middle_click` - Middle mouse click (scroll wheel)
- `disabled` - Tap does nothing

### **Available Barrel Actions:**
- `right_click` - Right mouse click (default)
- `left_click` - Left mouse click
- `middle_click` - Middle mouse click
- `disabled` - Barrel button does nothing

### **Example Configurations:**
**Traditional Setup:**
- Tap Action: `left_click`
- Barrel Action: `right_click`

**Reverse Setup (your preference!):**
- Tap Action: `right_click`  
- Barrel Action: `left_click`

**Drawing/Painting Optimized:**
- Tap Action: `left_click` (brush stroke)
- Barrel Action: `middle_click` (color picker)

## 🔫 **Lightgun Mode Configuration**

### **Available Tap Actions:**
- `trigger` - Fire weapon (default for quick shots)
- `reload` - Reload weapon
- `left_click` - Left mouse equivalent
- `right_click` - Right mouse equivalent
- `disabled` - Tap does nothing

### **Available Barrel Actions:**
- `right_click` - Right mouse equivalent (default)
- `trigger` - Fire weapon (for precision shooting)
- `reload` - Reload weapon
- `left_click` - Left mouse equivalent
- `disabled` - Barrel button does nothing

### **Hover Behavior:**
- `cursor` - Standard cursor movement
- `lightgun_tracking` - **Aim without shooting!** (precision mode)
- `disabled` - Hover ignored

### **Example Configurations:**

**Quick Draw Setup:**
- Tap Action: `trigger` (fast shots)
- Barrel Action: `reload` (tactical reload)
- Hover: `lightgun_tracking` (precision aiming)

**Precision Shooter Setup:**
- Tap Action: `reload` (reload on tap)
- Barrel Action: `trigger` (precise manual firing)
- Hover: `lightgun_tracking` (aim without shooting)

**MAME Arcade Style:**
- Tap Action: `trigger` (quick arcade shooting)
- Barrel Action: `reload` (coin-op reload button)
- Hover: `lightgun_tracking` (crosshair tracking)

## ⚙️ **Per-Core Configuration**

### **SNES9x Options:**
```
Quick Menu → Options → SNES Mouse Mode → "Absolute"
Quick Menu → Options → Light Gun Mode → "Absolute"
Quick Menu → Options → S-Pen Tap Action → [your choice]
Quick Menu → Options → S-Pen Barrel Action → [your choice]
Quick Menu → Options → S-Pen Hover Behavior → [your choice]
```

### **MAME 2016 Options:**
```
Quick Menu → Options → Mouse input mode → "absolute"
Quick Menu → Options → S-Pen Tap Action → [your choice]
Quick Menu → Options → S-Pen Barrel Action → [your choice]
```

### **SwanStation Options:**
```
Quick Menu → Options → GunCon Input Mode → "Pointer"
Quick Menu → Options → S-Pen Tap Action → [your choice]
Quick Menu → Options → S-Pen Barrel Action → [your choice]
```

## 💾 **Saving Your Settings**

### **Per-Game Configuration (Recommended):**
1. Configure S-Pen settings for your game
2. **Quick Menu → Overrides → Save Game Overrides**
3. Settings will persist for this specific ROM

### **Per-Core Configuration:**
1. Configure S-Pen settings
2. **Quick Menu → Overrides → Save Core Overrides**
3. Settings apply to all games using this core

## 🎮 **Real-World Examples**

### **SNATCHER (PSX) Setup:**
```
swanstation_Controller_GunconInputMode = "Pointer"
swanstation_spen_tap_action = "reload"
swanstation_spen_barrel_action = "trigger"
```
*Hover to aim precisely, barrel button to fire, tap screen to reload*

### **Area 51 (MAME) Setup:**
```
mame2016_mouse_mode = "absolute"
mame2016_spen_tap_action = "trigger"
mame2016_spen_barrel_action = "reload"
```
*Tap to fire quickly, barrel button to reload tactically*

### **Drawing Game (SNES) Setup:**
```
snes9x_mouse_mode = "Absolute"
snes9x_spen_tap_action = "left_click"
snes9x_spen_barrel_action = "right_click"
snes9x_spen_hover_behavior = "cursor"
```
*Natural drawing with left/right click control*

## 🔧 **Advanced Tips**

- **Pressure sensitivity** is configurable per-core
- **Hover distance** can be tuned for lightgun tracking
- **All settings persist** using RetroArch's standard override system
- **Mix and match** configurations for different game genres
- **Experiment freely** - every interaction is remappable!

Your S-Pen, your rules! 🎯