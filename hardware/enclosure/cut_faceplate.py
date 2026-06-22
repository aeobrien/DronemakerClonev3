#!/usr/bin/env python3
"""
Cut all component holes into the clean original panel.
Starting from FaceplateOriginal.step (no component holes, just vents + Hammond hardware).

Coordinate system (from analysis):
- X: -127 to 127 (left to right, centred)
- Y: 0 (front/top) to -259.9 (back/bottom)
- Z: 0 (top) to -101.6 (bottom)

Key faces:
- Angled slope: normal (0, -0.5, 0.866) — screen, encoders, pots, LEDs
- Flat bottom (pushbuttons): normal (0, -1, 0) — the short vertical face at the front
- Back panel area: the lower rear portion

The slope runs from roughly Y=-133 (top of slope) to Y=-258 (bottom of slope),
Z from roughly 0 (top) to -74 (bottom).
"""

import sys
import math
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'
output_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v2.step'

print("Loading clean panel...")
doc = FreeCAD.newDocument("Faceplate")
Import.insert(input_path, doc.Name)
panel = doc.Objects[0].Shape

# ============================================================
# Geometry helpers
# ============================================================

# Angled face normal (30-degree slope, outward-facing)
slope_normal = FreeCAD.Vector(0, -0.5, 0.866)
slope_normal.normalize()

# Direction "up" along the slope (from bottom of slope toward top)
slope_up = FreeCAD.Vector(0, 0.866, 0.5)
slope_up.normalize()

# Flat bottom face normal (pushbuttons face forward)
flat_normal = FreeCAD.Vector(0, -1, 0)

# Cut depth — generous to go through 1.63mm panel
cut_depth = 10.0

def cut_circle(shape, x, y, z, diameter, axis):
    """Cut a circular hole through the panel at the given position."""
    r = diameter / 2.0
    centre = FreeCAD.Vector(x, y, z)
    start = centre - axis * cut_depth
    cyl = Part.makeCylinder(r, cut_depth * 2, start, axis)
    return shape.cut(cyl)

def cut_rectangle(shape, centre, width, height, axis, right_dir, up_dir):
    """Cut a rectangular hole through the panel."""
    hw = width / 2.0
    hh = height / 2.0
    c = FreeCAD.Vector(centre[0], centre[1], centre[2])

    p1 = c - right_dir * hw - up_dir * hh
    p2 = c + right_dir * hw - up_dir * hh
    p3 = c + right_dir * hw + up_dir * hh
    p4 = c - right_dir * hw + up_dir * hh

    wire = Part.makePolygon([p1, p2, p3, p4, p1])
    face = Part.Face(wire)
    # Extrude in both directions to ensure full cut-through
    box = face.extrude(axis * cut_depth)
    result = shape.cut(box)
    box2 = face.extrude(axis * (-cut_depth))
    result = result.cut(box2)
    return result

# ============================================================
# Position calculations
# ============================================================

# We need to find actual positions on the slope surface.
# From v1 analysis, the slope face (Face 14) runs from:
#   Y ≈ -133 to -258, Z ≈ 0 to -74
# The slope centre is approximately Y=-195.5, Z=-37.7
#
# For the original v1 model (different coord system), positions were:
#   Encoders: Y=-115.33, Z=30.3 (in v1 coords where Y went positive)
#   Pots: (-101, -74.63, 53.8) and (-101, -39.99, 73.8)
#   Screen mounts: (-78.45, -107.1, 35.05) etc.
#
# In THIS coord system (Y goes negative, Z goes negative):
# The slope surface normal is (0, -0.5, 0.866) which means the
# outer face points down-and-forward.
#
# Let me find a reference point on the slope.
# The slope face bounding box centre from analysis: Y=-195.5, Z=-37.7
# The slope spans 254mm in X (full width), ~125mm along Y, ~72mm along Z

# To place features on the slope, I need to pick a point ON the slope surface
# and offset from there using slope_up and the X axis.

# Let me find the slope surface equation.
# The slope face is Face 14: normal=(0, -0.5, 0.866)
# A point on this face... let me sample it.
slope_face = None
for face in panel.Faces:
    if isinstance(face.Surface, Part.Plane):
        n = face.Surface.Axis
        if abs(n.y - (-0.5)) < 0.01 and abs(n.z - 0.866) < 0.01 and face.Area > 10000:
            slope_face = face
            # Get a point on the surface
            uv = face.Surface.parameter(face.BoundBox.Center)
            pt = face.Surface.value(uv[0], uv[1])
            print(f"Slope face found: area={face.Area:.0f}, point on surface=({pt.x:.1f},{pt.y:.1f},{pt.z:.1f})")
            print(f"  BBox: Y={face.BoundBox.YMin:.1f} to {face.BoundBox.YMax:.1f}, Z={face.BoundBox.ZMin:.1f} to {face.BoundBox.ZMax:.1f}")
            break

