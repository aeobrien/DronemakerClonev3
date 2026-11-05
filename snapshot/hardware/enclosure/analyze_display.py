#!/usr/bin/env python3
"""Analyze the Waveshare display STEP model to determine cutout size."""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'

doc = FreeCAD.newDocument("Display")
Import.insert(display_path, doc.Name)
shape = doc.Objects[0].Shape

bb = shape.BoundBox
print(f"Overall bounding box:")
print(f"  X: {bb.XMin:.2f} to {bb.XMax:.2f} = {bb.XLength:.2f}mm")
print(f"  Y: {bb.YMin:.2f} to {bb.YMax:.2f} = {bb.YLength:.2f}mm")
print(f"  Z: {bb.ZMin:.2f} to {bb.ZMax:.2f} = {bb.ZLength:.2f}mm")

# The display might be oriented with:
# - Longest dimension (166mm) = board width
# - Medium dimension (124mm) = board height + standoffs + connector
# - Shortest dimension (14mm) = board depth + standoffs

# Find all faces and their sizes to understand the structure
print(f"\nTotal faces: {len(shape.Faces)}")
print(f"Total solids: {len(shape.Solids)}")

# Look for the largest flat faces (likely the screen surface and back)
print("\nLargest planar faces (likely screen surface and board back):")
flat_faces = []
for i, face in enumerate(shape.Faces):
    if isinstance(face.Surface, Part.Plane) and face.Area > 500:
        n = face.Surface.Axis
        fbb = face.BoundBox
        flat_faces.append((face.Area, i, n, fbb))

flat_faces.sort(reverse=True)
for area, idx, normal, fbb in flat_faces[:10]:
    print(f"  Face {idx}: area={area:.0f}mm² normal=({normal.x:.2f},{normal.y:.2f},{normal.z:.2f}) bbox=({fbb.XLength:.1f}x{fbb.YLength:.1f}x{fbb.ZLength:.1f}) centre=({fbb.Center.x:.1f},{fbb.Center.y:.1f},{fbb.Center.z:.1f})")

# Look for cylindrical surfaces (mounting hole locations)
print("\nCylindrical surfaces (mounting holes/standoffs):")
cyls = []
for i, face in enumerate(shape.Faces):
    if isinstance(face.Surface, Part.Cylinder):
        r = face.Surface.Radius
        c = face.Surface.Center
        a = face.Surface.Axis
        if r > 1.0:  # ignore tiny features
            cyls.append((r, i, c, a))

cyls.sort(reverse=True)
for r, idx, centre, axis in cyls[:20]:
    print(f"  Face {idx}: r={r:.2f}mm (d={r*2:.2f}) centre=({centre.x:.1f},{centre.y:.1f},{centre.z:.1f}) axis=({axis.x:.2f},{axis.y:.2f},{axis.z:.2f})")

FreeCAD.closeDocument(doc.Name)
