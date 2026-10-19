#!/usr/bin/env python3
"""Analyze the clean original panel to find face normals and positions."""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'

doc = FreeCAD.newDocument("Original")
Import.insert(input_path, doc.Name)
shape = doc.Objects[0].Shape

print(f"Shape: {len(shape.Faces)} faces, {len(shape.Edges)} edges, {len(shape.Solids)} solids")
print(f"Bounding box: X={round(shape.BoundBox.XMin,1)} to {round(shape.BoundBox.XMax,1)}, Y={round(shape.BoundBox.YMin,1)} to {round(shape.BoundBox.YMax,1)}, Z={round(shape.BoundBox.ZMin,1)} to {round(shape.BoundBox.ZMax,1)}")

print("\n=== ALL FACES ===")
for i, face in enumerate(shape.Faces):
    bb = face.BoundBox
    area = face.Area
    if isinstance(face.Surface, Part.Plane):
        n = face.Surface.Axis
        print(f"  Face {i}: PLANE normal=({n.x:.3f},{n.y:.3f},{n.z:.3f}) area={area:.1f}mm² bbox=({round(bb.XLength,1)} x {round(bb.YLength,1)} x {round(bb.ZLength,1)}) centre=({round(bb.Center.x,1)},{round(bb.Center.y,1)},{round(bb.Center.z,1)})")
    elif isinstance(face.Surface, Part.Cylinder):
        r = face.Surface.Radius
        print(f"  Face {i}: CYLINDER r={round(r,2)}mm centre=({round(bb.Center.x,1)},{round(bb.Center.y,1)},{round(bb.Center.z,1)})")
    else:
        stype = type(face.Surface).__name__
        print(f"  Face {i}: {stype} area={area:.1f}mm² bbox=({round(bb.XLength,1)} x {round(bb.YLength,1)} x {round(bb.ZLength,1)}) centre=({round(bb.Center.x,1)},{round(bb.Center.y,1)},{round(bb.Center.z,1)})")

# Identify key faces
print("\n=== KEY FACES FOR CUTTING ===")
for i, face in enumerate(shape.Faces):
    if not isinstance(face.Surface, Part.Plane):
        continue
    n = face.Surface.Axis
    area = face.Area

    # Angled face (30-degree slope, outward-facing)
    # Normal should be approximately (0, -0.5, 0.866) or (0, 0.5, -0.866)
    if abs(abs(n.y) - 0.5) < 0.1 and abs(abs(n.z) - 0.866) < 0.1 and area > 1000:
        print(f"  ANGLED FACE (slope): Face {i}, normal=({n.x:.3f},{n.y:.3f},{n.z:.3f}), area={area:.0f}mm²")

    # Flat bottom face (pushbuttons)
    # Normal should be (0, -1, 0) or (0, 1, 0)
    if abs(abs(n.y) - 1.0) < 0.01 and abs(n.x) < 0.01 and abs(n.z) < 0.01 and area > 1000:
        print(f"  FLAT FACE (bottom): Face {i}, normal=({n.x:.3f},{n.y:.3f},{n.z:.3f}), area={area:.0f}mm²")

    # Back panel face
    # Normal should be (0, 0, 1) or (0, 0, -1) and be at the rear
    if abs(abs(n.z) - 1.0) < 0.01 and abs(n.x) < 0.01 and abs(n.y) < 0.01 and area > 1000:
        print(f"  FLAT TOP/BACK: Face {i}, normal=({n.x:.3f},{n.y:.3f},{n.z:.3f}), area={area:.0f}mm², Z={round(face.BoundBox.Center.z,1)}")

FreeCAD.closeDocument(doc.Name)
