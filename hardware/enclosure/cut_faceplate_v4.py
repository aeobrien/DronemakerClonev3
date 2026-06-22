#!/usr/bin/env python3
"""
Faceplate v4 — fixes from v3:
1. Screen cutout sized to actual Waveshare board (165 x 103mm)
2. Buttons correctly positioned ON the flat face surface
3. Button barrel diameter = 16mm (matches hole), not 22mm
4. LED hole size TBD — keeping 8mm for now, will adjust when bezels arrive
"""

import sys
import math
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Part
import Import

input_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/FaceplateOriginal.step'
display_path = '/Users/aidan/Dev/DronemakerClonev3/hardware/components/display.stp'
output_faceplate = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v4.step'
output_assembly = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/Faceplate_v4_assembly.step'

print("Loading clean panel...")
doc = FreeCAD.newDocument("Faceplate")
Import.insert(input_path, doc.Name)
panel = doc.Objects[0].Shape

print("Loading display model...")
doc2 = FreeCAD.newDocument("Display")
Import.insert(display_path, doc2.Name)
display_shape = doc2.Objects[0].Shape

# ============================================================
# Geometry constants
# ============================================================

slope_normal = FreeCAD.Vector(0, -0.5, 0.866)
slope_normal.normalize()
slope_up = FreeCAD.Vector(0, 0.866, 0.5)
slope_up.normalize()
flat_normal = FreeCAD.Vector(0, -1, 0)
slope_right = FreeCAD.Vector(1, 0, 0)

cut_depth = 10.0
panel_thickness = 1.63

def point_on_slope(x, slope_offset_mm):
    bottom_y = -258.0
    bottom_z = -74.0
    y = bottom_y + slope_up.y * slope_offset_mm
    z = bottom_z + slope_up.z * slope_offset_mm
    return FreeCAD.Vector(x, y, z)

def cut_circle(shape, pos, diameter, axis):
    r = diameter / 2.0
    start = pos - axis * cut_depth
    cyl = Part.makeCylinder(r, cut_depth * 2, start, axis)
    return shape.cut(cyl)

def cut_rectangle(shape, centre, width, height, axis, right_dir, up_dir):
    hw = width / 2.0
    hh = height / 2.0
    p1 = centre - right_dir * hw - up_dir * hh
    p2 = centre + right_dir * hw - up_dir * hh
    p3 = centre + right_dir * hw + up_dir * hh
    p4 = centre - right_dir * hw + up_dir * hh
    wire = Part.makePolygon([p1, p2, p3, p4, p1])
    face = Part.Face(wire)
    box = face.extrude(axis * cut_depth)
    result = shape.cut(box)
    box2 = face.extrude(axis * (-cut_depth))
    result = result.cut(box2)
    return result

# ============================================================
# Find exact face positions from the panel
# ============================================================

# Find the outer flat bottom face (pushbutton surface)
# This is the face pointing in -Y direction (outward) with large area
flat_outer_y = None
for face in panel.Faces:
    if isinstance(face.Surface, Part.Plane):
        n = face.Surface.Axis
        if abs(n.y - (-1.0)) < 0.01 and abs(n.x) < 0.01 and abs(n.z) < 0.01 and face.Area > 5000:
            flat_outer_y = face.BoundBox.YMin  # outermost Y of the flat face
            flat_z_min = face.BoundBox.ZMin
            flat_z_max = face.BoundBox.ZMax
            flat_z_centre = (flat_z_min + flat_z_max) / 2
            print(f"Flat face outer surface at Y={flat_outer_y:.2f}")
            print(f"  Z range: {flat_z_min:.1f} to {flat_z_max:.1f}, centre={flat_z_centre:.1f}")
            break

# ============================================================
# Layout
# ============================================================

encoder_slope_offset = 10.0
screen_slope_offset = 80.0
pot_slope_offsets = [screen_slope_offset + 25.0, screen_slope_offset - 25.0]
encoder_new_x = [-69.555 + i * 19.87 for i in range(8)]

# Pushbutton positions — ON the actual flat face surface
pushbutton_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]
pb_y = flat_outer_y  # exact outer surface position
pb_z = flat_z_centre  # vertically centred on the flat face

print(f"Pushbutton positions: Y={pb_y:.2f}, Z={pb_z:.2f}")

