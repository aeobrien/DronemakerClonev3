#!/usr/bin/env python3
"""
Rebuild Faceplate_v2.step from scratch using the original panel geometry
with corrected hole sizes and positions.

Approach: Load original, extract the base panel (no holes), then cut all
holes fresh with correct dimensions.
"""

import sys
import math
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v1.step'
output_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v2.step'

print("Loading original STEP file...")
doc = FreeCAD.newDocument("Faceplate")
Import.insert(input_path, doc.Name)
original = doc.Objects[0].Shape

# ============================================================
# Panel geometry constants (from analysis)
# ============================================================

# Angled face normal (30-degree slope)
angled_normal = FreeCAD.Vector(0, -0.5, 0.866)
angled_normal.normalize()

# Flat bottom face normal (pushbuttons)
flat_normal = FreeCAD.Vector(0, -1, 0)

# Z-axis normal (top flat panel, for Hammond holes)
z_normal = FreeCAD.Vector(0, 0, -1)

# Generous cut-through length
cut_depth = 20.0

# ============================================================
# STEP 1: Fill ALL existing component holes in the original
# (keep only Hammond enclosure holes: 4.8mm, 2.79mm, hinge features)
# ============================================================

print("Filling all component holes from original...")

# All holes to fill (from analysis):
# - 8 x 16mm pushbuttons on flat face
# - 8 x 7mm encoders on angled face
# - 4 x 7mm pots on angled face
# - 4 x 3mm screen mounting on angled face
# - Screen cutout (rectangular)

# Pushbutton positions (flat face, Y-axis)
pushbutton_old = [
    (-87.5, -122.5, 16.85),
    (-62.5, -122.5, 16.85),
    (-37.5, -122.5, 16.85),
    (-12.5, -122.5, 16.85),
    (12.5, -122.5, 16.85),
    (37.5, -122.5, 16.85),
    (62.5, -122.5, 16.85),
    (87.5, -122.5, 16.85),
]

# Encoder positions (angled face) - OLD positions
encoder_old = [
    (-64.75, -115.33, 30.3),
    (-46.25, -115.33, 30.3),
    (-27.75, -115.33, 30.3),
    (-9.25, -115.33, 30.3),
    (9.25, -115.33, 30.3),
    (27.75, -115.33, 30.3),
    (46.25, -115.33, 30.3),
    (64.75, -115.33, 30.3),
]

# Pot positions (angled face)
pot_positions = [
    (-101.0, -74.63, 53.8),   # Left top (XLR/TRS gain)
    (-101.0, -39.99, 73.8),   # Left bottom (instrument gain)
    (101.0, -74.63, 53.8),    # Right top (master volume)
    (101.0, -39.99, 73.8),    # Right bottom (headphone volume)
]

# Screen mounting holes (angled face)
screen_mount_old = [
    (-78.45, -107.1, 35.05),
    (78.45, -107.1, 35.05),
    (-78.45, -7.51, 92.55),
    (78.45, -7.51, 92.55),
]

def make_fill_cylinder(x, y, z, diameter, axis, length=cut_depth):
    """Create a cylinder to fill an existing hole."""
    r = (diameter / 2.0) - 0.005  # Tiny undersize for clean boolean
    centre = FreeCAD.Vector(x, y, z)
    start = centre - axis * (length / 2)
    return Part.makeCylinder(r, length, start, axis)

def make_cut_cylinder(x, y, z, diameter, axis, length=cut_depth):
    """Create a cylinder to cut a new hole."""
    r = diameter / 2.0
    centre = FreeCAD.Vector(x, y, z)
    start = centre - axis * (length / 2)
    return Part.makeCylinder(r, length, start, axis)

# Build fill shapes
fills = []

# Fill old pushbutton holes
for pos in pushbutton_old:
    fills.append(make_fill_cylinder(pos[0], pos[1], pos[2], 16.0, flat_normal))

# Fill old encoder holes
for pos in encoder_old:
    fills.append(make_fill_cylinder(pos[0], pos[1], pos[2], 7.0, angled_normal))

# Fill old pot holes
for pos in pot_positions:
    fills.append(make_fill_cylinder(pos[0], pos[1], pos[2], 7.0, angled_normal))

# Fill old screen mounting holes
for pos in screen_mount_old:
    fills.append(make_fill_cylinder(pos[0], pos[1], pos[2], 3.0, angled_normal))