if slope_face is None:
    # Try the other slope face
    for face in panel.Faces:
        if isinstance(face.Surface, Part.Plane):
            n = face.Surface.Axis
            if abs(n.y + 0.5) < 0.01 and abs(n.z - 0.866) < 0.01 and face.Area > 10000:
                slope_face = face
                print(f"Slope face (inner): area={face.Area:.0f}")
                break

# Find the flat bottom face (pushbutton area)
flat_face = None
for face in panel.Faces:
    if isinstance(face.Surface, Part.Plane):
        n = face.Surface.Axis
        if abs(n.y - (-1.0)) < 0.01 and abs(n.x) < 0.01 and abs(n.z) < 0.01 and face.Area > 5000:
            flat_face = face
            print(f"Flat face found: area={face.Area:.0f}")
            print(f"  BBox: X={face.BoundBox.XMin:.1f} to {face.BoundBox.XMax:.1f}, Y={face.BoundBox.Center.y:.1f}, Z={face.BoundBox.ZMin:.1f} to {face.BoundBox.ZMax:.1f}")
            break

# The slope surface: y * (-0.5) + z * 0.866 = d (for some constant d)
# From the analysis, using the face centre at approx Y=-195.5, Z=-37.7:
# We can compute d, but more practically, let's use the actual surface position.
# A point on the outer slope face: the surface passes through the panel's outer surface.

# Let me use a direct approach: compute positions relative to the slope.
# The slope's "zero" in the slope-up direction corresponds to the bottom edge of the slope.
# The slope's bottom edge is at approximately Y=-258, Z=-74 (from bounding box).
# The slope's top edge is at approximately Y=-133, Z=0.

# Slope length along the surface: sqrt((258-133)^2 + 74^2) = sqrt(125^2 + 72^2) ≈ 144.3mm
# Or more precisely, the slope runs 125mm in Y and 72mm in Z,
# but the actual surface length = 125 / cos(30°) ≈ 144.3mm

# Reference: centre of the slope
slope_y_min = slope_face.BoundBox.YMin  # bottom of slope (most negative Y = near back)
slope_y_max = slope_face.BoundBox.YMax  # top of slope (least negative Y = near front)
slope_z_min = slope_face.BoundBox.ZMin  # bottom Z
slope_z_max = slope_face.BoundBox.ZMax  # top Z

slope_y_centre = (slope_y_min + slope_y_max) / 2
slope_z_centre = (slope_z_min + slope_z_max) / 2

print(f"\nSlope Y range: {slope_y_min:.1f} to {slope_y_max:.1f} (centre {slope_y_centre:.1f})")
print(f"Slope Z range: {slope_z_min:.1f} to {slope_z_max:.1f} (centre {slope_z_centre:.1f})")

# The slope surface in this coord system:
# Going "up" the slope (toward the top/front of the enclosure) means:
#   Y increases (toward 0), Z increases (toward 0)
# slope_up = (0, 0.866, 0.5) normalised = (0, 0.866, 0.5)

# ============================================================
# Feature positions on the slope
# ============================================================
# All positions need to be ON the slope surface.
# I'll define positions as (X, offset_along_slope_from_centre)
# Then compute actual Y,Z coordinates.

# From v1, the relative positions were (converting to this system):
# The v1 had encoders at the bottom of the slope (near the front edge)
# and pots flanking the screen in the middle of the slope.

# Let me map from v1 positions to this coord system.
# In v1: Y ranged roughly -130 to +130, Z ranged 0 to 103
# In this: Y ranges 0 to -260, Z ranges 0 to -102
# The transformation appears to be: this_Y = -(v1_Y + 130), this_Z = -(v1_Z)
# Let me verify with the encoder positions from v1: Y=-115.33, Z=30.3
# this_Y = -(-115.33 + 130) = -(14.67) = -14.67? That doesn't seem right.

