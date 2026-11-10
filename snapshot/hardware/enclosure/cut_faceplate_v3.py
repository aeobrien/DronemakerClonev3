#!/usr/bin/env python3
"""
Cut all component holes into clean panel — v3.
Changes from v2:
- Screen cutout: full board size (165 x 107mm) for flush mounting
- Screen moved up the slope to clear encoder row
- Encoder alignment: knob edges align with active display area (154.21mm)
"""

import sys
import math
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'
output_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v3.step'
output_with_components = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v3_with_components.step'

print("Loading clean panel...")
doc = FreeCAD.newDocument("Faceplate")
Import.insert(input_path, doc.Name)
panel = doc.Objects[0].Shape

# ============================================================
# Geometry constants
# ============================================================

slope_normal = FreeCAD.Vector(0, -0.5, 0.866)
slope_normal.normalize()
slope_up = FreeCAD.Vector(0, 0.866, 0.5)
slope_up.normalize()
flat_normal = FreeCAD.Vector(0, -1, 0)
slope_right = FreeCAD.Vector(1, 0, 0)

cut_depth = 10.0
slope_length = 144.0  # mm along the slope surface

def point_on_slope(x, slope_offset_mm):
    bottom_y = -258.0
    bottom_z = -74.0
    y = bottom_y + slope_up.y * slope_offset_mm
    z = bottom_z + slope_up.z * slope_offset_mm
    return FreeCAD.Vector(x, y, z)

def cut_circle(shape, pos, diameter, axis):
    r = diameter / 2.0
    start = pos - axis * cut_depth
    cyl = Part.makeCylinder(r, cut_depth * 2, start, axis)
    return shape.cut(cyl)

def cut_rectangle(shape, centre, width, height, axis, right_dir, up_dir):
    hw = width / 2.0
    hh = height / 2.0
    p1 = centre - right_dir * hw - up_dir * hh
    p2 = centre + right_dir * hw - up_dir * hh
    p3 = centre + right_dir * hw + up_dir * hh
    p4 = centre - right_dir * hw + up_dir * hh
    wire = Part.makePolygon([p1, p2, p3, p4, p1])
    face = Part.Face(wire)
    box = face.extrude(axis * cut_depth)
    result = shape.cut(box)
    box2 = face.extrude(axis * (-cut_depth))
    result = result.cut(box2)
    return result

# ============================================================
# Layout calculations
# ============================================================

# Slope runs 0 (bottom, near pushbuttons) to ~144mm (top, near back)

# ENCODERS: 10mm up from slope bottom
# (moved down slightly from v2's 15mm to give more clearance to screen mounts)
encoder_slope_offset = 10.0

# SCREEN: 80mm up from slope bottom (moved up from 72mm to clear encoders)
screen_slope_offset = 80.0

# Check clearances:
# Bottom screen mount: 80 - 57.5 = 22.5mm from slope bottom
# Encoder row: 10mm from slope bottom
# Encoder hole top edge: 10 + 4.75 = 14.75mm
# Screen mount hole bottom edge: 22.5 - 1.6 = 20.9mm
# Clearance between encoder hole and screen mount: 20.9 - 14.75 = 6.15mm ✓

# Top screen mount: 80 + 57.5 = 137.5mm (6.5mm from top of slope) ✓

# Bottom of screen cutout (board): 80 - 107/2 = 80 - 53.5 = 26.5mm from slope bottom
# This is well above the encoder row at 10mm ✓

print(f"Layout check:")
print(f"  Encoder row: {encoder_slope_offset}mm from slope bottom")
print(f"  Encoder hole top edge: {encoder_slope_offset + 4.75}mm")
print(f"  Screen centre: {screen_slope_offset}mm from slope bottom")
print(f"  Screen cutout bottom edge: {screen_slope_offset - 107/2}mm")
print(f"  Bottom screen mount: {screen_slope_offset - 115/2}mm")
print(f"  Top screen mount: {screen_slope_offset + 115/2}mm")
print(f"  Clearance encoder-to-mount: {(screen_slope_offset - 115/2) - (encoder_slope_offset + 4.75):.1f}mm")
print(f"  Top mount to slope edge: {slope_length - (screen_slope_offset + 115/2):.1f}mm")

