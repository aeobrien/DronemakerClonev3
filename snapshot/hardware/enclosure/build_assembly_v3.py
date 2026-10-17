#!/usr/bin/env python3
"""
Build assembly visualisation with:
- Accurate display model from Waveshare STEP file
- More accurate encoder and button shapes based on datasheet dimensions
- Faceplate v3

Button dimensions (from user):
- 1.5mm above panel
- 21.5mm from button top to terminal tips
- M16x1 thread (16mm dia body)
- Panel cutout: 16mm

Encoder dimensions (from user + datasheet):
- 15mm from thread base to shaft tip (above panel)
- Thread protrusion above panel: ~3.1mm (bushing)
- Shaft above bushing: ~11.9mm
- Below panel: 6.5mm body + 2.7mm terminals = 9.2mm
- Body width: 16.8mm
- Shaft diameter: 6mm

LED bezels:
- 8mm outer diameter (flange)
- Mounting hole size TBD (likely 6-7mm, not 8mm)
- For now, model as 8mm flange + 6mm body through panel
"""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

faceplate_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v3.step'
display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'
output_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v3_assembly.step'

# ============================================================
# Load faceplate
# ============================================================
print("Loading faceplate...")
doc = FreeCAD.newDocument("Assembly")
Import.insert(faceplate_path, doc.Name)
faceplate = doc.Objects[0].Shape

# ============================================================
# Load and analyze display model
# ============================================================
print("Loading display model...")
doc2 = FreeCAD.newDocument("Display")
Import.insert(display_path, doc2.Name)
display_shape = doc2.Objects[0].Shape

print(f"Display bounding box:")
bb = display_shape.BoundBox
print(f"  X: {bb.XMin:.1f} to {bb.XMax:.1f} ({bb.XLength:.1f}mm)")
print(f"  Y: {bb.YMin:.1f} to {bb.YMax:.1f} ({bb.YLength:.1f}mm)")
print(f"  Z: {bb.ZMin:.1f} to {bb.ZMax:.1f} ({bb.ZLength:.1f}mm)")

# ============================================================
# Geometry constants (same as cut_faceplate_v3.py)
# ============================================================
slope_normal = FreeCAD.Vector(0, -0.5, 0.866)
slope_normal.normalize()
slope_up = FreeCAD.Vector(0, 0.866, 0.5)
slope_up.normalize()
flat_normal = FreeCAD.Vector(0, -1, 0)
slope_right = FreeCAD.Vector(1, 0, 0)

def point_on_slope(x, slope_offset_mm):
    bottom_y = -258.0
    bottom_z = -74.0
    y = bottom_y + slope_up.y * slope_offset_mm
    z = bottom_z + slope_up.z * slope_offset_mm
    return FreeCAD.Vector(x, y, z)

encoder_slope_offset = 10.0
screen_slope_offset = 80.0
pot_slope_offsets = [screen_slope_offset + 25.0, screen_slope_offset - 25.0]
encoder_new_x = [-69.555 + i * 19.87 for i in range(8)]
pushbutton_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]
pb_y = -258.3
pb_z = -86.8

panel_thickness = 1.63

components = []

# ============================================================
# ENCODERS — more accurate model
# ============================================================
print("Creating encoder models...")
for x in encoder_new_x:
    pos = point_on_slope(x, encoder_slope_offset)

    # Above panel:
    # Threaded bushing: 9mm dia, 3.1mm tall above panel surface
    bushing = Part.makeCylinder(9.0/2, 3.1, pos, slope_normal)
    components.append(bushing)

    # Shaft: 6mm dia, extends from top of bushing up 11.9mm more
    shaft_start = pos + slope_normal * 3.1
    shaft = Part.makeCylinder(6.0/2, 11.9, shaft_start, slope_normal)
    components.append(shaft)

    # Knob on top of shaft: 15.1mm dia, 15.2mm tall
    # Knob sits on the shaft, its bottom at roughly the panel surface level
    # (the shaft goes up inside the knob)
    knob = Part.makeCylinder(15.1/2, 15.2, pos, slope_normal)
    components.append(knob)

    # Below panel:
    # Main body: 16.8 x 16.8mm (model as cylinder), 6.5mm deep
    body_start = pos - slope_normal * (panel_thickness + 6.5)
    body = Part.makeCylinder(16.8/2, 6.5, body_start, slope_normal)
    components.append(body)

    # Terminals: extend 2.7mm further below body
    # (smaller, model as 10mm dia cylinder)
    term_start = pos - slope_normal * (panel_thickness + 6.5 + 2.7)
    terminals = Part.makeCylinder(10.0/2, 2.7, term_start, slope_normal)
    components.append(terminals)

# ============================================================
# BUTTONS — more accurate model
# ============================================================
print("Creating button models...")
for x in pushbutton_x:
    pos = FreeCAD.Vector(x, pb_y, pb_z)

    # Above panel: button cap 1.5mm, 16mm dia
    cap = Part.makeCylinder(16.0/2, 1.5, pos, flat_normal)
    components.append(cap)

    # Below panel: M16 threaded body
    # Total 21.5mm from top to terminals, minus 1.5mm above = 20mm below top of panel
    # Panel is 1.63mm thick, so below panel inner surface: 20 - 1.63 = 18.37mm
    # Body is roughly 16mm dia (thread)
    body_start = pos + FreeCAD.Vector(0, panel_thickness, 0)  # inside the enclosure
    body = Part.makeCylinder(16.0/2, 18.37, body_start, FreeCAD.Vector(0, 1, 0))
    components.append(body)

    # Wider housing behind the thread: 22mm square (datasheet says 22x22mm body)
    # This extends from panel inner surface inward, roughly same depth
    # Model as 22mm dia cylinder
    housing = Part.makeCylinder(22.0/2, 18.37, body_start, FreeCAD.Vector(0, 1, 0))
    components.append(housing)