# Fill screen cutout - need a box on the angled surface
# Screen cutout sides are at X = ±75, on the angled face
# From analysis: Face 54 at X=-75, Face 57 at X=75 (vertical sides of cutout)
# Face 55 and 56 are the top/bottom edges on the slope
# The cutout spans from roughly Y=-107.4 to Y=-13.9 on the slope
# We'll create a box to fill it
screen_fill_width = 150.0  # matches current cutout
screen_fill_height = 110.0  # generous to cover full cutout on slope
screen_fill_depth = 5.0  # thicker than panel

# The angled surface centre for the screen area:
# midpoint of the screen cutout, on the angled face
screen_centre_y = (-107.44 + -13.9) / 2  # = -60.67
screen_centre_z = (42.63 + 96.62) / 2    # = 69.625

# Create fill box aligned to the angled surface
# We need to create a box and rotate it to match the 30-degree slope
screen_fill = Part.makeBox(screen_fill_width, screen_fill_depth, screen_fill_height)
# Position: centre it on X=0, and align to the slope
# The slope normal is (0, -sin30, cos30) = (0, -0.5, 0.866)
# The slope "up" direction is (0, -cos30, -sin30) = (0, -0.866, -0.5)
# We need to rotate the box so its depth aligns with the slope normal

# Simpler approach: create the fill box in the slope's coordinate system
# Slope tangent (horizontal): (1, 0, 0)
# Slope tangent (uphill): (0, cos30, sin30) = (0, 0.866, 0.5) -- but going "up" the slope toward the back
# Actually from the analysis, the slope goes from bottom-front to top-back
# Let me use the actual face positions to determine the fill geometry

# Use an extruded rectangle approach
slope_up = FreeCAD.Vector(0, 0.866, 0.5)  # direction up the slope
slope_right = FreeCAD.Vector(1, 0, 0)  # horizontal

# Centre of screen cutout on the slope
sc_centre = FreeCAD.Vector(0, screen_centre_y, screen_centre_z)

# Create a wire rectangle on the slope, slightly oversized
hw = screen_fill_width / 2 + 0.5  # half width + margin
hh = screen_fill_height / 2 + 0.5  # half height + margin

p1 = sc_centre - slope_right * hw - slope_up * hh
p2 = sc_centre + slope_right * hw - slope_up * hh
p3 = sc_centre + slope_right * hw + slope_up * hh
p4 = sc_centre - slope_right * hw + slope_up * hh

wire = Part.makePolygon([p1, p2, p3, p4, p1])
face = Part.Face(wire)
screen_fill_solid = face.extrude(angled_normal * screen_fill_depth)
fills.append(screen_fill_solid)

print(f"  Filling {len(fills)} features...")
result = original
for i, fill in enumerate(fills):
    try:
        result = result.fuse(fill)
    except Exception as e:
        print(f"  Warning: fill {i} failed: {e}")

result = result.removeSplitter()
print("  Fill complete, cleaning up...")

# ============================================================
# STEP 2: Cut all holes with correct dimensions
# ============================================================

print("Cutting new holes...")

cuts = []

# --- Pushbuttons: 8 x 16mm, same positions, 25mm spacing ---
for pos in pushbutton_old:
    cuts.append(make_cut_cylinder(pos[0], pos[1], pos[2], 16.0, flat_normal))

# --- Encoders: 8 x 9.5mm, NEW positions at 19.87mm spacing ---
# Centred on X=0, same Y/Z as original row
# Total span = 7 * 19.87 = 139.09mm
encoder_y = encoder_old[0][1]  # -115.33
encoder_z = encoder_old[0][2]  # 30.3
encoder_new_x = [-69.545 + i * 19.87 for i in range(8)]

for x in encoder_new_x:
    cuts.append(make_cut_cylinder(x, encoder_y, encoder_z, 9.5, angled_normal))

print(f"  Encoder positions: {[round(x,2) for x in encoder_new_x]}")
print(f"  Encoder spacing: 19.87mm c-to-c")

# --- Pots: 4 x 7mm, same positions ---
for pos in pot_positions:
    cuts.append(make_cut_cylinder(pos[0], pos[1], pos[2], 7.0, angled_normal))

# --- Screen mounting holes: 4 x 3.2mm, same positions ---
for pos in screen_mount_old:
    cuts.append(make_cut_cylinder(pos[0], pos[1], pos[2], 3.2, angled_normal))

# --- Screen cutout: 155 x 87mm (Option A: active area + tolerance) ---
# Centred on same position as original cutout
# The cutout is on the angled (30-degree) surface
# 155mm wide (X direction), 87mm along the slope

