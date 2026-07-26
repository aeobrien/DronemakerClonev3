#!/usr/bin/env python3
"""
Faceplate v7 — positions derived from display model geometry.

Strategy:
1. Place the display at the desired position on the slope
2. Extract its mounting holes, active area, and board outline IN slope coordinates
3. Use those exact positions for cutout and mounting holes
4. Align encoder row to the active display area edges
"""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import
import math

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'
display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'
output_faceplate = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v7.step'
output_assembly = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v7_assembly.step'

# ============================================================
# Helpers
# ============================================================

cut_depth = 10.0

def cut_circle(shape, pos, diameter, axis):
    r = diameter / 2.0
    start = pos - axis * cut_depth
    return shape.cut(Part.makeCylinder(r, cut_depth * 2, start, axis))

def cut_rectangle(shape, centre, width, height, axis, right_dir, up_dir):
    hw, hh = width / 2.0, height / 2.0
    p1 = centre - right_dir * hw - up_dir * hh
    p2 = centre + right_dir * hw - up_dir * hh
    p3 = centre + right_dir * hw + up_dir * hh
    p4 = centre - right_dir * hw + up_dir * hh
    wire = Part.makePolygon([p1, p2, p3, p4, p1])
    face = Part.Face(wire)
    r = shape.cut(face.extrude(axis * cut_depth))
    return r.cut(face.extrude(axis * (-cut_depth)))

# ============================================================
# Load models
# ============================================================

print("Loading models...")
doc = FreeCAD.newDocument("Main")
Import.insert(input_path, doc.Name)
panel = doc.Objects[0].Shape

doc2 = FreeCAD.newDocument("Display")
Import.insert(display_path, doc2.Name)
display_shape = doc2.Objects[0].Shape

# ============================================================
# Directions and surfaces
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
    a = face.Area
    if abs(n.y + 1.0) < 0.01 and abs(n.x) < 0.01 and abs(n.z) < 0.01 and a > 5000:
        flat_ys.append(face.BoundBox.Center.y)
    if abs(n.y + 0.5) < 0.1 and abs(n.z - 0.866) < 0.1 and a > 10000:
        c = face.BoundBox.Center
        slope_dots.append((c.y * slope_outward.y + c.z * slope_outward.z, face))

flat_outer_y = min(flat_ys)
slope_outer_face = max(slope_dots, key=lambda x: x[0])[1]
slope_plane = slope_outer_face.Surface

print(f"Flat outer Y: {flat_outer_y:.2f}")

def point_on_outer_slope(x, slope_offset_mm):
    base_y = slope_outer_face.BoundBox.YMin
    base_z = slope_outer_face.BoundBox.ZMin
    base = FreeCAD.Vector(x, base_y, base_z)
    p0 = slope_plane.Position
    dist = (base - p0).dot(slope_outward)
    base_on_plane = base - slope_outward * dist
    return base_on_plane + slope_up * slope_offset_mm

# ============================================================
# DISPLAY: extract geometry, then position on slope
# ============================================================

print("\nExtracting display geometry...")

# Display native coords (X=horiz, Y=depth, Z=vertical):
# Mount holes at X=±78.5, Z=±57.5 (centred on 0,0)
# Active area: 158x90mm centred at X=1.9, Z=-5.15
# Board: 165x100mm centred at X=0, Z=-1.45
# Screen surface at Y=-3.4

# Active area bounds in native coords:
active_cx, active_cz = 1.9, -5.15
active_w, active_h = 158.0, 90.0
active_left = active_cx - active_w / 2    # = 1.9 - 79 = -77.1
active_right = active_cx + active_w / 2   # = 1.9 + 79 = 80.9
active_bottom = active_cz - active_h / 2  # = -5.15 - 45 = -50.15
active_top = active_cz + active_h / 2     # = -5.15 + 45 = 39.85

print(f"Active area: X=[{active_left:.1f}, {active_right:.1f}], Z=[{active_bottom:.1f}, {active_top:.1f}]")
print(f"Active area centre offset from mount centre: X={active_cx:.1f}, Z={active_cz:.1f}")

# When we place the display on the slope:
# - Native X maps to slope X (horizontal)
# - Native Z maps to slope_up direction (up the slope)
# - Native -Y maps to slope_outward (screen faces outward)

# We'll position the display so its mount hole centre is at a chosen point on the slope.
# Then the active area, board edges, etc. are offset from that.

