# S-Pen Implementation Testing Plan

## 🎯 **Test Scenarios Overview**

### **Core Testing Matrix**

| Core | Device Type | Games/Use Cases | Settings Required | Expected Behavior |
|------|-------------|-----------------|-------------------|-------------------|
| **SNES9x** | SNES Mouse | Mario Paint | `snes9x_mouse_mode = "Absolute"` | Direct stylus drawing |
| **SNES9x** | Super Scope | Duck Hunt, etc. | `snes9x_lightgun_mode = "Absolute"` | Direct aim targeting |
| **Genesis Plus GX** | Light Gun | Area 51, Lethal Enforcers | `genesis_plus_gx_gun_input = "touchscreen"` | Direct lightgun aiming |
| **Genesis Plus GX** | Sega Pico | Educational games | Set to Pico device | Stylus drawing/pointing |
| **DeSmuME** | NDS Touchscreen | Any DS game | Default settings | Lower screen touch |
| **MelonDS** | NDS Touchscreen | Any DS game | Default settings | Lower screen touch |

---

## 🧪 **Detailed Test Cases**

### **Test 1: SNES9x Mouse Mode (Mario Paint)**

**Setup:**
- Core: SNES9x
- ROM: Mario Paint (USA)
- Controller 1: Set to "SNES Mouse"
- Core Option: `snes9x_mouse_mode = "Absolute"`

**Test Steps:**
1. Launch Mario Paint
2. Enter drawing mode
3. **Test A: Direct positioning**
   - Touch stylus to different screen areas
   - Verify cursor appears exactly where stylus touches
   - No cursor jumping or accumulated offset errors
   
4. **Test B: Drawing strokes**
   - Draw continuous lines with stylus
   - Verify smooth line following stylus movement
   - Test pressure sensitivity (if supported by RetroArch input driver)
   
5. **Test C: Button mapping**
   - Stylus tip contact = left mouse button
   - Side button (if available) = right mouse button
   - Verify paint tool activates correctly

**Expected Results:**
- ✅ Immediate cursor positioning where stylus touches
- ✅ Smooth drawing without lag or jitter
- ✅ No phantom cursor movements after stylus lift
- ✅ Button presses register correctly

---

### **Test 2: SNES9x Lightgun Mode (Super Scope)**

**Setup:**
- Core: SNES9x  
- ROM: Super Scope 6 compatible game
- Controller 2: Set to "SuperScope"
- Core Option: `snes9x_lightgun_mode = "Absolute"`

**Test Steps:**
1. Launch compatible game
2. **Test A: Targeting accuracy**
   - Point stylus at on-screen targets
   - Fire using stylus tip contact
   - Verify hits register at stylus position
   
3. **Test B: Off-screen detection**
   - Move stylus completely off screen
   - Verify game recognizes off-screen state
   
4. **Test C: Cursor visibility**
   - Check if crosshair follows stylus accurately
   - Test various screen positions (edges, center)

**Expected Results:**
- ✅ Accurate hit detection where stylus points
- ✅ Proper off-screen reload mechanics
- ✅ Crosshair tracks stylus smoothly

---

### **Test 3: Genesis Plus GX Lightgun Mode (Area 51)**

**Setup:**
- Core: Genesis Plus GX
- ROM: Area 51 (or similar lightgun game)
- Controller: Set to "Light Gun" 
- Core Option: `genesis_plus_gx_gun_input = "touchscreen"`

**Test Steps:**
1. Launch Area 51
2. **Test A: Direct aiming**
   - Point stylus at enemies/targets
   - Verify shots hit where stylus points
   
3. **Test B: Multi-touch gestures**
   - Test 2-finger touch for secondary action
   - Test 3-finger touch for START
   - Test 4-finger touch for additional button
   
4. **Test C: Movement precision**
   - Test rapid stylus movements
   - Verify tracking accuracy across screen
   - Check for any dead zones or calibration issues

