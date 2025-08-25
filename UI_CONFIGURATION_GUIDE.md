# 🎯 Complete S-Pen UI Configuration Guide
## **Zero Code Required - Pure Menu Interface**

All S-Pen configuration happens through RetroArch's familiar menu system. No configuration files, no code editing!

---

## 📱 **Step-by-Step Configuration**

### **1. Load Your Game**
```
Main Menu → Load Core → [Select your core: SNES9x/MAME2016/SwanStation]
Main Menu → Load Content → [Select your ROM/game]
```

### **2. Access S-Pen Options**
```
Quick Menu → Options
```
**You'll see familiar RetroArch option categories:**
- 📺 Video
- 🔊 Audio  
- **🎮 Input** ← S-Pen options are here!
- ⚡ Performance

### **3. Configure S-Pen Behavior**
**Navigate to Input section, you'll see:**

#### **Core Mode Selection:**
```
□ SNES Mouse Mode          [Relative ▼] → Change to [Absolute ▼]
□ Light Gun Mode          [Lightgun ▼] → Change to [Absolute ▼]  
□ GunCon Input Mode       [Light Gun ▼] → Change to [Pointer ▼]
```

#### **S-Pen Specific Options:**
```
🖊️ S-Pen Tap Action        [Left Click ▼]
   Options: Left Click / Right Click / Middle Click / Trigger/Shoot / Reload / Disabled

🖊️ S-Pen Barrel Action     [Right Click ▼]  
   Options: Right Click / Left Click / Middle Click / Trigger/Shoot / Reload / Disabled

🖊️ S-Pen Hover Behavior    [Cursor ▼]
   Options: Cursor Only / Lightgun Tracking / Disabled
```

---

## 🎮 **Per-Core UI Locations**

### **SNES9x Menu Path:**
```
Quick Menu → Options → [Input Category]
├── SNES Mouse Mode: [Relative/Absolute]
├── Light Gun Mode: [Lightgun/Absolute]  
├── S-Pen Tap Action: [6 choices]
├── S-Pen Barrel Action: [6 choices]
└── S-Pen Hover Behavior: [3 choices]
```

### **MAME 2016 Menu Path:**
```
Quick Menu → Options → [All Options]
├── Mouse input mode: [relative/absolute]
├── S-Pen Tap Action: [6 choices]  
└── S-Pen Barrel Action: [6 choices]
```

### **SwanStation Menu Path:**
```
Quick Menu → Options → [Port Category]
├── GunCon Input Mode: [Light Gun/Pointer]
├── S-Pen Tap Action: [5 choices]
└── S-Pen Barrel Action: [5 choices]
```

---

## 💾 **Saving Your Settings (Pure UI)**

### **Per-Game Settings (Recommended):**
```
Quick Menu → Overrides → Save Game Overrides
```
*Settings save automatically for this specific game*

### **Per-Core Settings:**
```
Quick Menu → Overrides → Save Core Overrides  
```
*Settings apply to all games using this core*

### **Global Settings:**
```
Quick Menu → Overrides → Save Content Directory Overrides
```
*Settings apply to all games in this folder*

---

## 🖱️ **UI Configuration Examples**

### **Example 1: Drawing Game Setup**
**Navigation:** `Quick Menu → Options → Input`
```
□ SNES Mouse Mode: [Absolute]
🖊️ S-Pen Tap Action: [Left Click]
🖊️ S-Pen Barrel Action: [Right Click]  
🖊️ S-Pen Hover Behavior: [Cursor Only]
```
**Save:** `Quick Menu → Overrides → Save Game Overrides`

### **Example 2: Lightgun Game Setup**  
**Navigation:** `Quick Menu → Options → Input`
```
□ Light Gun Mode: [Absolute]
🖊️ S-Pen Tap Action: [Trigger/Shoot]
🖊️ S-Pen Barrel Action: [Reload]
🖊️ S-Pen Hover Behavior: [Lightgun Tracking]
```
**Save:** `Quick Menu → Overrides → Save Game Overrides`

### **Example 3: Custom Setup** 
**Navigation:** `Quick Menu → Options → Port`
```
□ GunCon Input Mode: [Pointer]
🖊️ S-Pen Tap Action: [Reload]     ← User preference!
🖊️ S-Pen Barrel Action: [Trigger/Shoot]  ← User preference!
```
**Save:** `Quick Menu → Overrides → Save Game Overrides`

---

## 📋 **All Available UI Options**

### **S-Pen Tap Action Dropdown:**
- ✅ Left Click
- ✅ Right Click  
- ✅ Middle Click
- ✅ Trigger/Shoot
- ✅ Reload
- ✅ Disabled

### **S-Pen Barrel Action Dropdown:**
- ✅ Right Click *(default)*
- ✅ Left Click
- ✅ Middle Click
- ✅ Trigger/Shoot
- ✅ Reload
- ✅ Disabled

### **S-Pen Hover Behavior Dropdown:**
- ✅ Cursor Only *(default)*
- ✅ Lightgun Tracking *(aim without shooting!)*
- ✅ Disabled

---

## 🎯 **Real-World UI Workflows**

### **Scenario: Setting Up SNATCHER**
1. **Load Game:** `Load Core: SwanStation → Load Content: snatcher.cue`
2. **Configure:** `Quick Menu → Options → Port`
3. **Set Mode:** `GunCon Input Mode → [Pointer]`
4. **Customize:** `S-Pen Barrel Action → [Trigger/Shoot]`
5. **Save:** `Quick Menu → Overrides → Save Game Overrides`
6. **Done!** SNATCHER now uses barrel button to shoot!

### **Scenario: Your Custom Preference**
1. **Any Game/Core**
2. **Configure:** `Quick Menu → Options → [Input/Port]`
3. **Set Your Way:** 
   - `S-Pen Tap Action → [Right Click]` *(your preference!)*
   - `S-Pen Barrel Action → [Left Click]` *(your preference!)*
4. **Save:** Your choice of override level
5. **Perfect!** Stylus works exactly how YOU want!

---

## ✨ **Key Benefits**

- 🚫 **Zero Code Editing** - Pure menu interface
- 💾 **Automatic Saving** - RetroArch handles all file management  
- 🎮 **Per-Game Memory** - Each game remembers your preferences
- 🔄 **Easy Changes** - Modify anytime through menus
- 📱 **Touch Friendly** - Works great on Android RetroArch
- 🎯 **Your Rules** - Every interaction fully customizable

**Your S-Pen, configured your way, through familiar RetroArch menus!** 🎉