# ============================================================
# Cut holes in faceplate
# ============================================================

print("\n=== CUTTING FACEPLATE ===")
result = panel

# Screen cutout — 165 x 103mm (board width x board height + clearance)
# Board PCB is ~100mm height at the PCB level, standoffs extend beyond
# Cutout accommodates the board, standoffs rest on panel back
screen_centre = point_on_slope(0, screen_slope_offset)
print(f"Screen cutout (165 x 103mm): centre ({screen_centre.x:.1f}, {screen_centre.y:.1f}, {screen_centre.z:.1f})")
result = cut_rectangle(result, screen_centre, 165.0, 103.0, slope_normal, slope_right, slope_up)

# Screen mounting holes — 157.0 x 115.0mm pattern
screen_mounts = []
for v_off in [-115.0/2, 115.0/2]:
    for x in [-157.0/2, 157.0/2]:
        pos = point_on_slope(x, screen_slope_offset + v_off)
        screen_mounts.append(pos)
print("Screen mounting holes (4 x 3.2mm)")
for pos in screen_mounts:
    result = cut_circle(result, pos, 3.2, slope_normal)

# Encoders — 8 x 9.5mm
encoder_positions = [point_on_slope(x, encoder_slope_offset) for x in encoder_new_x]
print("Encoder holes (8 x 9.5mm)")
for pos in encoder_positions:
    result = cut_circle(result, pos, 9.5, slope_normal)

# Pots — 4 x 7.0mm
pot_positions = []
for side_x in [-101.0, 101.0]:
    for slope_off in pot_slope_offsets:
        pos = point_on_slope(side_x, slope_off)
        pot_positions.append(pos)
print("Pot holes (4 x 7.0mm)")
for pos in pot_positions:
    result = cut_circle(result, pos, 7.0, slope_normal)

# LED bezels — 4 x 8.0mm (TBD, may change to ~7.5mm)
led_x = -116.0
led_positions = []
for pot_off in pot_slope_offsets:
    for v in [7.0, -7.0]:
        pos = point_on_slope(led_x, pot_off + v)
        led_positions.append(pos)
print("LED holes (4 x 8.0mm)")
for pos in led_positions:
    result = cut_circle(result, pos, 8.0, slope_normal)

# Pushbuttons — 8 x 16mm
print("Pushbutton holes (8 x 16.0mm)")
for x in pushbutton_x:
    pos = FreeCAD.Vector(x, pb_y, pb_z)
    result = cut_circle(result, pos, 16.0, flat_normal)

# Export faceplate
print(f"\nExporting faceplate to {output_faceplate}...")
Part.Shape.exportStep(result, output_faceplate)

# ============================================================
# Build assembly visualisation
# ============================================================

print("\n=== BUILDING ASSEMBLY ===")
components = []

# ENCODERS
print("Encoder models...")
for pos in encoder_positions:
    # Knob: 15.1mm dia, 15.2mm tall above panel
    knob = Part.makeCylinder(15.1/2, 15.2, pos, slope_normal)
    components.append(knob)

    # Bushing: 9mm dia, 3.1mm above panel
    bushing = Part.makeCylinder(9.0/2, 3.1, pos, slope_normal)
    components.append(bushing)

    # Shaft inside knob: 6mm dia
    shaft = Part.makeCylinder(6.0/2, 15.0, pos, slope_normal)
    components.append(shaft)

    # Body behind panel: 16.8mm wide, 6.5mm deep
    body_start = pos - slope_normal * (panel_thickness + 6.5)
    body = Part.makeCylinder(16.8/2, 6.5, body_start, slope_normal)
    components.append(body)

    # Terminals: 2.7mm further, narrower
    term_start = body_start - slope_normal * 2.7
    terminals = Part.makeCylinder(10.0/2, 2.7, term_start, slope_normal)
    components.append(terminals)

# BUTTONS — corrected
print("Button models...")
for x in pushbutton_x:
    surface_pos = FreeCAD.Vector(x, pb_y, pb_z)

    # Cap: extends 1.5mm outward (-Y) from panel surface, 16mm dia
    cap_start = surface_pos - flat_normal * 1.5  # 1.5mm outward (more negative Y)
    cap = Part.makeCylinder(16.0/2, 1.5, cap_start, flat_normal)
    components.append(cap)

    # Barrel through panel and behind: 16mm dia (fits the hole)
    # Total length from panel surface inward: 21.5 - 1.5 = 20.0mm
    # This goes through the 1.63mm panel and extends 18.37mm behind
    barrel_start = surface_pos  # starts at panel outer surface
    barrel = Part.makeCylinder(16.0/2, 20.0, barrel_start, FreeCAD.Vector(0, 1, 0))  # inward (+Y)
    components.append(barrel)