**Expected Results:**
- ✅ Perfect aim-and-shoot accuracy
- ✅ Multi-touch gestures work as designed
- ✅ Responsive tracking without lag

---

### **Test 4: Genesis Plus GX Sega Pico Mode**

**Setup:**
- Core: Genesis Plus GX
- ROM: Sega Pico educational game
- Controller: Set to "Sega Pico"

**Test Steps:**
1. Launch Pico game
2. **Test A: Drawing/pointing accuracy**
   - Use stylus on interactive elements
   - Verify precise coordinate mapping
   
3. **Test B: Page turning**
   - Test mouse wheel equivalent actions
   - Verify proper page navigation

**Expected Results:**
- ✅ Accurate stylus positioning for educational activities
- ✅ Smooth interaction with Pico-specific features

---

### **Test 5: NDS Touchscreen (DeSmuME/MelonDS)**

**Setup:**
- Core: DeSmuME or MelonDS
- ROM: Any DS game with touch controls
- Default touchscreen settings

**Test Steps:**
1. Launch DS game with touch features
2. **Test A: Lower screen touch accuracy**
   - Touch various areas of lower screen
   - Verify touches register at correct positions
   
3. **Test B: Multi-point touch (if supported)**
   - Test games requiring multiple touch points
   - Verify gesture recognition

**Expected Results:**
- ✅ Accurate touch detection on lower screen
- ✅ No interference with upper screen display
- ✅ Proper touch coordinate mapping

---

## 🔧 **RetroArch Integration Testing**

### **Test 6: S-Pen Input Driver Features**

**Test Steps:**
1. **Hover Guard Testing**
   - Hover stylus near screen without touching
   - Move finger/touch screen immediately after
   - Verify phantom touches are blocked
   
2. **Tool Type Detection** 
   - Use stylus vs finger
   - Verify different input handling
   
3. **Pressure Sensitivity**
   - Test light vs firm stylus contact
   - Verify proper contact detection
   
4. **Side Button Support**
   - Test stylus side button (if available)
   - Verify right-click functionality

**Expected Results:**
- ✅ No phantom touches after stylus hover
- ✅ Proper tool type discrimination
- ✅ Reliable contact detection
- ✅ Side button works as right-click

---

## ⚠️ **Known Issues to Test For**

1. **Coordinate Offset Errors**
   - Check for any cursor positioning that doesn't match stylus position
   - Test screen edge accuracy
   
2. **Input Lag**
   - Verify minimal delay between stylus contact and response
   - Test during high CPU usage scenarios
   
3. **Calibration Drift**
   - Long-term testing for any coordinate drift
   - Test after device rotation (if applicable)
   
4. **Multi-Touch Conflicts**
   - Ensure stylus input doesn't conflict with finger touches
   - Test accidental palm rejection

---

## 📊 **Success Criteria**

For each core implementation to be considered successful:

- ✅ **Accuracy**: Cursor/targeting appears exactly where stylus touches
- ✅ **Responsiveness**: Minimal input lag (<50ms perceived)
- ✅ **Stability**: No crashes or input system failures
- ✅ **Compatibility**: Works with existing games without modification
- ✅ **Integration**: Proper integration with RetroArch S-Pen input driver
- ✅ **Fallback**: Graceful fallback to relative mouse when pointer disabled

---

## 🎮 **Recommended Test Games**

### **SNES9x Testing:**
- Mario Paint (Mouse absolute mode)
- Super Scope 6 (Lightgun absolute mode)
- Any Super Famicom Mouse compatible game

### **Genesis Plus GX Testing:**
- Area 51 (Menacer lightgun)
- Lethal Enforcers (Justifier lightgun) 
- Educational Sega Pico titles
- Drawing tablet compatible games

### **NDS Testing:**
- Brain Age (stylus writing)
- Elite Beat Agents (rhythm touch)
- Art Academy (drawing application)
- Any game with extensive touch controls

---

This comprehensive testing plan ensures our S-Pen implementations work correctly across all supported cores and use cases.