# Choose display position: mount hole centre at slope_offset = 82mm, X = 0
display_slope_offset = 82.0
display_mount_centre = point_on_outer_slope(0, display_slope_offset)

print(f"\nDisplay mount centre on slope: Y={display_mount_centre.y:.1f}, Z={display_mount_centre.z:.1f}")

# Compute all display features in slope coordinates
# Mount holes: ±78.5 in X, ±57.5 in slope_up from mount centre
mount_positions = []
for hx in [-78.5, 78.5]:
    for voff in [-57.5, 57.5]:
        pos = point_on_outer_slope(hx, display_slope_offset + voff)
        mount_positions.append(pos)

print(f"Mount holes:")
for p in mount_positions:
    print(f"  X={p.x:.1f}, Y={p.y:.1f}, Z={p.z:.1f}")

# Active area edges in slope coordinates
# Active area is offset from mount centre by (1.9, -5.15) in (X, Z_native)
# Z_native maps to slope_up, so active centre is at slope_offset + (-5.15) up
active_slope_centre_x = active_cx  # 1.9mm offset in X
active_slope_centre_offset = display_slope_offset + active_cz  # 82 + (-5.15) = 76.85mm

# Active area edges
active_left_x = active_left    # -77.1mm in X
active_right_x = active_right  # 80.9mm in X
active_bottom_slope = display_slope_offset + active_bottom  # 82 + (-50.15) = 31.85mm
active_top_slope = display_slope_offset + active_top        # 82 + 39.85 = 121.85mm

print(f"\nActive area on slope:")
print(f"  X: [{active_left_x:.1f}, {active_right_x:.1f}] (width {active_w:.0f}mm)")
print(f"  Slope offset: [{active_bottom_slope:.1f}, {active_top_slope:.1f}] (height {active_h:.0f}mm)")

# Board cutout: needs to accommodate the full board + standoffs
# Board: 165x100mm. Standoffs span 157x115mm.
# The board Z range in native: [-51.45, 48.55] (100mm, centred at -1.45)
# Standoffs Z range: [-57.5, 57.5] (115mm, centred at 0)
# We need the cutout to accommodate whichever is larger at each edge

board_bottom_native = -51.45
board_top_native = 48.55
standoff_bottom_native = -57.5
standoff_top_native = 57.5

# Use the outermost extent
cutout_bottom_native = min(board_bottom_native, standoff_bottom_native) - 0.5  # -58.0
cutout_top_native = max(board_top_native, standoff_top_native) + 0.5          # 58.0
cutout_height = cutout_top_native - cutout_bottom_native  # 116mm

# Board is 165mm wide, add 1mm clearance
cutout_width = 166.0

# Cutout centre in native Z
cutout_centre_native_z = (cutout_bottom_native + cutout_top_native) / 2  # 0.0
cutout_slope_offset = display_slope_offset + cutout_centre_native_z

cutout_centre = point_on_outer_slope(0, cutout_slope_offset)  # X=0, board is centred on X

print(f"\nScreen cutout: {cutout_width:.0f} x {cutout_height:.0f}mm")
print(f"  Centre on slope at offset {cutout_slope_offset:.1f}mm")

# ============================================================
# ENCODER alignment to active display area
# ============================================================

# The active area X range is [-77.1, 80.9] — NOT symmetric (offset by 1.9mm)
# Encoder knobs (15.1mm dia) should have:
#   leftmost knob left edge = active_left_x = -77.1
#   rightmost knob right edge = active_right_x = 80.9

# But wait — the active area is 158mm wide, not 154.21mm as we assumed earlier.
# The 154.21mm was from the datasheet. The STEP model shows 158mm for Face 168.
# Face 168 might include the bezel, not just the lit pixels.
# The datasheet says active area is 154.21 x 85.92mm.
# The STEP model's 158x90mm face is probably the glass/window, not the pixel area.
# Let's use the datasheet value (154.21mm) for encoder alignment,
# but we need to know where that sits within the STEP model.

# The glass face (158mm) is centred at X=1.9. The active pixels (154.21mm)
# would be centred at approximately the same X offset.
# So active pixel area: centred at X=1.9, width 154.21mm
active_pixel_left = active_cx - 154.21 / 2   # 1.9 - 77.105 = -75.205
active_pixel_right = active_cx + 154.21 / 2  # 1.9 + 77.105 = 79.005

