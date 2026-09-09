#!/usr/bin/env python3
"""Generate editable DXF + FreeCAD layout drawings for each panel face."""

import sys
sys.path.append('/Applications/FreeCAD.app/Contents/Resources/lib')
import FreeCAD
import Draft
import Part
import importDXF

output_dir = '/Users/aidan/Dev/DronemakerClonev3/hardware/enclosure/design-brief'

def circle_at(radius, x, y, label="Circle"):
    c = Draft.make_circle(radius)
    c.Placement = FreeCAD.Placement(FreeCAD.Vector(x, y, 0), FreeCAD.Rotation())
    c.Label = label
    return c

def rect_at(w, h, cx, cy, label="Rect"):
    r = Draft.make_rectangle(w, h)
    r.Placement = FreeCAD.Placement(FreeCAD.Vector(cx - w/2, cy - h/2, 0), FreeCAD.Rotation())
    r.Label = label
    return r

def dim_h(x1, x2, y, label="Dim"):
    d = Draft.make_linear_dimension(
        FreeCAD.Vector(x1, y, 0), FreeCAD.Vector(x2, y, 0),
        FreeCAD.Vector((x1+x2)/2, y - 3, 0))
    d.Label = label
    return d

def dim_v(x, y1, y2, label="Dim"):
    d = Draft.make_linear_dimension(
        FreeCAD.Vector(x, y1, 0), FreeCAD.Vector(x, y2, 0),
        FreeCAD.Vector(x + 5, (y1+y2)/2, 0))
    d.Label = label
    return d

def text_at(lines, x, y, label="Text"):
    t = Draft.make_text(lines, FreeCAD.Vector(x, y, 0))
    t.Label = label
    return t

# ============================================================
# Constants
# ============================================================

# Display geometry (from STEP model analysis)
active_cx = 1.9       # active area X offset from mount centre
active_cz = -5.15     # active area Z offset from mount centre (native coords)
mount_h_spacing = 157.0
mount_v_spacing = 115.0
active_pixel_w = 154.21
active_pixel_h = 85.92

# Layout positions (slope offsets from bottom, in mm)
mount_centre_offset = 78.0
active_centre_offset = mount_centre_offset + active_cz  # 72.85
encoder_offset = 10.0

# Encoder alignment
encoder_left_x = active_cx - active_pixel_w/2 + 15.1/2   # -67.655
encoder_right_x = active_cx + active_pixel_w/2 - 15.1/2  # 71.455
encoder_spacing = (encoder_right_x - encoder_left_x) / 7
encoder_x = [encoder_left_x + i * encoder_spacing for i in range(8)]

# Pots
pot_x_left, pot_x_right = -101.0, 101.0
pot_upper = mount_centre_offset + 25.0
pot_lower = mount_centre_offset - 25.0

# LEDs
led_x = -116.0

# ============================================================
# SLOPE FACE
# ============================================================

print("Creating slope face layout...")
doc = FreeCAD.newDocument("SlopeFace")

# Panel outline (254 x 144mm usable slope area)
rect_at(254.0, 144.0, 0, 72, "Panel_Slope_254x144")

# Screen cutout
rect_at(155.0, 87.0, active_cx, active_centre_offset, "CUTOUT_Screen_155x87")
dim_h(active_cx - 77.5, active_cx + 77.5, active_centre_offset - 87/2 - 5, "Dim_Cutout_W_155")
dim_v(active_cx + 77.5 + 5, active_centre_offset - 87/2, active_centre_offset + 87/2, "Dim_Cutout_H_87")

# Active pixel area (reference)
rect_at(active_pixel_w, active_pixel_h, active_cx, active_centre_offset, "REF_Active_Pixels_154.21x85.92")

# Mount holes
for hx in [-mount_h_spacing/2, mount_h_spacing/2]:
    for vy in [-mount_v_spacing/2, mount_v_spacing/2]:
        circle_at(3.3/2, hx, mount_centre_offset + vy, f"Mount_M3_3.3mm")

dim_h(-mount_h_spacing/2, mount_h_spacing/2, mount_centre_offset + mount_v_spacing/2 + 4, "Dim_Mount_H_157")
dim_v(mount_h_spacing/2 + 5, mount_centre_offset - mount_v_spacing/2, mount_centre_offset + mount_v_spacing/2, "Dim_Mount_V_115")

# Encoders
for i, ex in enumerate(encoder_x):
    circle_at(9.5/2, ex, encoder_offset, f"Encoder_{i+1}_Hole_9.5mm")
    circle_at(15.1/2, ex, encoder_offset, f"Encoder_{i+1}_Knob_15.1mm_REF")

dim_h(encoder_x[0], encoder_x[1], encoder_offset - 10, "Dim_Encoder_Spacing_19.87")
text_at(["8x Encoder: 9.5mm hole", f"19.87mm c-to-c", "Knob edges align with active pixels"], 0, encoder_offset - 16, "Note_Encoders")

