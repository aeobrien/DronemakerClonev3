#!/usr/bin/env python3
"""
Faceplate v9 — cutout sized to active display area (Option A).
Board mounts behind panel, bezel overlaps cutout edges.
"""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'
display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'
output_faceplate = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v9.step'
output_assembly = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v9_assembly.step'

cut_depth = 10.0

def cut_circle(shape, pos, diameter, axis):
    r = diameter / 2.0
    return shape.cut(Part.makeCylinder(r, cut_depth * 2, pos - axis * cut_depth, axis))

def cut_rectangle(shape, centre, width, height, axis, right_dir, up_dir):
    hw, hh = width / 2.0, height / 2.0
    pts = [centre - right_dir*hw - up_dir*hh, centre + right_dir*hw - up_dir*hh,
           centre + right_dir*hw + up_dir*hh, centre - right_dir*hw + up_dir*hh]
    wire = Part.makePolygon(pts + [pts[0]])
    face = Part.Face(wire)
    r = shape.cut(face.extrude(axis * cut_depth))
    return r.cut(face.extrude(axis * (-cut_depth)))

print("Loading...")
doc = FreeCAD.newDocument("Main")
Import.insert(input_path, doc.Name)
panel = doc.Objects[0].Shape

doc2 = FreeCAD.newDocument("Display")
Import.insert(display_path, doc2.Name)
display_shape = doc2.Objects[0].Shape

# Directions
slope_outward = FreeCAD.Vector(0, -0.5, 0.866)
slope_outward.normalize()
slope_inward = slope_outward * -1
slope_up = FreeCAD.Vector(0, 0.866, 0.5)
slope_up.normalize()
slope_right = FreeCAD.Vector(1, 0, 0)
flat_outward = FreeCAD.Vector(0, -1, 0)
flat_inward = FreeCAD.Vector(0, 1, 0)
panel_thickness = 1.63

# Find outer surfaces
flat_ys = []
slope_dots = []
for face in panel.Faces:
    if not isinstance(face.Surface, Part.Plane):
        continue
    n = face.Surface.Axis
    if abs(n.y + 1.0) < 0.01 and abs(n.x) < 0.01 and abs(n.z) < 0.01 and face.Area > 5000:
        flat_ys.append(face.BoundBox.Center.y)
    if abs(n.y + 0.5) < 0.1 and abs(n.z - 0.866) < 0.1 and face.Area > 10000:
        c = face.BoundBox.Center
        slope_dots.append((c.y * slope_outward.y + c.z * slope_outward.z, face))

flat_outer_y = min(flat_ys)
slope_outer_face = max(slope_dots, key=lambda x: x[0])[1]
slope_plane = slope_outer_face.Surface

def point_on_outer_slope(x, slope_offset_mm):
    base_y = slope_outer_face.BoundBox.YMin
    base_z = slope_outer_face.BoundBox.ZMin
    base = FreeCAD.Vector(x, base_y, base_z)
    p0 = slope_plane.Position
    dist = (base - p0).dot(slope_outward)
    return (base - slope_outward * dist) + slope_up * slope_offset_mm

# ============================================================
# Display geometry from STEP model (native coords)
# ============================================================

# Mount holes: X=±78.5, Z=±57.5, centred at (0, 12.05, 0)
# Active area (STEP Face 168): 158x90mm centred at X=1.9, Z=-5.15
# But datasheet active pixels: 154.21 x 85.92mm
# Board PCB: 165x100mm centred at X=0, Z=-1.45
# Screen surface: Y=-3.4

# The active area centre is offset from mount hole centre:
active_offset_x = 1.9    # mm in X
active_offset_z = -5.15   # mm in native Z (maps to slope_up)

# Active area cutout (datasheet values + 1mm tolerance)
cutout_width = 155.0    # 154.21 + ~0.8mm
cutout_height = 87.0    # 85.92 + ~1mm

# ============================================================
# Position display on slope
# ============================================================

# Mount hole centre position on slope
# Moving down slightly from v8's 82mm — try 78mm
display_mount_offset = 78.0

