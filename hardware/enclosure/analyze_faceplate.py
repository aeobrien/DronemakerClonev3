#!/usr/bin/env python3
"""Thorough analysis of all holes in Faceplate_v1.step"""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v1.step'
doc = FreeCAD.newDocument("Faceplate")
Import.insert(input_path, doc.Name)
shape = doc.Objects[0].Shape

print(f"Shape: {len(shape.Faces)} faces, {len(shape.Edges)} edges, {len(shape.Solids)} solids")

# Method 1: Find all cylindrical surfaces (hole walls)
print("\n=== CYLINDRICAL SURFACES (hole walls) ===")
cyls = []
for i, face in enumerate(shape.Faces):
    if isinstance(face.Surface, Part.Cylinder):
        c = face.Surface
        centre = c.Center
        radius = c.Radius
        axis = c.Axis
        cyls.append({
            'face': i,
            'centre': (round(centre.x, 2), round(centre.y, 2), round(centre.z, 2)),
            'diameter': round(radius * 2, 2),
            'axis': (round(axis.x, 3), round(axis.y, 3), round(axis.z, 3))
        })

# Group by diameter
from collections import defaultdict
by_d = defaultdict(list)
for c in cyls:
    by_d[c['diameter']].append(c)

for d in sorted(by_d.keys()):
    items = by_d[d]
    print(f"\nDiameter {d}mm: {len(items)} cylindrical faces")
    for item in items:
        print(f"  Face {item['face']}: centre=({item['centre'][0]}, {item['centre'][1]}, {item['centre'][2]}) axis={item['axis']}")

# Method 2: Check all edge curves more broadly
print("\n=== ALL CIRCULAR/ELLIPTICAL EDGES ===")
edge_types = defaultdict(int)
for edge in shape.Edges:
    t = type(edge.Curve).__name__
    edge_types[t] += 1

print("Edge curve types found:")
for t, count in sorted(edge_types.items()):
    print(f"  {t}: {count}")

# Method 3: Look for BSpline circles (sometimes STEP imports convert circles to BSplines)
print("\n=== BSPLINE EDGES (potential circles) ===")
for i, edge in enumerate(shape.Edges):
    if isinstance(edge.Curve, Part.BSplineCurve):
        # Check if it's roughly circular by sampling points
        pts = [edge.valueAt(edge.FirstParameter + (edge.LastParameter - edge.FirstParameter) * t / 8) for t in range(9)]
        # Check bounding box
        bb = edge.BoundBox
        if bb.XLength > 2 and bb.YLength > 2:  # skip tiny features
            print(f"  Edge {i}: BBox {round(bb.XLength,1)} x {round(bb.YLength,1)} x {round(bb.ZLength,1)} at ({round(bb.Center.x,1)}, {round(bb.Center.y,1)}, {round(bb.Center.z,1)})")

FreeCAD.closeDocument(doc.Name)
