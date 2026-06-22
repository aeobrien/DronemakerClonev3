#!/usr/bin/env python3
"""
Faceplate v8 — fixes:
1. Screen cutout: 166 x 102mm (board size, not standoff span)
2. Display right-side-up (add 180° flip)
3. Display flush with panel (better depth calc)
4. Mount holes OUTSIDE cutout in solid panel
"""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'
display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'
output_faceplate = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v8.step'
output_assembly = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v8_assembly.step'

cut_depth = 10.0

def cut_circle(shape, pos, diameter, axis):
    r = diameter / 2.0
    start = pos - axis * cut_depth
    return shape.cut(Part.makeCylinder(r, cut_depth * 2, start, axis))

def cut_rectangle(shape, centre, width, height, axis, right_dir, up_dir):
    hw, hh = width / 2.0, height / 2.0
    pts = [
        centre - right_dir * hw - up_dir * hh,
        centre + right_dir * hw - up_dir * hh,
        centre + right_dir * hw + up_dir * hh,
        centre - right_dir * hw + up_dir * hh,
    ]
    wire = Part.makePolygon(pts + [pts[0]])
    face = Part.Face(wire)
    r = shape.cut(face.extrude(axis * cut_depth))
    return r.cut(face.extrude(axis * (-cut_depth)))

# ============================================================
# Load
# ============================================================

print("Loading...")
doc = FreeCAD.newDocument("Main")
Import.insert(input_path, doc.Name)
panel = doc.Objects[0].Shape

doc2 = FreeCAD.newDocument("Display")
Import.insert(display_path, doc2.Name)
display_shape = doc2.Objects[0].Shape

# ============================================================
# Directions
# ============================================================

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
    base_on_plane = base - slope_outward * dist
    return base_on_plane + slope_up * slope_offset_mm

# ============================================================
# Display geometry (native coords: X=horiz, Y=depth, Z=vertical)
# ============================================================

# From analysis:
# Mount holes: X=±78.5, Z=±57.5 (centred at 0,0), Y=12.05
# Board PCB: 165 x 100mm, centred at X=0, Z=-1.45
# Active pixels (datasheet): 154.21 x 85.92mm, centred at approx X=1.9
# Screen surface: Y=-3.4

# Board Z range: [-51.45, 48.55] (100mm)
board_height = 100.0
board_centre_z = (-51.45 + 48.55) / 2  # = -1.45

# Active pixel area centre offset
active_cx = 1.9  # offset in X from mount centre
active_pixel_width = 154.21

# Display position on slope
display_slope_offset = 82.0  # mount hole centre at this offset

# The BOARD centre is offset from mount centre by Z=-1.45 in native
# This means the board centre is 1.45mm below the mount centre on the slope
board_slope_offset = display_slope_offset + board_centre_z  # 82 + (-1.45) = 80.55

# Cutout: sized for the BOARD (not standoffs)
# Standoffs are outside the cutout and rest on the panel back
cutout_width = 166.0   # board 165mm + 1mm clearance
cutout_height = 102.0  # board 100mm + 2mm clearance
cutout_centre = point_on_outer_slope(0, board_slope_offset)

print(f"Board centre slope offset: {board_slope_offset:.1f}mm")
print(f"Cutout: {cutout_width:.0f} x {cutout_height:.0f}mm")

# Cutout edges
cutout_bottom = board_slope_offset - cutout_height / 2  # 80.55 - 51 = 29.55
cutout_top = board_slope_offset + cutout_height / 2     # 80.55 + 51 = 131.55

# Mount holes are at ±57.5mm from display_slope_offset (82)
# Bottom mounts: 82 - 57.5 = 24.5mm — BELOW cutout bottom (29.55) ✓ in solid panel
# Top mounts: 82 + 57.5 = 139.5mm — ABOVE cutout top (131.55) ✓ in solid panel
print(f"Cutout bottom: {cutout_bottom:.1f}mm, bottom mount: {display_slope_offset - 57.5:.1f}mm")
print(f"Cutout top: {cutout_top:.1f}mm, top mount: {display_slope_offset + 57.5:.1f}mm")
print(f"Both mounts in solid panel: ✓")

# Mount hole positions
mount_positions = []
for hx in [-78.5, 78.5]:
    for voff in [-57.5, 57.5]:
        mount_positions.append(point_on_outer_slope(hx, display_slope_offset + voff))

# ============================================================
# Encoder alignment to active pixel area
# ============================================================

active_pixel_left = active_cx - active_pixel_width / 2   # -75.205
active_pixel_right = active_cx + active_pixel_width / 2  # 79.005

encoder_leftmost_x = active_pixel_left + 15.1 / 2   # -67.655
encoder_rightmost_x = active_pixel_right - 15.1 / 2  # 71.455
encoder_span = encoder_rightmost_x - encoder_leftmost_x
encoder_spacing = encoder_span / 7
encoder_x_positions = [encoder_leftmost_x + i * encoder_spacing for i in range(8)]

# Encoder row position: below the cutout with clearance
encoder_slope_offset = cutout_bottom - 12.0  # 12mm below cutout

