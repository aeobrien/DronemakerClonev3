#!/usr/bin/env python3
"""
Faceplate v5 — accurate component models, correct orientations.

Coordinate system:
- X: left/right (centred)
- Y: 0 = front/top, negative = toward back. Outward from flat face = -Y
- Z: 0 = top, negative = down.

Flat face (pushbuttons): outer surface at Y=-258.32, normal = (0, -1, 0) pointing OUTWARD
Slope face (encoders etc): normal = (0, -0.5, 0.866) pointing OUTWARD
Panel thickness: 1.63mm

"Outward" = away from enclosure interior = toward the user
"Inward" = into the enclosure interior
"""

import sys
import math
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'
display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'
output_faceplate = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v5.step'
output_assembly = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v5_assembly.step'

print("Loading panel and display...")
doc = FreeCAD.newDocument("Main")
Import.insert(input_path, doc.Name)
panel = doc.Objects[0].Shape

doc2 = FreeCAD.newDocument("Display")
Import.insert(display_path, doc2.Name)
display_shape = doc2.Objects[0].Shape

# ============================================================
# Geometry
# ============================================================

# Outward-pointing normals
slope_outward = FreeCAD.Vector(0, -0.5, 0.866)
slope_outward.normalize()
slope_inward = slope_outward * -1

flat_outward = FreeCAD.Vector(0, -1, 0)  # pushbutton face outward
flat_inward = FreeCAD.Vector(0, 1, 0)    # into enclosure

slope_up = FreeCAD.Vector(0, 0.866, 0.5)
slope_up.normalize()
slope_right = FreeCAD.Vector(1, 0, 0)

cut_depth = 10.0
panel_thickness = 1.63

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
    box1 = face.extrude(axis * cut_depth)
    result = shape.cut(box1)
    box2 = face.extrude(axis * (-cut_depth))
    result = result.cut(box2)
    return result

# ============================================================
# Find exact face positions
# ============================================================

flat_outer_y = -258.32  # from previous analysis
flat_z_centre = -86.8

# ============================================================
# Layout (same as v3/v4)
# ============================================================

encoder_slope_offset = 10.0
screen_slope_offset = 80.0
pot_slope_offsets = [screen_slope_offset + 25.0, screen_slope_offset - 25.0]
encoder_new_x = [-69.555 + i * 19.87 for i in range(8)]
pushbutton_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]

# ============================================================
# Cut holes in faceplate
# ============================================================

print("Cutting faceplate holes...")
result = panel

# Screen cutout: 165 x 103mm
screen_centre = point_on_slope(0, screen_slope_offset)
result = cut_rectangle(result, screen_centre, 165.0, 103.0, slope_outward, slope_right, slope_up)

# Screen mounting: 4 x 3.2mm
screen_mounts = []
for v_off in [-115.0/2, 115.0/2]:
    for x in [-157.0/2, 157.0/2]:
        pos = point_on_slope(x, screen_slope_offset + v_off)
        screen_mounts.append(pos)
        result = cut_circle(result, pos, 3.2, slope_outward)

# Encoders: 8 x 9.5mm
encoder_positions = [point_on_slope(x, encoder_slope_offset) for x in encoder_new_x]
for pos in encoder_positions:
    result = cut_circle(result, pos, 9.5, slope_outward)

# Pots: 4 x 7.0mm
pot_positions = []
for side_x in [-101.0, 101.0]:
    for slope_off in pot_slope_offsets:
        pos = point_on_slope(side_x, slope_off)
        pot_positions.append(pos)
        result = cut_circle(result, pos, 7.0, slope_outward)

# LEDs: 4 x 8.0mm (TBD)
led_positions = []
for pot_off in pot_slope_offsets:
    for v in [7.0, -7.0]:
        pos = point_on_slope(-116.0, pot_off + v)
        led_positions.append(pos)
        result = cut_circle(result, pos, 8.0, slope_outward)

# Pushbuttons: 8 x 16mm
for x in pushbutton_x:
    pos = FreeCAD.Vector(x, flat_outer_y, flat_z_centre)
    result = cut_circle(result, pos, 16.0, flat_outward)

print(f"Exporting faceplate to {output_faceplate}...")
Part.Shape.exportStep(result, output_faceplate)

# ============================================================
# Component models (no knobs — just the actual hardware)
# ============================================================

print("Building component models...")
components = []

# --- ENCODERS (PEC16) ---
# Above panel (outward): bushing Ø9mm 7mm long, then shaft Ø6mm 12mm beyond
# Behind panel (inward): body 16x16mm square ~10.5mm deep, pins 2.7mm further