# Pots
for px, py, name in [
    (pot_x_left, pot_upper, "XLR_Gain"),
    (pot_x_left, pot_lower, "Inst_Gain"),
    (pot_x_right, pot_upper, "Master_Vol"),
    (pot_x_right, pot_lower, "HP_Vol"),
]:
    circle_at(7.0/2, px, py, f"Pot_{name}_7mm")
    circle_at(20.1/2, px, py, f"Pot_{name}_Knob_20.1mm_REF")

text_at(["4x Pot: 7mm hole", "Knob 20.1mm dia"], pot_x_left, pot_lower - 10, "Note_Pots")

# LEDs
for py_base in [pot_upper, pot_lower]:
    for voff in [7.0, -7.0]:
        circle_at(8.0/2, led_x, py_base + voff, "LED_8mm_TBC")

text_at(["4x LED: 8mm hole (TBC)", "Stacked vert. left of pots"], led_x, pot_lower - 15, "Note_LEDs")

# Key note
text_at(["CRITICAL: Active area offset +1.9mm in X", "from mount hole centre. Cutout follows", "active area, NOT mount centre."], -80, 140, "Note_Offset")

doc.recompute()
importDXF.export([o for o in doc.Objects], f"{output_dir}/layout_slope_face.dxf")
doc.saveAs(f"{output_dir}/layout_slope_face.FCStd")
print("  Exported slope face DXF + FCStd")
FreeCAD.closeDocument(doc.Name)

# ============================================================
# FRONT FACE
# ============================================================

print("Creating front face layout...")
doc = FreeCAD.newDocument("FrontFace")

rect_at(254.0, 24.7, 0, 12.35, "Panel_Front_254x24.7")

pb_x = [-87.5, -62.5, -37.5, -12.5, 12.5, 37.5, 62.5, 87.5]
for i, x in enumerate(pb_x):
    circle_at(16.0/2, x, 12.35, f"Button_{i+1}_16mm")
    circle_at(18.5/2, x, 12.35, f"Button_{i+1}_Cap_18.5mm_REF")

dim_h(pb_x[0], pb_x[1], -3, "Dim_Button_Spacing_25")
text_at(["8x Pushbutton: 16mm hole", "25mm c-to-c, centred vertically"], 0, -8, "Note_Buttons")

doc.recompute()
importDXF.export([o for o in doc.Objects], f"{output_dir}/layout_front_face.dxf")
doc.saveAs(f"{output_dir}/layout_front_face.FCStd")
print("  Exported front face DXF + FCStd")
FreeCAD.closeDocument(doc.Name)

# ============================================================
# BACK PANEL
# ============================================================

print("Creating back panel layout...")
doc = FreeCAD.newDocument("BackPanel")

rect_at(233.4, 72.2, 0, 36.1, "Panel_Opening_233.4x72.2")

cy = 36.1  # centred vertically

# D-size connectors (approximated as 24mm circles)
d_connectors = [
    (-90, "XLR_TRS_In", 24.0),
    (-60, "Instrument_In", 24.0),
    (-30, "L_Out", 24.0),
    (0, "R_Out", 24.0),
    (30, "Headphone", 24.0),
]

for cx, name, dia in d_connectors:
    circle_at(dia/2, cx, cy, f"{name}_D-size_{dia}mm")
    # M3 screw holes
    circle_at(3.2/2, cx, cy + 14, f"{name}_screw_top")
    circle_at(3.2/2, cx, cy - 14, f"{name}_screw_bot")

# USB-A D-size
rect_at(24.0, 19.2, 60, cy, "USB-A_NAUSB3-B_D-size_24x19.2")
circle_at(3.2/2, 60, cy + 12, "USB-A_screw_top")
circle_at(3.2/2, 60, cy - 12, "USB-A_screw_bot")

# USB-C Keystone
rect_at(16.2, 14.7, 88, cy, "USB-C_Keystone_16.2x14.7")

# Labels
for cx, name in [(-90,"XLR/TRS In"), (-60,"Inst In"), (-30,"L Out"), (0,"R Out"),
                  (30,"HP Out"), (60,"USB-A"), (88,"USB-C")]:
    text_at([name], cx, cy - 20, f"Label_{name}")

dim_h(-90, -60, cy + 22, "Dim_D-spacing_30")
text_at(["6x D-size + 1x Keystone", "Centred in 233.4mm opening"], 0, -5, "Note_Back")

doc.recompute()
importDXF.export([o for o in doc.Objects], f"{output_dir}/layout_back_panel.dxf")
doc.saveAs(f"{output_dir}/layout_back_panel.FCStd")
print("  Exported back panel DXF + FCStd")
FreeCAD.closeDocument(doc.Name)

print("\nAll layouts generated!")