print(f"\nEncoder row: {encoder_slope_offset:.1f}mm from slope bottom")
print(f"  Gap to cutout: 12mm")
print(f"  Spacing: {encoder_spacing:.2f}mm")

encoder_positions = [point_on_outer_slope(x, encoder_slope_offset) for x in encoder_x_positions]

# ============================================================
# Other components
# ============================================================

pot_slope_offsets = [display_slope_offset + 25.0, display_slope_offset - 25.0]
pot_positions = []
for side_x in [-101.0, 101.0]:
    for off in pot_slope_offsets:
        pot_positions.append(point_on_outer_slope(side_x, off))

led_positions = []
for pot_off in pot_slope_offsets:
    for v in [7.0, -7.0]:
        led_positions.append(point_on_outer_slope(-116.0, pot_off + v))

pushbutton_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]

# ============================================================
# Cut holes
# ============================================================

print("\n=== CUTTING ===")
result = panel

result = cut_rectangle(result, cutout_centre, cutout_width, cutout_height, slope_outward, slope_right, slope_up)
for pos in mount_positions:
    result = cut_circle(result, pos, 3.3, slope_outward)
for pos in encoder_positions:
    result = cut_circle(result, pos, 9.5, slope_outward)
for pos in pot_positions:
    result = cut_circle(result, pos, 7.0, slope_outward)
for pos in led_positions:
    result = cut_circle(result, pos, 8.0, slope_outward)
for x in pushbutton_x:
    result = cut_circle(result, FreeCAD.Vector(x, flat_outer_y, -86.8), 16.0, flat_outward)

print(f"Exporting faceplate...")
Part.Shape.exportStep(result, output_faceplate)

# ============================================================
# Assembly
# ============================================================

print("\nBuilding assembly...")
components = []

# Encoders
for pos in encoder_positions:
    components.append(Part.makeCylinder(16.0/2, 6.5, pos + slope_inward * panel_thickness, slope_inward))
    components.append(Part.makeCylinder(9.0/2, 5.4, pos, slope_outward))
    components.append(Part.makeCylinder(6.0/2, 12.0, pos + slope_outward * 5.4, slope_outward))

# Buttons
for x in pushbutton_x:
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

# Display — rotate to slope, flip right-side-up, position flush
print("Positioning display...")
rotated = display_shape.copy()

# Two rotations:
# 1. -60° around X to tilt screen to match 30° slope
# 2. 180° around the slope normal to flip right-side-up
# Combined: first -60° around X, then 180° around the resulting normal

rot1 = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -60)
rot2 = FreeCAD.Rotation(slope_outward, 180)
combined_rot = rot2.multiply(rot1)

rotated.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), combined_rot)

# The mount hole centre in native is at (0, 12.05, 0).
# After combined rotation, we need to find where it ended up.
mount_native = FreeCAD.Vector(0, 12.05, 0)
mount_rotated = combined_rot.multVec(mount_native)
print(f"Mount centre after rotation: ({mount_rotated.x:.2f}, {mount_rotated.y:.2f}, {mount_rotated.z:.2f})")

# Translate so rotated mount centre goes to display_mount_centre
display_mount_centre = point_on_outer_slope(0, display_slope_offset)
translation = display_mount_centre - mount_rotated
rotated.translate(translation)

# Adjust depth: screen surface was at native (0, -3.4, 0)
screen_native = FreeCAD.Vector(0, -3.4, 0)
screen_rotated = combined_rot.multVec(screen_native)
# After translation, screen surface is at display_mount_centre + (screen_rotated - mount_rotated)
screen_offset_from_mount = screen_rotated - mount_rotated
# This offset projected onto slope_outward tells us how far the screen face is from the panel
screen_depth = screen_offset_from_mount.dot(slope_outward)
print(f"Screen face depth from panel: {screen_depth:.2f}mm (positive = outward, negative = behind)")

# Shift so screen face is flush with panel (at depth = 0)
rotated.translate(slope_inward * screen_depth)

components.append(rotated)

rbb = rotated.BoundBox
print(f"Display bbox: X=[{rbb.XMin:.1f},{rbb.XMax:.1f}] Y=[{rbb.YMin:.1f},{rbb.YMax:.1f}] Z=[{rbb.ZMin:.1f},{rbb.ZMax:.1f}]")

# Combine
print("Combining...")
combined = result
for comp in components:
    try:
        combined = combined.fuse(comp)
    except:
        pass

print(f"Exporting assembly...")
Part.Shape.exportStep(combined, output_assembly)

# Verify
doc3 = FreeCAD.newDocument("V")
Import.insert(output_faceplate, doc3.Name)
from collections import defaultdict
bd = defaultdict(int)
for f in doc3.Objects[0].Shape.Faces:
    if isinstance(f.Surface, Part.Cylinder):
        bd[round(f.Surface.Radius*2, 1)] += 1
print("\nFaceplate holes:")
for d in sorted(bd.keys()):
    print(f"  {d}mm: {bd[d]}")

print("\nDone!")
