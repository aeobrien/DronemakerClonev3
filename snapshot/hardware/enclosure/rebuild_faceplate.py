#!/usr/bin/env python3
"""
Rebuild Faceplate_v2.step from Faceplate_v1.step with corrected dimensions.

Strategy: Load the original solid, then use boolean operations to:
1. Fill in wrong-sized holes (by adding cylinders)
2. Cut new correctly-sized holes

Changes:
- Encoder holes: 7.0mm → 9.5mm, respaced to 19.87mm c-to-c
- Screen mounting holes: 3.0mm → 3.2mm (same positions)
- Add 4 × 8.0mm LED bezel holes near left-side pots
- Screen cutout: adjust to 155 × 87mm (Option A)
"""

import sys
import math
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v1.step'
output_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v2.step'

print("Loading STEP file...")
doc = FreeCAD.newDocument("Faceplate")
Import.insert(input_path, doc.Name)
shape = doc.Objects[0].Shape

# The angled face has normal (0, -0.5, 0.866) which is 30 degrees from vertical
# Axis for holes through the angled surface:
angled_axis = FreeCAD.Vector(0, -0.5, 0.866)  # normal to the 30-degree slope
angled_axis.normalize()

# Flat bottom face axis (for pushbuttons):
flat_axis = FreeCAD.Vector(0, -1, 0)

# We need a generous cylinder length to cut fully through the panel (1.63mm thick)
cut_length = 20.0  # generous to ensure full cut-through

# ============================================================
# STEP 1: Identify existing hole positions from the analysis
# ============================================================

# Encoder row (currently 7mm, 18.5mm spacing on angled face)
# Centres from analysis (Y and Z are on the angled plane):
# X positions: -64.75, -46.25, -27.75, -9.25, 9.25, 27.75, 46.25, 64.75
# These are 18.5mm apart. We need 19.87mm apart.
# The row centre Y,Z: (-115.33, 30.3)

encoder_old_x = [-64.75, -46.25, -27.75, -9.25, 9.25, 27.75, 46.25, 64.75]
encoder_y = -115.33
encoder_z = 30.3

# New encoder X positions: centred on 0, 19.87mm spacing
# 8 encoders, 7 gaps: total span = 7 * 19.87 = 139.09mm
# First centre at -139.09/2 = -69.545
encoder_new_x = [-69.545 + i * 19.87 for i in range(8)]
print(f"New encoder X positions: {[round(x,2) for x in encoder_new_x]}")
print(f"Total span: {encoder_new_x[-1] - encoder_new_x[0]:.2f}mm")

# Pot holes (7mm, correct size, on angled face)
# Left pots: (-101.0, -74.63, 53.8) and (-101.0, -39.99, 73.8)
# Right pots: (101.0, -74.63, 53.8) and (101.0, -39.99, 73.8)
# These stay as-is

# Screen mounting holes (currently 3mm, need 3.2mm)
screen_mount_positions = [
    (-78.45, -107.1, 35.05),
    (78.45, -107.1, 35.05),
    (-78.45, -7.51, 92.55),
    (78.45, -7.51, 92.55),
]

# LED bezel positions: two by each left-side pot
# Left top pot is at (-101.0, -74.63, 53.8)
# Left bottom pot is at (-101.0, -39.99, 73.8)
# Place LEDs to the right of each pot, offset by ~15mm in X
# Actually, "two by each pot" means two LEDs near each of the two left pots
# Let's place them vertically above and below each pot, offset in the slope direction
# Or more practically: two LEDs between/beside each left pot
# Pot knob is 20.1mm dia, LED bezel is 10mm outer dia
# Minimum c-to-c between pot and LED: (20.1/2 + 10/2) = 15.05mm
# Place LEDs 16mm to the right (+X) of each left pot
led_positions = [
    (-101.0 + 16.0, -74.63, 53.8),   # Right of left top pot
    (-101.0 + 32.0, -74.63, 53.8),   # Further right of left top pot
    (-101.0 + 16.0, -39.99, 73.8),   # Right of left bottom pot
    (-101.0 + 32.0, -39.99, 73.8),   # Further right of left bottom pot
]

print(f"\nLED positions: {[(round(x,1),round(y,1),round(z,1)) for x,y,z in led_positions]}")