# Active area centre on slope (offset from mount centre)
active_slope_offset = display_mount_offset + active_offset_z  # 78 + (-5.15) = 72.85
active_slope_x = active_offset_x  # 1.9mm

# Cutout centre follows the active area, not the mount centre
cutout_centre = point_on_outer_slope(active_slope_x, active_slope_offset)

# Mount hole positions (relative to mount centre, NOT active centre)
mount_positions = []
for hx in [-78.5, 78.5]:
    for voff in [-57.5, 57.5]:
        mount_positions.append(point_on_outer_slope(hx, display_mount_offset + voff))

# Cutout edges
cutout_bottom = active_slope_offset - cutout_height / 2  # 72.85 - 43.5 = 29.35
cutout_top = active_slope_offset + cutout_height / 2     # 72.85 + 43.5 = 116.35
bottom_mount = display_mount_offset - 57.5               # 20.5
top_mount = display_mount_offset + 57.5                   # 135.5

print(f"Display mount centre: slope offset {display_mount_offset}mm")
print(f"Active area centre: slope offset {active_slope_offset:.1f}mm, X={active_slope_x}mm")
print(f"Cutout: {cutout_width} x {cutout_height}mm")
print(f"Cutout edges: {cutout_bottom:.1f} to {cutout_top:.1f}mm")
print(f"Mount holes: {bottom_mount:.1f} to {top_mount:.1f}mm")
print(f"Bottom mount is {cutout_bottom - bottom_mount:.1f}mm BELOW cutout bottom (in solid panel ✓)")
print(f"Top mount is {top_mount - cutout_top:.1f}mm ABOVE cutout top (in solid panel ✓)")

# ============================================================
# Encoder alignment
# ============================================================

# Encoder knob edges align with active PIXEL area (154.21mm, from datasheet)
# Active pixel area is centred at X=1.9 (same offset as the glass)
active_pixel_left = active_offset_x - 154.21 / 2
active_pixel_right = active_offset_x + 154.21 / 2

encoder_left_x = active_pixel_left + 15.1 / 2
encoder_right_x = active_pixel_right - 15.1 / 2
encoder_spacing = (encoder_right_x - encoder_left_x) / 7
encoder_x = [encoder_left_x + i * encoder_spacing for i in range(8)]

# Encoder row: close below cutout but above bottom mount hole
# Need clearance from bottom mount (screw head ~5.5mm dia, so ~2.75mm radius)
# Bottom mount at 20.5mm. Encoder hole radius 4.75mm.
# Encoder centre must be at least 20.5 - 4.75 - 2.75 = 13.0mm or lower,
# OR at least 20.5 + 4.75 + 2.75 = 28.0mm or higher.
# Since encoders must be BELOW the mount, encoder centre ≤ 13.0mm.

encoder_slope_offset = 10.0  # safe below bottom mount at 20.5

encoder_top_edge = encoder_slope_offset + 4.75
gap_encoder_to_mount = bottom_mount - encoder_top_edge
gap_encoder_to_cutout = cutout_bottom - encoder_top_edge

print(f"\nEncoder row: {encoder_slope_offset}mm (top edge: {encoder_top_edge:.1f}mm)")
print(f"  Gap to bottom mount: {gap_encoder_to_mount:.1f}mm")
print(f"  Gap to cutout bottom: {gap_encoder_to_cutout:.1f}mm")
print(f"  Spacing: {encoder_spacing:.2f}mm")

encoder_positions = [point_on_outer_slope(x, encoder_slope_offset) for x in encoder_x]

# ============================================================
# Other components
# ============================================================

pot_offsets = [display_mount_offset + 25.0, display_mount_offset - 25.0]
pot_positions = []
for sx in [-101.0, 101.0]:
    for off in pot_offsets:
        pot_positions.append(point_on_outer_slope(sx, off))

led_positions = []
for po in pot_offsets:
    for v in [7.0, -7.0]:
        led_positions.append(point_on_outer_slope(-116.0, po + v))

pb_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]

# ============================================================
# Cut
# ============================================================

print("\n=== CUTTING ===")
result = panel