# Better approach: just use the surface parametrically.
# The slope is a plane. I need a point on it and the two tangent directions.
# From the face's Surface object:
slope_surface = slope_face.Surface
slope_origin = slope_surface.Position  # A point on the plane
slope_plane_normal = slope_surface.Axis

print(f"\nSlope plane origin: ({slope_origin.x:.2f}, {slope_origin.y:.2f}, {slope_origin.z:.2f})")
print(f"Slope plane normal: ({slope_plane_normal.x:.4f}, {slope_plane_normal.y:.4f}, {slope_plane_normal.z:.4f})")

# Project points onto the slope surface
def point_on_slope(x, slope_offset_mm):
    """
    Get a 3D point on the slope surface.
    x: horizontal position (left-right)
    slope_offset_mm: distance along the slope from the bottom edge.
        0 = bottom of slope (near pushbuttons)
        positive = up the slope toward the top
    """
    # Bottom of slope (where it meets the flat pushbutton face)
    # From analysis: Y ≈ -258.3, Z ≈ -74.1 (the bend)
    bottom_y = -258.0
    bottom_z = -74.0

    # slope_up direction: (0, 0.866, 0.5) — going up means Y increases, Z increases
    y = bottom_y + slope_up.y * slope_offset_mm
    z = bottom_z + slope_up.z * slope_offset_mm

    return (x, y, z)

# Slope length ≈ 144mm (from analysis)
slope_length = 144.0

# ============================================================
# Layout: Encoders at bottom of slope, screen in middle, pots flanking
# ============================================================

# Encoders: row near the bottom of the slope
# In v1, they were close to the slope/flat transition
# Let's place them about 15mm up from the bottom of the slope
encoder_slope_offset = 15.0

# Screen: centred on slope
# Active area 155 x 87mm (our Option A cutout)
# Screen centre roughly at half the slope height
screen_slope_offset = slope_length / 2  # ~72mm up from bottom

# Screen mounting holes: 157.0 x 115.0mm pattern, centred on screen
# The 115mm is along the slope direction
screen_mount_slope_offsets = [
    screen_slope_offset - 115.0/2,  # bottom pair
    screen_slope_offset + 115.0/2,  # top pair
]
screen_mount_x = [-157.0/2, 157.0/2]  # left and right

# Pots: flanking the screen, roughly vertically centred on the screen
# Left pots at X = -101
# Right pots at X = +101
# Top pot: screen_centre + 20mm up the slope
# Bottom pot: screen_centre - 20mm down the slope
pot_slope_offsets = [
    screen_slope_offset + 20.0,  # top pot
    screen_slope_offset - 20.0,  # bottom pot
]

# LEDs: to the left of each left pot, stacked vertically
# 2 LEDs per pot, ±7mm from pot centre along slope
led_x = -116.0  # left of pots at -101
led_slope_offsets = []
for pot_offset in pot_slope_offsets:
    led_slope_offsets.append((led_x, pot_offset + 7.0))  # upper LED
    led_slope_offsets.append((led_x, pot_offset - 7.0))  # lower LED

# Pushbuttons: on the flat face
# 8 buttons, 25mm spacing, centred on X=0
# The flat face centre is at approximately Y=-258.3, Z=-86.8
pushbutton_y = -258.3  # Will be adjusted to actual face position
pushbutton_z = -86.8
pushbutton_x_positions = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]

# ============================================================
# Compute actual 3D positions
# ============================================================

print("\n=== COMPUTED POSITIONS ===")

# Encoders: 8 x 9.5mm, 19.87mm spacing
encoder_new_x = [-69.545 + i * 19.87 for i in range(8)]
encoder_positions = []
for x in encoder_new_x:
    pos = point_on_slope(x, encoder_slope_offset)
    encoder_positions.append(pos)
print(f"\nEncoders (9.5mm, 19.87mm spacing):")
for i, pos in enumerate(encoder_positions):
    print(f"  E{i+1}: ({pos[0]:.2f}, {pos[1]:.2f}, {pos[2]:.2f})")

# Screen cutout
screen_centre = point_on_slope(0, screen_slope_offset)
print(f"\nScreen cutout (155 x 87mm): centre ({screen_centre[0]:.1f}, {screen_centre[1]:.1f}, {screen_centre[2]:.1f})")

# Screen mounting holes
screen_mount_positions = []
for slope_off in screen_mount_slope_offsets:
    for x in screen_mount_x:
        pos = point_on_slope(x, slope_off)
        screen_mount_positions.append(pos)