# ============================================================
# POTS — simple model
# ============================================================
print("Creating pot models...")
for side_x in [-101.0, 101.0]:
    for slope_off in pot_slope_offsets:
        pos = point_on_slope(side_x, slope_off)

        # Above panel: shaft 6mm dia, ~8mm tall (enough for knob grub screw)
        shaft = Part.makeCylinder(6.0/2, 8.0, pos, slope_normal)
        components.append(shaft)

        # Knob: 20.1mm dia, 15.5mm tall
        knob = Part.makeCylinder(20.1/2, 15.5, pos, slope_normal)
        components.append(knob)

        # Below panel: body 9x9x5mm
        body_start = pos - slope_normal * (panel_thickness + 5.0)
        body = Part.makeCylinder(9.0/2, 5.0, body_start, slope_normal)
        components.append(body)

# ============================================================
# LED BEZELS — model with flange and body
# ============================================================
print("Creating LED bezel models...")
led_x = -116.0
for pot_off in pot_slope_offsets:
    for v in [7.0, -7.0]:
        pos = point_on_slope(led_x, pot_off + v)

        # Flange (sits on panel surface): 8mm dia, 2mm tall
        flange = Part.makeCylinder(8.0/2, 2.0, pos, slope_normal)
        components.append(flange)

        # Body through panel: ~6mm dia (estimated), panel thickness deep
        body_start = pos - slope_normal * panel_thickness
        body = Part.makeCylinder(6.0/2, panel_thickness, body_start, slope_normal)
        components.append(body)

# ============================================================
# DISPLAY — position the actual Waveshare model
# ============================================================
print("Positioning display model...")

# The display needs to be:
# 1. Rotated to align with the 30-degree slope
# 2. Positioned so it sits in the screen cutout, flush with the panel

# First, figure out the display model's orientation
# The display STEP is probably oriented flat (XY plane)
# We need to rotate it to match our slope

# The screen centre on our slope:
screen_centre = point_on_slope(0, screen_slope_offset)

# The display model dimensions tell us its native orientation
# Let's check which axis is which
print(f"Display native orientation: {bb.XLength:.1f} x {bb.YLength:.1f} x {bb.ZLength:.1f}")
print(f"Expected: ~164.9 x 107.0 x 8.0mm")

# Create a placement that:
# 1. Moves the display centre to the screen centre position
# 2. Rotates it to align with the slope

# Native display centre
display_native_centre = FreeCAD.Vector(
    (bb.XMin + bb.XMax) / 2,
    (bb.YMin + bb.YMax) / 2,
    (bb.ZMin + bb.ZMax) / 2
)

# We need to build a rotation matrix
# The slope coordinate system:
#   X-axis: (1, 0, 0) — horizontal, same as global
#   "Y-axis" (up the slope): slope_up = (0, 0.866, 0.5)
#   Normal (out of slope): slope_normal = (0, -0.5, 0.866)

# The display's native coordinate system (assuming it's flat in XY):
#   Display X = our X (horizontal)
#   Display Y = up the display = our slope_up
#   Display Z = out of display = our slope_normal (but pointing inward for behind-panel mounting)

import math

# Rotation: rotate from Z-up to slope_normal direction
# The slope is 30 degrees from horizontal, tilted around the X axis
# So we rotate -60 degrees around X (since slope normal is 30° from vertical = 60° from horizontal Z)
angle_rad = math.radians(-60)  # 30-degree slope = 60-degree rotation from Z-up

# Build rotation matrix around X axis
cos_a = math.cos(angle_rad)
sin_a = math.sin(angle_rad)

# Rotate the display shape
rotated_display = display_shape.copy()
rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -60)  # -60 degrees around X
placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), rotation)
rotated_display.Placement = placement

# Now translate to position
# After rotation, find the new centre
rbb = rotated_display.BoundBox
rotated_centre = FreeCAD.Vector(
    (rbb.XMin + rbb.XMax) / 2,
    (rbb.YMin + rbb.YMax) / 2,
    (rbb.ZMin + rbb.ZMax) / 2
)

# Translate so the rotated centre aligns with screen_centre
translation = screen_centre - rotated_centre
rotated_display.translate(translation)

# Shift slightly behind the panel surface (the display face should be flush)
# The display's front face should align with the panel outer surface
# We may need to adjust along slope_normal
# For now, position it approximately
rotated_display.translate(slope_normal * (-2))  # shift 2mm behind panel

components.append(rotated_display)

rbb2 = rotated_display.BoundBox
print(f"Positioned display bbox: X={rbb2.XMin:.1f} to {rbb2.XMax:.1f}, Y={rbb2.YMin:.1f} to {rbb2.YMax:.1f}, Z={rbb2.ZMin:.1f} to {rbb2.ZMax:.1f}")

# ============================================================
# Combine and export
# ============================================================

print("\nCombining assembly...")
combined = faceplate
for comp in components:
    try:
        combined = combined.fuse(comp)
    except Exception as e:
        pass  # Some components may not fuse cleanly

print(f"Exporting to {output_path}...")
Part.Shape.exportStep(combined, output_path)
print("Done!")

FreeCAD.closeDocument(doc.Name)
FreeCAD.closeDocument(doc2.Name)
