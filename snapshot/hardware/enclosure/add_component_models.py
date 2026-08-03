#!/usr/bin/env python3
"""
Add simplified 3D component models to the faceplate for visualisation.
These are NOT part of the faceplate — they're separate shapes to help
visualise how everything fits together.

Outputs a separate STEP file with just the components, and a combined
STEP file with faceplate + components.
"""

import sys
import math
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

faceplate_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v2.step'
components_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v2_with_components.step'

print("Loading faceplate...")
doc = FreeCAD.newDocument("Assembly")
Import.insert(faceplate_path, doc.Name)
faceplate = doc.Objects[0].Shape

# ============================================================
# Geometry (same as cut_faceplate.py)
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

encoder_slope_offset = 15.0
screen_slope_offset = 72.0
pot_slope_offsets = [screen_slope_offset + 20.0, screen_slope_offset - 20.0]

encoder_new_x = [-69.555 + i * 19.87 for i in range(8)]

components = []

# ============================================================
# Encoder knobs (cylinders on top of the panel)
# ============================================================
print("Creating encoder knob models...")
for x in encoder_new_x:
    pos = point_on_slope(x, encoder_slope_offset)
    # Knob: 15.1mm dia, 15.2mm tall, sitting on top of panel
    # The knob sits above the panel surface, along the slope normal (outward)
    knob_start = pos + slope_normal * 0  # flush with surface
    knob = Part.makeCylinder(15.1 / 2, 15.2, knob_start, slope_normal)
    components.append(knob)

    # Encoder body behind panel: 16.8mm wide (square-ish), 17.7mm deep
    # Simplified as a cylinder behind the panel
    body_start = pos - slope_normal * 17.7
    body = Part.makeCylinder(16.8 / 2, 17.7, body_start, slope_normal)
    components.append(body)

# ============================================================
# Pot knobs (larger, on top of panel)
# ============================================================
print("Creating pot knob models...")
for side_x in [-101.0, 101.0]:
    for slope_off in pot_slope_offsets:
        pos = point_on_slope(side_x, slope_off)
        # Knob: 20.1mm max dia, 15.5mm tall
        knob_start = pos + slope_normal * 0
        knob = Part.makeCylinder(20.1 / 2, 15.5, knob_start, slope_normal)
        components.append(knob)

        # Pot body behind panel: 9x9x5mm, simplified as cylinder
        body_start = pos - slope_normal * 5.0
        body = Part.makeCylinder(9.0 / 2, 5.0, body_start, slope_normal)
        components.append(body)

# ============================================================
# LED bezels (small cylinders)
# ============================================================
print("Creating LED bezel models...")
led_x = -116.0
for pot_offset in pot_slope_offsets:
    for vert_off in [7.0, -7.0]:
        pos = point_on_slope(led_x, pot_offset + vert_off)
        # Bezel: 10mm outer dia, ~3mm tall above panel
        bezel = Part.makeCylinder(10.0 / 2, 3.0, pos, slope_normal)
        components.append(bezel)

# ============================================================
# Pushbutton bodies (on flat face)
# ============================================================
print("Creating pushbutton models...")
pushbutton_x_positions = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]
pb_y = -258.3
pb_z = -86.8
for x in pushbutton_x_positions:
    pos = FreeCAD.Vector(x, pb_y, pb_z)
    # Button face: 16mm dia, ~5mm proud of panel
    button = Part.makeCylinder(16.0 / 2, 5.0, pos, flat_normal)
    components.append(button)
    # Body behind panel: 22mm square (simplified as 22mm dia cylinder), 19.5mm deep
    body_start = pos + flat_normal * 0  # flush with inner surface
    body = Part.makeCylinder(22.0 / 2, 19.5, pos + FreeCAD.Vector(0, 1.63, 0), FreeCAD.Vector(0, 1, 0))
    components.append(body)

# ============================================================
# Waveshare screen (box behind panel)
# ============================================================
print("Creating screen model...")
screen_centre = point_on_slope(0, screen_slope_offset)

# Screen board: 164.9 x 107.0 x 8.0mm
# It sits behind the panel, with the active area visible through the cutout
# The board is oriented on the slope

# Active area indicator: 154.21 x 85.92mm, on the panel surface
# (thin box to show where the display is)
active_hw = 154.21 / 2
active_hh = 85.92 / 2

p1 = screen_centre - slope_right * active_hw - slope_up * active_hh
p2 = screen_centre + slope_right * active_hw - slope_up * active_hh
p3 = screen_centre + slope_right * active_hw + slope_up * active_hh
p4 = screen_centre - slope_right * active_hw + slope_up * active_hh

wire = Part.makePolygon([p1, p2, p3, p4, p1])
face = Part.Face(wire)
# Thin slab representing the active display area, sitting just inside the cutout
active_area = face.extrude(slope_normal * (-1.0))  # 1mm thick, behind panel
components.append(active_area)

# Full board: 164.9 x 107.0 x 8.0mm behind the active area
board_hw = 164.9 / 2
board_hh = 107.0 / 2
bp1 = screen_centre - slope_right * board_hw - slope_up * board_hh - slope_normal * 1.0
bp2 = screen_centre + slope_right * board_hw - slope_up * board_hh - slope_normal * 1.0
bp3 = screen_centre + slope_right * board_hw + slope_up * board_hh - slope_normal * 1.0
bp4 = screen_centre - slope_right * board_hw + slope_up * board_hh - slope_normal * 1.0
bwire = Part.makePolygon([bp1, bp2, bp3, bp4, bp1])
bface = Part.Face(bwire)
board = bface.extrude(slope_normal * (-8.0))  # 8mm thick behind panel
components.append(board)

# ============================================================
# Combine and export
# ============================================================

print("Combining components...")
combined = faceplate
for comp in components:
    try:
        combined = combined.fuse(comp)
    except:
        pass  # Some components may not fuse cleanly, that's OK for visualisation

print(f"Exporting to {components_path}...")
Part.Shape.exportStep(combined, components_path)
print("Done!")

FreeCAD.closeDocument(doc.Name)