for pos in encoder_positions:
    # Bushing: Ø9, 7mm, extends outward from panel surface
    bushing = Part.makeCylinder(9.0/2, 7.0, pos, slope_outward)
    components.append(bushing)

    # Shaft: Ø6, 12mm beyond bushing (starts at end of bushing)
    shaft_start = pos + slope_outward * 7.0
    shaft = Part.makeCylinder(6.0/2, 12.0, shaft_start, slope_outward)
    components.append(shaft)

    # Body behind panel: 16mm wide, ~10.5mm deep
    # Starts at panel inner surface (1.63mm behind outer surface)
    body_inner_start = pos + slope_inward * panel_thickness
    body = Part.makeCylinder(16.0/2, 10.5, body_inner_start, slope_inward)
    components.append(body)

    # Pins: extend ~2.7mm further behind body (narrower, ~10mm envelope)
    pins_start = body_inner_start + slope_inward * 10.5
    pins = Part.makeCylinder(10.0/2, 2.7, pins_start, slope_inward)
    components.append(pins)

# --- PUSHBUTTONS ---
# Above panel (outward -Y): cap Ø18.5, ~1.5mm proud
# Behind panel (inward +Y): cylindrical body Ø16, 20mm long from panel rear face

for x in pushbutton_x:
    surface_pos = FreeCAD.Vector(x, flat_outer_y, flat_z_centre)

    # Cap: Ø18.5, extends 1.5mm outward from panel surface
    cap = Part.makeCylinder(18.5/2, 1.5, surface_pos, flat_outward)
    components.append(cap)

    # Body: Ø16, extends 20mm inward from panel inner surface
    inner_surface = surface_pos + flat_inward * panel_thickness
    body = Part.makeCylinder(16.0/2, 20.0, inner_surface, flat_inward)
    components.append(body)

# --- POTS ---
# Above panel: shaft Ø6, ~8mm
# Behind panel: body Ø9, 5mm

for pos in pot_positions:
    shaft = Part.makeCylinder(6.0/2, 8.0, pos, slope_outward)
    components.append(shaft)

    body_start = pos + slope_inward * panel_thickness
    body = Part.makeCylinder(9.0/2, 5.0, body_start, slope_inward)
    components.append(body)

# --- LED BEZELS ---
for pos in led_positions:
    flange = Part.makeCylinder(8.0/2, 2.0, pos, slope_outward)
    components.append(flange)

    body_start = pos + slope_inward * panel_thickness
    body_through = Part.makeCylinder(6.0/2, panel_thickness, pos, slope_inward)
    components.append(body_through)

# --- DISPLAY (actual Waveshare model) ---
print("Positioning display model...")

# Display native orientation:
#   X = horizontal (166mm)
#   Y = depth/normal. Screen surface at Y=-3.4. Back at Y=+10.7
#   Z = vertical (124mm)
# Screen faces -Y in native coords.

# We need screen to face slope_outward = (0, -0.5, 0.866)
# Rotate -60° around X axis transforms (0, -1, 0) to (0, -0.5, 0.866) ✓

rotated_display = display_shape.copy()
rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -60)
rotated_display.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), rotation)

# Find the rotated bounding box
rbb = rotated_display.BoundBox
rotated_centre = FreeCAD.Vector(
    (rbb.XMin + rbb.XMax) / 2,
    (rbb.YMin + rbb.YMax) / 2,
    (rbb.ZMin + rbb.ZMax) / 2
)

# Translate centre to screen_centre
translation = screen_centre - rotated_centre
rotated_display.translate(translation)

# The screen surface in native was at Y=-3.4, which is 7.05mm from the bbox centre
# in the -Y direction. After rotation, -Y maps to slope_outward.
# So the screen surface is at screen_centre + slope_outward * 7.05 after centering.
# We want the screen surface approximately AT the panel outer surface.
# So shift inward by 7.05mm to bring the screen face to screen_centre,
# then the body extends behind the panel.
rotated_display.translate(slope_inward * 7.05)

components.append(rotated_display)

rbb2 = rotated_display.BoundBox
print(f"Display bbox: X={rbb2.XMin:.1f} to {rbb2.XMax:.1f}")
print(f"              Y={rbb2.YMin:.1f} to {rbb2.YMax:.1f}")
print(f"              Z={rbb2.ZMin:.1f} to {rbb2.ZMax:.1f}")

# ============================================================
# Combine and export
# ============================================================

print("Combining assembly...")
combined = result
for comp in components:
    try:
        combined = combined.fuse(comp)
    except:
        pass

print(f"Exporting assembly to {output_assembly}...")
Part.Shape.exportStep(combined, output_assembly)

# Verify
print("\nVerifying faceplate...")
doc3 = FreeCAD.newDocument("V")
Import.insert(output_faceplate, doc3.Name)
vs = doc3.Objects[0].Shape
from collections import defaultdict
bd = defaultdict(int)
for f in vs.Faces:
    if isinstance(f.Surface, Part.Cylinder):
        bd[round(f.Surface.Radius*2, 1)] += 1
for d in sorted(bd.keys()):
    print(f"  {d}mm: {bd[d]}")

print("\nDone!")
FreeCAD.closeDocument(doc.Name)
FreeCAD.closeDocument(doc2.Name)
FreeCAD.closeDocument(doc3.Name)
