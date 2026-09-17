#!/usr/bin/env python3
"""Faceplate v6 — correct surfaces, contiguous components, display-matched cutout."""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'
display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'
output_faceplate = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v6.step'
output_assembly = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v6_assembly.step'

# ============================================================
# Helper functions (defined first)
# ============================================================

cut_depth = 10.0

def cut_circle(shape, pos, diameter, axis):
    r = diameter / 2.0
    start = pos - axis * cut_depth
    cyl = Part.makeCylinder(r, cut_depth * 2, start, axis)
    return shape.cut(cyl)

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

# ============================================================
# Find OUTER surfaces
# ============================================================

# Flat face: find both faces with normal (0,-1,0) and area > 5000
flat_ys = []
for face in panel.Faces:
    if isinstance(face.Surface, Part.Plane):
        n = face.Surface.Axis
        if abs(n.y + 1.0) < 0.01 and abs(n.x) < 0.01 and abs(n.z) < 0.01 and face.Area > 5000:
            flat_ys.append(face.BoundBox.Center.y)

flat_outer_y = min(flat_ys)  # most negative Y = outermost
flat_inner_y = max(flat_ys)
print(f"Flat face: outer Y={flat_outer_y:.2f}, inner Y={flat_inner_y:.2f}")

flat_z_centre = -86.8

# Slope face: find outer surface
slope_faces_data = []
for face in panel.Faces:
    if isinstance(face.Surface, Part.Plane):
        n = face.Surface.Axis
        if abs(n.y + 0.5) < 0.1 and abs(n.z - 0.866) < 0.1 and face.Area > 10000:
            c = face.BoundBox.Center
            dot = c.y * slope_outward.y + c.z * slope_outward.z
            slope_faces_data.append((dot, face))

slope_faces_data.sort()
slope_outer_face = slope_faces_data[-1][1]  # highest dot product = furthest in outward direction
slope_plane = slope_outer_face.Surface

print(f"Slope outer face: Y range {slope_outer_face.BoundBox.YMin:.1f} to {slope_outer_face.BoundBox.YMax:.1f}")

def point_on_outer_slope(x, slope_offset_mm):
    """Point on the OUTER slope surface, offset mm up from the bottom edge."""
    # Start at the bottom of the outer slope
    base_y = slope_outer_face.BoundBox.YMin
    base_z = slope_outer_face.BoundBox.ZMin
    base = FreeCAD.Vector(x, base_y, base_z)

    # Project onto the plane
    p0 = slope_plane.Position
    n = slope_outward
    diff = base - p0
    dist_to_plane = diff.dot(n)
    base_on_plane = base - n * dist_to_plane

    return base_on_plane + slope_up * slope_offset_mm

# ============================================================
# Extract display info
# ============================================================

# Mounting holes (r≈1.65mm = 3.3mm dia M3)
display_mounts = []
for face in display_shape.Faces:
    if isinstance(face.Surface, Part.Cylinder) and abs(face.Surface.Radius - 1.65) < 0.1:
        c = face.Surface.Center
        dup = any(abs(c.x - e.x) < 1 and abs(c.z - e.z) < 1 for e in display_mounts)
        if not dup:
            display_mounts.append(c)

xs = sorted(set(round(c.x, 0) for c in display_mounts))
zs = sorted(set(round(c.z, 0) for c in display_mounts))
mount_h_spacing = xs[-1] - xs[0]  # 157mm
mount_v_spacing = zs[-1] - zs[0]  # 115mm
print(f"Display mount spacing: {mount_h_spacing:.0f} x {mount_v_spacing:.0f}mm")

# Display bbox for cutout sizing
dbb = display_shape.BoundBox
print(f"Display bbox: {dbb.XLength:.1f} x {dbb.YLength:.1f} x {dbb.ZLength:.1f}mm")

# Cutout: accommodate full display including standoffs
cutout_width = dbb.XLength + 1.0   # 167mm
cutout_height = mount_v_spacing + 1.0  # 116mm (standoffs need to pass through)
print(f"Cutout: {cutout_width:.0f} x {cutout_height:.0f}mm")

# ============================================================
# Layout
# ============================================================

encoder_slope_offset = 10.0
screen_slope_offset = 82.0  # raised slightly to give more clearance from encoders

# Check clearances
screen_bottom = screen_slope_offset - cutout_height / 2
encoder_top = encoder_slope_offset + 9.5 / 2
screen_top = screen_slope_offset + cutout_height / 2
print(f"\nClearances:")
print(f"  Encoder top edge: {encoder_top:.1f}mm")
print(f"  Screen cutout bottom: {screen_bottom:.1f}mm")
print(f"  Gap: {screen_bottom - encoder_top:.1f}mm")
print(f"  Screen cutout top: {screen_top:.1f}mm (slope ≈ 144mm)")

pot_slope_offsets = [screen_slope_offset + 25.0, screen_slope_offset - 25.0]
encoder_new_x = [-69.555 + i * 19.87 for i in range(8)]
pushbutton_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]

# ============================================================
# Cut holes
# ============================================================

print("\n=== CUTTING ===")
result = panel

# Screen cutout
screen_centre = point_on_outer_slope(0, screen_slope_offset)
result = cut_rectangle(result, screen_centre, cutout_width, cutout_height, slope_outward, slope_right, slope_up)

# Screen mounting holes (from display model: 3.3mm dia)
mount_h_half = mount_h_spacing / 2
mount_v_half = mount_v_spacing / 2
screen_mounts = []
for v_sign in [-1, 1]:
    for h_sign in [-1, 1]:
        pos = point_on_outer_slope(h_sign * mount_h_half, screen_slope_offset + v_sign * mount_v_half)
        screen_mounts.append(pos)
        result = cut_circle(result, pos, 3.3, slope_outward)