sc_hw = 155.0 / 2  # half width = 77.5mm
sc_hh = 87.0 / 2   # half height on slope = 43.5mm

p1 = sc_centre - slope_right * sc_hw - slope_up * sc_hh
p2 = sc_centre + slope_right * sc_hw - slope_up * sc_hh
p3 = sc_centre + slope_right * sc_hw + slope_up * sc_hh
p4 = sc_centre - slope_right * sc_hw + slope_up * sc_hh

wire = Part.makePolygon([p1, p2, p3, p4, p1])
face = Part.Face(wire)
screen_cut = face.extrude(angled_normal * (-screen_fill_depth))  # cut inward
cuts.append(screen_cut)

print(f"  Screen cutout: 155 x 87mm at centre ({round(sc_centre.x,1)}, {round(sc_centre.y,1)}, {round(sc_centre.z,1)})")

# --- LED bezels: 4 x 8mm, stacked vertically by each left pot ---
# Left top pot: (-101.0, -74.63, 53.8)
# Left bottom pot: (-101.0, -39.99, 73.8)
# LEDs go to the LEFT of each pot, stacked vertically (along slope)
# Pot knob edge at X = -101.0 - 10.05 = -111.05
# LED bezel centre at X = -116.0 (5mm clearance from knob edge, bezel edge at -121)
# Vertical offset: two LEDs per pot, ±7mm along slope from pot centre

led_x = -116.0
led_vert_offset = 7.0  # mm along slope direction from pot centre

for pot_pos in [(-101.0, -74.63, 53.8), (-101.0, -39.99, 73.8)]:
    pot_centre = FreeCAD.Vector(pot_pos[0], pot_pos[1], pot_pos[2])
    # Upper LED
    led_upper = FreeCAD.Vector(led_x,
                                pot_pos[1] + slope_up.y * led_vert_offset,
                                pot_pos[2] + slope_up.z * led_vert_offset)
    # Lower LED
    led_lower = FreeCAD.Vector(led_x,
                                pot_pos[1] - slope_up.y * led_vert_offset,
                                pot_pos[2] - slope_up.z * led_vert_offset)

    cuts.append(make_cut_cylinder(led_upper.x, led_upper.y, led_upper.z, 8.0, angled_normal))
    cuts.append(make_cut_cylinder(led_lower.x, led_lower.y, led_lower.z, 8.0, angled_normal))

    print(f"  LED pair near pot at X={pot_pos[0]}: upper=({round(led_upper.x,1)},{round(led_upper.y,1)},{round(led_upper.z,1)}), lower=({round(led_lower.x,1)},{round(led_lower.y,1)},{round(led_lower.z,1)})")

print(f"\n  Cutting {len(cuts)} features total...")
for i, cut in enumerate(cuts):
    try:
        result = result.cut(cut)
    except Exception as e:
        print(f"  Warning: cut {i} failed: {e}")

# ============================================================
# STEP 3: Export
# ============================================================

print(f"\nExporting to {output_path}...")
Part.Shape.exportStep(result, output_path)
print("Done!")

# ============================================================
# STEP 4: Verify
# ============================================================

print("\nVerifying output...")
doc2 = FreeCAD.newDocument("Verify")
Import.insert(output_path, doc2.Name)
vshape = doc2.Objects[0].Shape
print(f"Output: {len(vshape.Faces)} faces, {len(vshape.Edges)} edges")

from collections import defaultdict
by_d = defaultdict(int)
for face in vshape.Faces:
    if isinstance(face.Surface, Part.Cylinder):
        d = round(face.Surface.Radius * 2, 2)
        by_d[d] += 1

print("\nCylindrical surfaces by diameter:")
for d in sorted(by_d.keys()):
    print(f"  {d}mm: {by_d[d]} faces")

# Summary
print("\n=== SUMMARY ===")
print(f"Pushbuttons: 8 x 16.0mm (25mm spacing)")
print(f"Encoders:    8 x 9.5mm  (19.87mm spacing)")
print(f"Pots:        4 x 7.0mm")
print(f"Screen mount:4 x 3.2mm  (157.0 x 115.0mm)")
print(f"Screen cut:  155 x 87mm (Option A)")
print(f"LED bezels:  4 x 8.0mm  (2 per left pot, stacked vertically)")

FreeCAD.closeDocument(doc.Name)
FreeCAD.closeDocument(doc2.Name)