# POTS
print("Pot models...")
for pos in pot_positions:
    knob = Part.makeCylinder(20.1/2, 15.5, pos, slope_normal)
    components.append(knob)

    shaft = Part.makeCylinder(6.0/2, 8.0, pos, slope_normal)
    components.append(shaft)

    body_start = pos - slope_normal * (panel_thickness + 5.0)
    body = Part.makeCylinder(9.0/2, 5.0, body_start, slope_normal)
    components.append(body)

# LED BEZELS
print("LED bezel models...")
for pos in led_positions:
    # Flange on panel surface: 8mm dia, 2mm
    flange = Part.makeCylinder(8.0/2, 2.0, pos, slope_normal)
    components.append(flange)

    # Body through panel
    body_start = pos - slope_normal * panel_thickness
    body = Part.makeCylinder(6.0/2, panel_thickness, body_start, slope_normal)
    components.append(body)

# DISPLAY — position the actual Waveshare STEP model
print("Positioning display...")

# Display native: X horizontal (166mm), Y is depth/normal (-3.4 to 10.7), Z is vertical (124mm)
# Screen surface faces -Y at Y=-3.4
# We need:
#   Display -Y face → slope_normal direction (outward from slope)
#   Display X → slope_right (horizontal)
#   Display Z → slope_up (up the slope)

# Rotation: from display native to slope orientation
# Native: screen faces -Y, up is +Z
# Target: screen faces slope_normal outward, up is slope_up
# This is a rotation of -60 degrees around X (tilting the screen to match 30-degree slope)

rotated_display = display_shape.copy()
rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), -60)
rotated_display.Placement = FreeCAD.Placement(FreeCAD.Vector(0, 0, 0), rotation)

# Find where it ended up
rbb = rotated_display.BoundBox
rotated_centre = FreeCAD.Vector(
    (rbb.XMin + rbb.XMax) / 2,
    (rbb.YMin + rbb.YMax) / 2,
    (rbb.ZMin + rbb.ZMax) / 2
)

# Translate to screen centre position
translation = screen_centre - rotated_centre
rotated_display.translate(translation)

# Adjust depth: the screen glass should be roughly flush with the panel outer surface
# The display's screen surface was at Y=-3.4 in native coords
# After rotation and translation, we may need to nudge along slope_normal
# For now, shift it so the front face aligns with the panel surface
rotated_display.translate(slope_normal * 3)  # adjust to roughly flush

components.append(rotated_display)

rbb2 = rotated_display.BoundBox
print(f"Display positioned: X={rbb2.XMin:.1f} to {rbb2.XMax:.1f}, Y={rbb2.YMin:.1f} to {rbb2.YMax:.1f}, Z={rbb2.ZMin:.1f} to {rbb2.ZMax:.1f}")

# Combine
print("\nCombining assembly...")
combined = result  # start with faceplate (with holes)
for comp in components:
    try:
        combined = combined.fuse(comp)
    except:
        pass

print(f"Exporting assembly to {output_assembly}...")
Part.Shape.exportStep(combined, output_assembly)

# Verify faceplate
print("\nVerifying faceplate holes...")
doc3 = FreeCAD.newDocument("Verify")
Import.insert(output_faceplate, doc3.Name)
vshape = doc3.Objects[0].Shape

from collections import defaultdict
by_d = defaultdict(int)
for face in vshape.Faces:
    if isinstance(face.Surface, Part.Cylinder):
        d = round(face.Surface.Radius * 2, 1)
        by_d[d] += 1

print(f"Faceplate: {len(vshape.Faces)} faces")
for d in sorted(by_d.keys()):
    print(f"  {d}mm: {by_d[d]} faces")

print("\n=== DONE ===")

FreeCAD.closeDocument(doc.Name)
FreeCAD.closeDocument(doc2.Name)
FreeCAD.closeDocument(doc3.Name)
