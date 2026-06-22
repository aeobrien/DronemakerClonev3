#!/usr/bin/env python3
"""Analyze the user's fixed v6 file and the display model together."""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

fix_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v6_fix.step'
display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'

# Load both
doc1 = FreeCAD.newDocument("Fix")
Import.insert(fix_path, doc1.Name)
fix_shape = doc1.Objects[0].Shape

doc2 = FreeCAD.newDocument("Display")
Import.insert(display_path, doc2.Name)
display_shape = doc2.Objects[0].Shape

# ============================================================
# Analyze fixed faceplate
# ============================================================
print("=== FIXED FACEPLATE ===")
print(f"Faces: {len(fix_shape.Faces)}, Edges: {len(fix_shape.Edges)}")

from collections import defaultdict
bd = defaultdict(list)
for face in fix_shape.Faces:
    if isinstance(face.Surface, Part.Cylinder):
        d = round(face.Surface.Radius * 2, 2)
        c = face.Surface.Center
        a = face.Surface.Axis
        bd[d].append({'centre': (round(c.x,1), round(c.y,1), round(c.z,1)),
                      'axis': (round(a.x,2), round(a.y,2), round(a.z,2))})

print("\nCylinders by diameter:")
for d in sorted(bd.keys()):
    items = bd[d]
    print(f"\n  {d}mm: {len(items)} faces")
    for item in items:
        print(f"    centre={item['centre']} axis={item['axis']}")

# ============================================================
# Deep analysis of display model
# ============================================================
print("\n\n=== DISPLAY MODEL DEEP ANALYSIS ===")

# Native orientation: X=horizontal, Y=depth (screen at Y=-3.4), Z=vertical
dbb = display_shape.BoundBox
print(f"BBox: X=[{dbb.XMin:.2f}, {dbb.XMax:.2f}] ({dbb.XLength:.2f})")
print(f"      Y=[{dbb.YMin:.2f}, {dbb.YMax:.2f}] ({dbb.YLength:.2f})")
print(f"      Z=[{dbb.ZMin:.2f}, {dbb.ZMax:.2f}] ({dbb.ZLength:.2f})")

# Find the screen/display glass surface
# This should be the largest face with normal (0, -1, 0) — facing forward
print("\nFaces with normal ≈ (0, -1, 0) — screen-facing surfaces:")
screen_faces = []
for i, face in enumerate(display_shape.Faces):
    if isinstance(face.Surface, Part.Plane):
        n = face.Surface.Axis
        if abs(n.y + 1.0) < 0.01 and face.Area > 100:
            fbb = face.BoundBox
            screen_faces.append((face.Area, i, fbb))
            print(f"  Face {i}: area={face.Area:.0f} size=({fbb.XLength:.1f}x{fbb.ZLength:.1f}) at Y={fbb.Center.y:.2f}")

# The active display area should be a distinct face
# From earlier analysis: Face 168 was 158x90mm at Y=-3.3 — that's likely the glass
# And Face 9 was 165x124mm at Y=4.6 — that's the back of the board

print("\nAll mounting-related cylinders:")
mount_holes = []
standoff_bosses = []
for i, face in enumerate(display_shape.Faces):
    if isinstance(face.Surface, Part.Cylinder):
        r = face.Surface.Radius
        c = face.Surface.Center
        if abs(r - 1.65) < 0.1:  # M3 holes (3.3mm)
            dup = any(abs(c.x - e['x']) < 0.5 and abs(c.z - e['z']) < 0.5 for e in mount_holes)
            if not dup:
                mount_holes.append({'x': c.x, 'y': c.y, 'z': c.z, 'r': r})
        elif abs(r - 5.8) < 0.5:  # standoff bosses (11.6mm dia)
            dup = any(abs(c.x - e['x']) < 0.5 and abs(c.z - e['z']) < 0.5 for e in standoff_bosses)
            if not dup:
                standoff_bosses.append({'x': c.x, 'y': c.y, 'z': c.z, 'r': r})

print(f"\nMounting holes (3.3mm dia): {len(mount_holes)}")
for h in sorted(mount_holes, key=lambda h: (h['z'], h['x'])):
    print(f"  X={h['x']:.2f}, Y={h['y']:.2f}, Z={h['z']:.2f}")

print(f"\nStandoff bosses (11.6mm dia): {len(standoff_bosses)}")
for s in sorted(standoff_bosses, key=lambda s: (s['z'], s['x'])):
    print(f"  X={s['x']:.2f}, Y={s['y']:.2f}, Z={s['z']:.2f}")

# Calculate exact spacings
if len(mount_holes) >= 2:
    x_vals = sorted(set(round(h['x'], 1) for h in mount_holes))
    z_vals = sorted(set(round(h['z'], 1) for h in mount_holes))
    print(f"\nMount hole X positions: {x_vals}")
    print(f"Mount hole Z positions: {z_vals}")
    if len(x_vals) >= 2:
        print(f"H spacing: {x_vals[-1] - x_vals[0]:.2f}mm")
    if len(z_vals) >= 2:
        print(f"V spacing: {z_vals[-1] - z_vals[0]:.2f}mm")

# Find the active display area
# Look for a rectangular face that's roughly 154x86mm
print("\nLooking for active display area face:")
for i, face in enumerate(display_shape.Faces):
    if isinstance(face.Surface, Part.Plane):
        fbb = face.BoundBox
        # Active area should be ~154 x 86mm
        if 140 < fbb.XLength < 160 and 80 < fbb.ZLength < 95:
            n = face.Surface.Axis
            print(f"  CANDIDATE Face {i}: {fbb.XLength:.2f} x {fbb.ZLength:.2f}mm at Y={fbb.Center.y:.2f}")
            print(f"    X range: [{fbb.XMin:.2f}, {fbb.XMax:.2f}]")
            print(f"    Z range: [{fbb.ZMin:.2f}, {fbb.ZMax:.2f}]")
            print(f"    Normal: ({n.x:.2f}, {n.y:.2f}, {n.z:.2f})")
            print(f"    Centre: ({fbb.Center.x:.2f}, {fbb.Center.y:.2f}, {fbb.Center.z:.2f})")

# Also find the board outline face
print("\nLooking for board outline (PCB body):")
for i, face in enumerate(display_shape.Faces):
    if isinstance(face.Surface, Part.Plane):
        fbb = face.BoundBox
        if 160 < fbb.XLength < 170 and 95 < fbb.ZLength < 110:
            n = face.Surface.Axis
            print(f"  CANDIDATE Face {i}: {fbb.XLength:.2f} x {fbb.ZLength:.2f}mm at Y={fbb.Center.y:.2f}")
            print(f"    X range: [{fbb.XMin:.2f}, {fbb.XMax:.2f}]")
            print(f"    Z range: [{fbb.ZMin:.2f}, {fbb.ZMax:.2f}]")

# Key relationships to compute:
print("\n=== KEY RELATIONSHIPS ===")
if mount_holes and screen_faces:
    # Mount hole centre (average)
    avg_mount_x = sum(h['x'] for h in mount_holes) / len(mount_holes)
    avg_mount_z = sum(h['z'] for h in mount_holes) / len(mount_holes)
    print(f"Mount hole geometric centre: X={avg_mount_x:.2f}, Z={avg_mount_z:.2f}")

    # Display active area centre (from the candidate face)
    # Will be printed above

FreeCAD.closeDocument(doc1.Name)
FreeCAD.closeDocument(doc2.Name)