print(f"\nScreen mounting holes (3.2mm):")
for pos in screen_mount_positions:
    print(f"  ({pos[0]:.2f}, {pos[1]:.2f}, {pos[2]:.2f})")

# Pot positions
pot_positions = []
for side_x in [-101.0, 101.0]:
    for slope_off in pot_slope_offsets:
        pos = point_on_slope(side_x, slope_off)
        pot_positions.append(pos)
print(f"\nPots (7.0mm):")
for pos in pot_positions:
    print(f"  ({pos[0]:.2f}, {pos[1]:.2f}, {pos[2]:.2f})")

# LED positions
led_positions = []
for led_x_pos, slope_off in led_slope_offsets:
    pos = point_on_slope(led_x_pos, slope_off)
    led_positions.append(pos)
print(f"\nLED bezels (8.0mm):")
for pos in led_positions:
    print(f"  ({pos[0]:.2f}, {pos[1]:.2f}, {pos[2]:.2f})")

# Pushbutton positions (on flat face)
print(f"\nPushbuttons (16.0mm, 25mm spacing):")
for x in pushbutton_x_positions:
    print(f"  ({x:.1f}, {pushbutton_y:.1f}, {pushbutton_z:.1f})")

# ============================================================
# Cut all holes
# ============================================================

print("\n=== CUTTING HOLES ===")
result = panel

# 1. Encoders: 8 x 9.5mm
print("Cutting 8 encoder holes (9.5mm)...")
for pos in encoder_positions:
    result = cut_circle(result, pos[0], pos[1], pos[2], 9.5, slope_normal)

# 2. Screen cutout: 155 x 87mm rectangle
print("Cutting screen cutout (155 x 87mm)...")
slope_right = FreeCAD.Vector(1, 0, 0)
result = cut_rectangle(result, screen_centre, 155.0, 87.0, slope_normal, slope_right, slope_up)

# 3. Screen mounting holes: 4 x 3.2mm
print("Cutting 4 screen mounting holes (3.2mm)...")
for pos in screen_mount_positions:
    result = cut_circle(result, pos[0], pos[1], pos[2], 3.2, slope_normal)

# 4. Pots: 4 x 7.0mm
print("Cutting 4 pot holes (7.0mm)...")
for pos in pot_positions:
    result = cut_circle(result, pos[0], pos[1], pos[2], 7.0, slope_normal)

# 5. LED bezels: 4 x 8.0mm
print("Cutting 4 LED bezel holes (8.0mm)...")
for pos in led_positions:
    result = cut_circle(result, pos[0], pos[1], pos[2], 8.0, slope_normal)

# 6. Pushbuttons: 8 x 16mm
print("Cutting 8 pushbutton holes (16.0mm)...")
for x in pushbutton_x_positions:
    result = cut_circle(result, x, pushbutton_y, pushbutton_z, 16.0, flat_normal)

# ============================================================
# Export
# ============================================================

print(f"\nExporting to {output_path}...")
Part.Shape.exportStep(result, output_path)
print("Done!")

# Verify
print("\nVerifying...")
doc2 = FreeCAD.newDocument("Verify")
Import.insert(output_path, doc2.Name)
vshape = doc2.Objects[0].Shape
print(f"Output: {len(vshape.Faces)} faces, {len(vshape.Edges)} edges")

from collections import defaultdict
by_d = defaultdict(int)
for face in vshape.Faces:
    if isinstance(face.Surface, Part.Cylinder):
        d = round(face.Surface.Radius * 2, 1)
        by_d[d] += 1

print("\nCylindrical surfaces:")
for d in sorted(by_d.keys()):
    print(f"  {d}mm: {by_d[d]} faces")

print("\n=== SUMMARY ===")
print(f"Encoders:     8 x 9.5mm  @ 19.87mm c-to-c, {encoder_slope_offset:.0f}mm up from slope bottom")
print(f"Screen cut:   155 x 87mm @ slope centre ({screen_slope_offset:.0f}mm up)")
print(f"Screen mount: 4 x 3.2mm  @ 157.0 x 115.0mm pattern")
print(f"Pots:         4 x 7.0mm  @ X=±101, ±20mm from screen centre")
print(f"LED bezels:   4 x 8.0mm  @ X=-116, stacked ±7mm from each left pot")
print(f"Pushbuttons:  8 x 16.0mm @ 25mm spacing on flat face")

FreeCAD.closeDocument(doc.Name)
FreeCAD.closeDocument(doc2.Name)