# ============================================================
# STEP 2: Build modification shapes
# ============================================================

print("\nBuilding modification geometry...")

# Helper: create a cylinder on the angled surface at a given centre point
def make_angled_cylinder(x, y, z, diameter):
    """Create a cylinder perpendicular to the 30-degree angled surface."""
    radius = diameter / 2.0
    # The cylinder needs to be along the angled_axis direction
    # Position it so it extends through the panel
    start = FreeCAD.Vector(x, y, z) - angled_axis * (cut_length / 2)
    cyl = Part.makeCylinder(radius, cut_length, start, angled_axis)
    return cyl

# --- Fill old encoder holes (7mm) ---
fill_shapes = []
for x in encoder_old_x:
    cyl = make_angled_cylinder(x, encoder_y, encoder_z, 7.0 - 0.01)  # slightly smaller to ensure clean fill
    fill_shapes.append(cyl)

# --- Fill old screen mounting holes (3.0mm) ---
for pos in screen_mount_positions:
    cyl = make_angled_cylinder(pos[0], pos[1], pos[2], 3.0 - 0.01)
    fill_shapes.append(cyl)

# --- Cut new encoder holes (9.5mm at new positions) ---
cut_shapes = []
for x in encoder_new_x:
    cyl = make_angled_cylinder(x, encoder_y, encoder_z, 9.5)
    cut_shapes.append(cyl)

# --- Cut new screen mounting holes (3.2mm at same positions) ---
for pos in screen_mount_positions:
    cyl = make_angled_cylinder(pos[0], pos[1], pos[2], 3.2)
    cut_shapes.append(cyl)

# --- Cut LED bezel holes (8.0mm) ---
for pos in led_positions:
    cyl = make_angled_cylinder(pos[0], pos[1], pos[2], 8.0)
    cut_shapes.append(cyl)

# ============================================================
# STEP 3: Apply boolean operations
# ============================================================

print("Applying boolean operations...")

# First: fill old holes by fusing cylinders into the solid
result = shape
print(f"  Filling {len(fill_shapes)} old holes...")
for i, fill in enumerate(fill_shapes):
    try:
        result = result.fuse(fill)
    except Exception as e:
        print(f"  Warning: fill {i} failed: {e}")

# Clean up fuse artifacts
result = result.removeSplitter()

# Then: cut new holes
print(f"  Cutting {len(cut_shapes)} new holes...")
for i, cut in enumerate(cut_shapes):
    try:
        result = result.cut(cut)
    except Exception as e:
        print(f"  Warning: cut {i} failed: {e}")

# ============================================================
# STEP 4: Handle screen cutout resize
# ============================================================
# The screen cutout is 150mm wide currently. We need 155 × 87mm.
# This is harder to do with boolean ops on the existing cutout.
# For now, we'll skip this and flag it for manual adjustment.
# The cutout sides are vertical faces at X = ±75 (150mm total).
# We need X = ±77.5 (155mm total).
# And the height on the slope needs to be 87mm.
#
# TODO: Screen cutout resize - may need to be done in FreeCAD GUI
# Current: 150mm wide. Need: 155mm wide.
# Current height on slope: check against 87mm target.

print("\nNOTE: Screen cutout resize (150 → 155mm wide, height → 87mm) not applied.")
print("This may need manual adjustment in FreeCAD GUI.")

# ============================================================
# STEP 5: Export
# ============================================================

print(f"\nExporting to {output_path}...")
Part.Shape.exportStep(result, output_path)
print("Done!")

# Verify
print("\nVerifying output...")
doc2 = FreeCAD.newDocument("Verify")
Import.insert(output_path, doc2.Name)
vshape = doc2.Objects[0].Shape
print(f"Output shape: {len(vshape.Faces)} faces, {len(vshape.Edges)} edges")

# Check new holes
print("\nNew cylindrical surfaces:")
from collections import defaultdict
by_d = defaultdict(list)
for i, face in enumerate(vshape.Faces):
    if isinstance(face.Surface, Part.Cylinder):
        d = round(face.Surface.Radius * 2, 2)
        by_d[d].append(i)

for d in sorted(by_d.keys()):
    print(f"  {d}mm: {len(by_d[d])} faces")

FreeCAD.closeDocument(doc.Name)
FreeCAD.closeDocument(doc2.Name)
