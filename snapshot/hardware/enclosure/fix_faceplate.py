#!/usr/bin/env python3
"""
Fix Faceplate_v1.step dimensions using FreeCAD scripting.

Changes:
1. Encoder holes: 7.0mm → 9.5mm diameter
2. Encoder spacing: 18.5mm → 19.87mm centre-to-centre
3. Screen mounting holes: 3.0mm → 3.2mm diameter
4. Screen cutout: current size → 155 × 87mm (Option A: active area + tolerance)
5. Add 4 × 8mm LED bezel holes (two by each left-side pot)

This script loads the STEP, identifies features by geometry, applies fixes,
and exports a corrected STEP file.
"""

import sys
import math

# FreeCAD imports
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

# Load the STEP file
input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v1.step'
output_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v2.step'

print("Loading STEP file...")
doc = FreeCAD.newDocument("Faceplate")
Import.insert(input_path, doc.Name)

# Get the shape
shape = doc.Objects[0].Shape
print(f"Loaded shape with {len(shape.Faces)} faces, {len(shape.Edges)} edges")

# Analyze all circular edges to find holes
circles = []
for i, edge in enumerate(shape.Edges):
    if isinstance(edge.Curve, Part.Circle):
        c = edge.Curve
        circles.append({
            'index': i,
            'centre': (round(c.Center.x, 4), round(c.Center.y, 4), round(c.Center.z, 4)),
            'radius': round(c.Radius, 4),
            'diameter': round(c.Radius * 2, 4),
            'axis': (round(c.Axis.x, 4), round(c.Axis.y, 4), round(c.Axis.z, 4))
        })

# Group by diameter
from collections import defaultdict
by_diameter = defaultdict(list)
for c in circles:
    d = round(c['diameter'], 1)
    by_diameter[d].append(c)

print("\n=== HOLES FOUND ===")
for d in sorted(by_diameter.keys()):
    holes = by_diameter[d]
    print(f"\nDiameter {d}mm: {len(holes)} circles")
    for h in holes:
        print(f"  Centre: ({h['centre'][0]:.2f}, {h['centre'][1]:.2f}, {h['centre'][2]:.2f})  Axis: {h['axis']}")

# Also find rectangular cutouts (screen)
print("\n=== LOOKING FOR RECTANGULAR FEATURES ===")
for i, face in enumerate(shape.Faces):
    if isinstance(face.Surface, Part.Plane):
        bb = face.BoundBox
        w = round(bb.XLength, 2)
        h_y = round(bb.YLength, 2)
        h_z = round(bb.ZLength, 2)
        # Look for faces that might be screen cutout edges (large rectangular features)
        if w > 50 or h_y > 50 or h_z > 50:
            normal = face.Surface.Axis
            print(f"  Face {i}: {w} x {h_y} x {h_z}mm, normal=({normal.x:.3f},{normal.y:.3f},{normal.z:.3f}), centre=({round(bb.Center.x,2)},{round(bb.Center.y,2)},{round(bb.Center.z,2)})")

print("\n=== ANALYSIS COMPLETE ===")
print("Review the output above before proceeding with modifications.")

FreeCAD.closeDocument(doc.Name)