print(f"\nActive pixel area (154.21mm, from datasheet):")
print(f"  X: [{active_pixel_left:.2f}, {active_pixel_right:.2f}]")

# Encoder centres: leftmost knob left edge = active_pixel_left
# Knob radius = 15.1/2 = 7.55mm
encoder_leftmost_x = active_pixel_left + 7.55   # -67.655
encoder_rightmost_x = active_pixel_right - 7.55  # 71.455

# 7 gaps between 8 encoders
encoder_span = encoder_rightmost_x - encoder_leftmost_x  # 139.11mm
encoder_spacing = encoder_span / 7  # 19.87mm

encoder_x_positions = [encoder_leftmost_x + i * encoder_spacing for i in range(8)]
print(f"\nEncoder positions (accounting for display X offset):")
print(f"  Spacing: {encoder_spacing:.2f}mm c-to-c")
for i, x in enumerate(encoder_x_positions):
    print(f"  E{i+1}: X={x:.2f}")

# Encoder row: positioned just below the screen cutout
# Screen cutout bottom edge on slope:
cutout_bottom_slope = cutout_slope_offset - cutout_height / 2
encoder_slope_offset_mm = cutout_bottom_slope - 12.0  # 12mm gap below cutout

print(f"\nEncoder row at slope offset: {encoder_slope_offset_mm:.1f}mm")
print(f"  Gap from cutout bottom: 12.0mm")

encoder_positions = [point_on_outer_slope(x, encoder_slope_offset_mm) for x in encoder_x_positions]

# ============================================================
# Other components
# ============================================================

# Pots: flanking the screen
pot_slope_offsets = [display_slope_offset + 25.0, display_slope_offset - 25.0]

pot_positions = []
for side_x in [-101.0, 101.0]:
    for slope_off in pot_slope_offsets:
        pot_positions.append(point_on_outer_slope(side_x, slope_off))

# LEDs: to the left of each left pot, stacked vertically
led_positions = []
for pot_off in pot_slope_offsets:
    for v in [7.0, -7.0]:
        led_positions.append(point_on_outer_slope(-116.0, pot_off + v))

# Pushbuttons
pushbutton_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]

# ============================================================
# Print clearance summary
# ============================================================

encoder_top_edge = encoder_slope_offset_mm + 9.5 / 2
slope_length_approx = 144.0
mount_top_slope = display_slope_offset + 57.5
mount_bottom_slope = display_slope_offset - 57.5

print(f"\n=== CLEARANCE SUMMARY ===")
print(f"Slope length: ~{slope_length_approx:.0f}mm")
print(f"Encoder row: {encoder_slope_offset_mm:.1f}mm (top edge: {encoder_top_edge:.1f}mm)")
print(f"Screen cutout: {cutout_bottom_slope:.1f} to {cutout_bottom_slope + cutout_height:.1f}mm")
print(f"Mount holes: {mount_bottom_slope:.1f} to {mount_top_slope:.1f}mm")
print(f"Gap encoder→cutout: {cutout_bottom_slope - encoder_top_edge:.1f}mm")
print(f"Top mount→slope top: {slope_length_approx - mount_top_slope:.1f}mm")

# ============================================================
# Cut holes
# ============================================================

print("\n=== CUTTING ===")
result = panel

# Screen cutout
print(f"Screen cutout: {cutout_width:.0f} x {cutout_height:.0f}mm")
result = cut_rectangle(result, cutout_centre, cutout_width, cutout_height, slope_outward, slope_right, slope_up)

# Screen mounting holes (3.3mm from display model)
print("Screen mount holes: 4 x 3.3mm")
for pos in mount_positions:
    result = cut_circle(result, pos, 3.3, slope_outward)

# Encoders
print(f"Encoders: 8 x 9.5mm at {encoder_spacing:.2f}mm spacing")
for pos in encoder_positions:
    result = cut_circle(result, pos, 9.5, slope_outward)

# Pots
print("Pots: 4 x 7.0mm")
for pos in pot_positions:
    result = cut_circle(result, pos, 7.0, slope_outward)

# LEDs
print("LEDs: 4 x 8.0mm")
for pos in led_positions:
    result = cut_circle(result, pos, 8.0, slope_outward)

# Pushbuttons
print("Pushbuttons: 8 x 16.0mm")
for x in pushbutton_x:
    pos = FreeCAD.Vector(x, flat_outer_y, -86.8)
    result = cut_circle(result, pos, 16.0, flat_outward)