# Encoders: 8 x 9.5mm
encoder_positions = [point_on_outer_slope(x, encoder_slope_offset) for x in encoder_new_x]
for pos in encoder_positions:
    result = cut_circle(result, pos, 9.5, slope_outward)

# Pots: 4 x 7.0mm
pot_positions = []
for side_x in [-101.0, 101.0]:
    for slope_off in pot_slope_offsets:
        pos = point_on_outer_slope(side_x, slope_off)
        pot_positions.append(pos)
        result = cut_circle(result, pos, 7.0, slope_outward)

# LEDs: 4 x 8.0mm
led_positions = []
for pot_off in pot_slope_offsets:
    for v in [7.0, -7.0]:
        pos = point_on_outer_slope(-116.0, pot_off + v)
        led_positions.append(pos)
        result = cut_circle(result, pos, 8.0, slope_outward)

# Pushbuttons: 8 x 16mm — ON THE OUTER SURFACE
for x in pushbutton_x:
    pos = FreeCAD.Vector(x, flat_outer_y, flat_z_centre)
    result = cut_circle(result, pos, 16.0, flat_outward)

print(f"Exporting faceplate...")
Part.Shape.exportStep(result, output_faceplate)

# ============================================================
# Component models
# ============================================================

print("\nBuilding component models...")
components = []

# ENCODERS — single contiguous shape per encoder
for pos in encoder_positions:
    # One cylinder from behind the panel, through the hole, out the front
    # Behind panel: body 16mm dia, 6.5mm deep from inner surface
    # Through panel: bushing 9mm dia (but hole is 9.5mm, so bushing fits)
    # Above panel: bushing 9mm dia continues ~5.4mm, then shaft 6mm for 12mm

    # Total outward from outer surface: bushing ~5.4mm + shaft 12mm = ~17.4mm
    # Total inward from outer surface: panel 1.63mm + body 6.5mm = 8.13mm

    # Body behind panel (inward from outer surface)
    body_depth = 6.5
    body_start = pos + slope_inward * (panel_thickness + body_depth)
    body = Part.makeCylinder(16.0/2, body_depth, body_start, slope_outward)
    components.append(body)

    # Bushing through panel and above: 9mm dia
    # From inner surface to 5.4mm above outer surface = panel_thickness + 5.4 = 7.03mm
    bushing_total = panel_thickness + 5.4  # through panel + above
    bushing_start = pos + slope_inward * panel_thickness
    bushing = Part.makeCylinder(9.0/2, bushing_total, bushing_start, slope_outward)
    components.append(bushing)

    # Shaft: 6mm dia, 12mm above the bushing top
    shaft_start = pos + slope_outward * 5.4  # top of bushing
    shaft = Part.makeCylinder(6.0/2, 12.0, shaft_start, slope_outward)
    components.append(shaft)

# PUSHBUTTONS — through the panel
for x in pushbutton_x:
    outer_pos = FreeCAD.Vector(x, flat_outer_y, flat_z_centre)

    # Cap: Ø18.5, 1.5mm outward from outer surface
    cap = Part.makeCylinder(18.5/2, 1.5, outer_pos, flat_outward)
    components.append(cap)

    # Body: Ø16, from outer surface inward 21.5 - 1.5 = 20mm
    body = Part.makeCylinder(16.0/2, 20.0, outer_pos, flat_inward)
    components.append(body)

# POTS
for pos in pot_positions:
    shaft = Part.makeCylinder(6.0/2, 8.0, pos, slope_outward)
    components.append(shaft)
    body_start = pos + slope_inward * (panel_thickness + 5.0)
    body = Part.makeCylinder(9.0/2, 5.0, body_start, slope_outward)
    components.append(body)

# LED BEZELS
for pos in led_positions:
    flange = Part.makeCylinder(8.0/2, 2.0, pos, slope_outward)
    components.append(flange)

# DISPLAY — actual Waveshare model
print("Positioning display...")
rotated_display = display_shape.copy()
rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -60)
rotated_display.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), rotation)

rbb = rotated_display.BoundBox
rot_centre = FreeCAD.Vector(
    (rbb.XMin + rbb.XMax) / 2,
    (rbb.YMin + rbb.YMax) / 2,
    (rbb.ZMin + rbb.ZMax) / 2
)

# Translate to screen centre
rotated_display.translate(screen_centre - rot_centre)

# Adjust depth: screen face should be flush with outer panel surface
# In native coords, screen face was at Y=-3.4, bbox centre Y = 3.65
# So screen face is 7.05mm from centre in the -Y direction (= slope_outward after rotation)
# After centering, screen face is at screen_centre + slope_outward * 7.05
# We want it AT screen_centre (the outer panel surface), so shift inward by 7.05
rotated_display.translate(slope_inward * 7.05)

components.append(rotated_display)

rbb2 = rotated_display.BoundBox
print(f"Display bbox: X=[{rbb2.XMin:.1f}, {rbb2.XMax:.1f}] Y=[{rbb2.YMin:.1f}, {rbb2.YMax:.1f}] Z=[{rbb2.ZMin:.1f}, {rbb2.ZMax:.1f}]")

# ============================================================
# Export assembly
# ============================================================

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
print("\nVerifying faceplate holes:")
doc3 = FreeCAD.newDocument("V")
Import.insert(output_faceplate, doc3.Name)
from collections import defaultdict
bd = defaultdict(int)
for f in doc3.Objects[0].Shape.Faces:
    if isinstance(f.Surface, Part.Cylinder):
        bd[round(f.Surface.Radius*2, 1)] += 1
for d in sorted(bd.keys()):
    print(f"  {d}mm: {bd[d]}")

print("\nDone!")