# ENCODER POSITIONS: knob edges align with 154.21mm active display area
# Leftmost knob left edge = left edge of active area
# Active area left edge at X = -154.21/2 = -77.105
# Knob radius = 15.1/2 = 7.55
# Leftmost encoder centre: -77.105 + 7.55 = -69.555
encoder_new_x = [-69.555 + i * 19.87 for i in range(8)]

# POTS: flanking the screen, vertically centred on screen
# ±25mm from screen centre along slope (gives more separation than v2's ±20mm)
pot_slope_offsets = [screen_slope_offset + 25.0, screen_slope_offset - 25.0]

# LED BEZELS: stacked vertically to left of each left pot
led_x = -116.0
led_vert_offset = 7.0  # ±7mm from pot centre along slope

# PUSHBUTTONS: 8 x 16mm, 25mm spacing on flat bottom face
pushbutton_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]
pb_y = -258.3
pb_z = -86.8

# ============================================================
# Compute all positions
# ============================================================

print("\n=== POSITIONS ===")

# Encoders
encoder_positions = [point_on_slope(x, encoder_slope_offset) for x in encoder_new_x]
print(f"\nEncoders (9.5mm, 19.87mm c-to-c):")
for i, pos in enumerate(encoder_positions):
    print(f"  E{i+1}: ({pos.x:.2f}, {pos.y:.2f}, {pos.z:.2f})")

# Screen centre
screen_centre = point_on_slope(0, screen_slope_offset)
print(f"\nScreen cutout (165 x 107mm, flush mount): centre ({screen_centre.x:.1f}, {screen_centre.y:.1f}, {screen_centre.z:.1f})")

# Screen mounting holes (157 x 115mm pattern)
screen_mounts = []
for v_off in [-115.0/2, 115.0/2]:
    for x in [-157.0/2, 157.0/2]:
        pos = point_on_slope(x, screen_slope_offset + v_off)
        screen_mounts.append(pos)
print(f"\nScreen mounting holes (3.2mm):")
for pos in screen_mounts:
    print(f"  ({pos.x:.2f}, {pos.y:.2f}, {pos.z:.2f})")

# Pots
pot_positions = []
for side_x in [-101.0, 101.0]:
    for slope_off in pot_slope_offsets:
        pos = point_on_slope(side_x, slope_off)
        pot_positions.append(pos)
print(f"\nPots (7.0mm):")
for pos in pot_positions:
    print(f"  ({pos.x:.2f}, {pos.y:.2f}, {pos.z:.2f})")

# LEDs
led_positions = []
for pot_off in pot_slope_offsets:
    for v in [led_vert_offset, -led_vert_offset]:
        pos = point_on_slope(led_x, pot_off + v)
        led_positions.append(pos)
print(f"\nLED bezels (8.0mm):")
for pos in led_positions:
    print(f"  ({pos.x:.2f}, {pos.y:.2f}, {pos.z:.2f})")

# ============================================================
# Cut holes
# ============================================================

print("\n=== CUTTING ===")
result = panel

# 1. Screen cutout — full board size for flush mounting
# Board is 164.9 x 107.0mm, round up slightly for clearance
print("Screen cutout (165 x 107mm)...")
result = cut_rectangle(result, screen_centre, 165.0, 107.0, slope_normal, slope_right, slope_up)

# 2. Screen mounting holes
print("Screen mounting holes (4 x 3.2mm)...")
for pos in screen_mounts:
    result = cut_circle(result, pos, 3.2, slope_normal)

# 3. Encoders
print("Encoder holes (8 x 9.5mm)...")
for pos in encoder_positions:
    result = cut_circle(result, pos, 9.5, slope_normal)

# 4. Pots
print("Pot holes (4 x 7.0mm)...")
for pos in pot_positions:
    result = cut_circle(result, pos, 7.0, slope_normal)

# 5. LED bezels
print("LED holes (4 x 8.0mm)...")
for pos in led_positions:
    result = cut_circle(result, pos, 8.0, slope_normal)

# 6. Pushbuttons
print("Pushbutton holes (8 x 16.0mm)...")
for x in pushbutton_x:
    pos = FreeCAD.Vector(x, pb_y, pb_z)
    result = cut_circle(result, pos, 16.0, flat_normal)

# ============================================================
# Export faceplate
# ============================================================