print(f"\nExporting faceplate to {output_faceplate}...")
Part.Shape.exportStep(result, output_faceplate)

# ============================================================
# Assembly: components + display
# ============================================================

print("\nBuilding assembly...")
components = []

# Encoders (bushing + shaft above, body below)
for pos in encoder_positions:
    # Body behind panel: 16mm dia, 6.5mm
    body_start = pos + slope_inward * panel_thickness
    components.append(Part.makeCylinder(16.0/2, 6.5, body_start, slope_inward))
    # Bushing above: 9mm dia, 5.4mm above panel
    components.append(Part.makeCylinder(9.0/2, 5.4, pos, slope_outward))
    # Shaft: 6mm dia, 12mm above bushing
    components.append(Part.makeCylinder(6.0/2, 12.0, pos + slope_outward * 5.4, slope_outward))

# Buttons (cap outside, body inside)
for x in pushbutton_x:
    outer = FreeCAD.Vector(x, flat_outer_y, -86.8)
    components.append(Part.makeCylinder(18.5/2, 1.5, outer, flat_outward))  # cap
    components.append(Part.makeCylinder(16.0/2, 20.0, outer, flat_inward))  # body

# Pots (shaft above, body below)
for pos in pot_positions:
    components.append(Part.makeCylinder(6.0/2, 8.0, pos, slope_outward))
    body_start = pos + slope_inward * panel_thickness
    components.append(Part.makeCylinder(9.0/2, 5.0, body_start, slope_inward))

# LEDs (flange above only)
for pos in led_positions:
    components.append(Part.makeCylinder(8.0/2, 2.0, pos, slope_outward))

# Display model — rotate and position
print("Positioning display...")
rotated = display_shape.copy()
rotated.Placement = FreeCAD.Placement(
    FreeCAD.Vector(0, 0, 0),
    FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -60)
)

# After rotation, the mount hole centre (which was at native X=0, Z=0)
# needs to go to display_mount_centre on the slope.
# The display's native origin (0,0,0) is approximately the mount hole centre.
rbb = rotated.BoundBox
# The mount holes were at native X=0, Z=0 (average), Y=12.05
# After -60° rotation around X:
# X stays 0
# Y' = 12.05 * cos(-60) - 0 * sin(-60) = 12.05 * 0.5 = 6.025
# Z' = 12.05 * sin(-60) + 0 * cos(-60) = 12.05 * (-0.866) = -10.44
# So the mount centre after rotation is at approximately (0, 6.025, -10.44)

mount_after_rot = FreeCAD.Vector(0, 12.05 * 0.5, 12.05 * (-0.866))
translation = display_mount_centre - mount_after_rot
rotated.translate(translation)

# The screen surface was at Y=-3.4 in native.
# After rotation: Y' = -3.4 * cos(-60) = -3.4 * 0.5 = -1.7
#                 Z' = -3.4 * sin(-60) = -3.4 * (-0.866) = 2.944
# So screen surface after rotation is offset by (0, -1.7, 2.944) from native origin.
# After translation to display_mount_centre, screen surface is at:
# display_mount_centre + (0, -1.7, 2.944) — this should be approximately at the panel surface.
# We want the screen face flush with the OUTER panel surface.
# Fine-tune: shift along slope_outward to align screen face with panel.
# The panel outer surface passes through display_mount_centre (since we computed it on the outer slope).
# The screen face is at offset (0, -1.7, 2.944) from mount centre.
# The dot product of this offset with slope_outward tells us how far the screen face is from the panel:
screen_offset = FreeCAD.Vector(0, -1.7, 2.944)
screen_dist_from_panel = screen_offset.dot(slope_outward)
print(f"Screen face offset from panel: {screen_dist_from_panel:.2f}mm (positive = outward)")
# Shift inward by that amount to make flush
rotated.translate(slope_inward * screen_dist_from_panel)

components.append(rotated)

rbb2 = rotated.BoundBox
print(f"Display final bbox: X=[{rbb2.XMin:.1f},{rbb2.XMax:.1f}] Y=[{rbb2.YMin:.1f},{rbb2.YMax:.1f}] Z=[{rbb2.ZMin:.1f},{rbb2.ZMax:.1f}]")

# Combine
print("Combining...")
combined = result
for comp in components:
    try:
        combined = combined.fuse(comp)
    except:
        pass

print(f"Exporting assembly to {output_assembly}...")
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