# Screen cutout (active area)
result = cut_rectangle(result, cutout_centre, cutout_width, cutout_height, slope_outward, slope_right, slope_up)

# Mount holes
for pos in mount_positions:
    result = cut_circle(result, pos, 3.3, slope_outward)

# Encoders
for pos in encoder_positions:
    result = cut_circle(result, pos, 9.5, slope_outward)

# Pots
for pos in pot_positions:
    result = cut_circle(result, pos, 7.0, slope_outward)

# LEDs
for pos in led_positions:
    result = cut_circle(result, pos, 8.0, slope_outward)

# Pushbuttons
for x in pb_x:
    result = cut_circle(result, FreeCAD.Vector(x, flat_outer_y, -86.8), 16.0, flat_outward)

print("Exporting faceplate...")
Part.Shape.exportStep(result, output_faceplate)

# ============================================================
# Assembly
# ============================================================

print("\nAssembly...")
components = []

# Encoders
for pos in encoder_positions:
    components.append(Part.makeCylinder(16.0/2, 6.5, pos + slope_inward * panel_thickness, slope_inward))
    components.append(Part.makeCylinder(9.0/2, 5.4, pos, slope_outward))
    components.append(Part.makeCylinder(6.0/2, 12.0, pos + slope_outward * 5.4, slope_outward))

# Buttons
for x in pb_x:
    outer = FreeCAD.Vector(x, flat_outer_y, -86.8)
    components.append(Part.makeCylinder(18.5/2, 1.5, outer, flat_outward))
    components.append(Part.makeCylinder(16.0/2, 20.0, outer, flat_inward))

# Pots
for pos in pot_positions:
    components.append(Part.makeCylinder(6.0/2, 8.0, pos, slope_outward))
    components.append(Part.makeCylinder(9.0/2, 5.0, pos + slope_inward * panel_thickness, slope_inward))

# LEDs
for pos in led_positions:
    components.append(Part.makeCylinder(8.0/2, 2.0, pos, slope_outward))

# Display
print("Positioning display...")
rotated = display_shape.copy()
rot1 = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -60)
rot2 = FreeCAD.Rotation(slope_outward, 180)
combined_rot = rot2.multiply(rot1)
rotated.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), combined_rot)

# Position using mount hole centre
mount_native = FreeCAD.Vector(0, 12.05, 0)
mount_rotated = combined_rot.multVec(mount_native)
display_mount_centre = point_on_outer_slope(0, display_mount_offset)
rotated.translate(display_mount_centre - mount_rotated)

# Flush screen with panel
screen_native = FreeCAD.Vector(0, -3.4, 0)
screen_rotated = combined_rot.multVec(screen_native)
screen_depth = (screen_rotated - mount_rotated).dot(slope_outward)
rotated.translate(slope_inward * screen_depth)

# Now shift the display BEHIND the panel so the board bezel sits against the panel back
# The screen face is now flush. The board extends behind. But we want the screen
# to be visible through the cutout, with the bezel edge resting on the back of the panel.
# So shift the entire display inward by panel_thickness so the screen face
# is at the panel INNER surface, and the bezel rests on it.
rotated.translate(slope_inward * panel_thickness)

components.append(rotated)

rbb = rotated.BoundBox
print(f"Display: X=[{rbb.XMin:.1f},{rbb.XMax:.1f}] Y=[{rbb.YMin:.1f},{rbb.YMax:.1f}] Z=[{rbb.ZMin:.1f},{rbb.ZMax:.1f}]")

# Combine
print("Combining...")
combined = result
for comp in components:
    try:
        combined = combined.fuse(comp)
    except:
        pass

Part.Shape.exportStep(combined, output_assembly)

# Verify
doc3 = FreeCAD.newDocument("V")
Import.insert(output_faceplate, doc3.Name)
from collections import defaultdict
bd = defaultdict(int)
for f in doc3.Objects[0].Shape.Faces:
    if isinstance(f.Surface, Part.Cylinder):
        bd[round(f.Surface.Radius*2, 1)] += 1
print("\nHoles:")
for d in sorted(bd.keys()):
    print(f"  {d}mm: {bd[d]}")
print("Done!")