print(f"\nExporting faceplate to {output_path}...")
Part.Shape.exportStep(result, output_path)

# ============================================================
# Add component visualisation models
# ============================================================

print("Adding component models for visualisation...")
components = []

# Encoder knobs + bodies
for pos in encoder_positions:
    knob = Part.makeCylinder(15.1/2, 15.2, pos, slope_normal)
    components.append(knob)
    body = Part.makeCylinder(16.8/2, 17.7, pos - slope_normal * 17.7, slope_normal)
    components.append(body)

# Pot knobs
for pos in pot_positions:
    knob = Part.makeCylinder(20.1/2, 15.5, pos, slope_normal)
    components.append(knob)

# LED bezels
for pos in led_positions:
    bezel = Part.makeCylinder(10.0/2, 3.0, pos, slope_normal)
    components.append(bezel)

# Pushbuttons
for x in pushbutton_x:
    pos = FreeCAD.Vector(x, pb_y, pb_z)
    button = Part.makeCylinder(16.0/2, 5.0, pos, flat_normal)
    components.append(button)
    body = Part.makeCylinder(22.0/2, 19.5, pos + FreeCAD.Vector(0, 1.63, 0), FreeCAD.Vector(0, 1, 0))
    components.append(body)

# Screen — active area (thin slab at panel surface)
active_hw = 154.21 / 2
active_hh = 85.92 / 2
ap1 = screen_centre - slope_right * active_hw - slope_up * active_hh
ap2 = screen_centre + slope_right * active_hw - slope_up * active_hh
ap3 = screen_centre + slope_right * active_hw + slope_up * active_hh
ap4 = screen_centre - slope_right * active_hw + slope_up * active_hh
awire = Part.makePolygon([ap1, ap2, ap3, ap4, ap1])
aface = Part.Face(awire)
active = aface.extrude(slope_normal * 0.5)  # thin slab proud of surface
components.append(active)

# Screen board (behind panel)
board_hw = 164.9 / 2
board_hh = 107.0 / 2
bp1 = screen_centre - slope_right * board_hw - slope_up * board_hh
bp2 = screen_centre + slope_right * board_hw - slope_up * board_hh
bp3 = screen_centre + slope_right * board_hw + slope_up * board_hh
bp4 = screen_centre - slope_right * board_hw + slope_up * board_hh
bwire = Part.makePolygon([bp1, bp2, bp3, bp4, bp1])
bface = Part.Face(bwire)
board = bface.extrude(slope_normal * (-8.0))
components.append(board)

# Combine
print("Fusing components with faceplate...")
combined = result
for comp in components:
    try:
        combined = combined.fuse(comp)
    except:
        pass

print(f"Exporting with components to {output_with_components}...")
Part.Shape.exportStep(combined, output_with_components)

# ============================================================
# Verify
# ============================================================

print("\nVerifying faceplate...")
doc2 = FreeCAD.newDocument("Verify")
Import.insert(output_path, doc2.Name)
vshape = doc2.Objects[0].Shape

from collections import defaultdict
by_d = defaultdict(int)
for face in vshape.Faces:
    if isinstance(face.Surface, Part.Cylinder):
        d = round(face.Surface.Radius * 2, 1)
        by_d[d] += 1

print(f"Output: {len(vshape.Faces)} faces, {len(vshape.Edges)} edges")
print("Hole sizes:")
for d in sorted(by_d.keys()):
    print(f"  {d}mm: {by_d[d]} faces")

print("\n=== SUMMARY ===")
print(f"Encoders:     8 x 9.5mm  @ 19.87mm c-to-c, knobs align with 154.21mm active area")
print(f"Screen cut:   165 x 107mm (full board, flush mount)")
print(f"Screen mount: 4 x 3.2mm  @ 157.0 x 115.0mm")
print(f"Pots:         4 x 7.0mm  @ X=±101, ±25mm from screen centre")
print(f"LED bezels:   4 x 8.0mm  @ X=-116, stacked ±7mm from each left pot")
print(f"Pushbuttons:  8 x 16.0mm @ 25mm spacing")
print(f"\nClearance encoder-to-screen-mount: {(screen_slope_offset - 115/2) - (encoder_slope_offset + 4.75):.1f}mm")

FreeCAD.closeDocument(doc.Name)
FreeCAD.closeDocument(doc2.Name)
