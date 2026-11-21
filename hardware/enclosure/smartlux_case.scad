// =====================================================================
// SMARTLUX — parametric 3D-printable enclosure
// Kelompok 2 · SMAN CMBBS · 2026/2027
//
// Units: millimetres. Requires OpenSCAD >= 2019.05.
// Plain primitives only — no external libraries (no BOSL2 / MCAD).
//
//   PART = "case" -> case (floor, 4x M3 corner bosses, standoffs, cutouts)
//   PART = "lid"  -> lid alone, z = 0 at its INNER face (flip in slicer)
//   PART = "both" -> exploded preview (lid lifted 12 mm)
//
// Export:
//   openscad -o smartlux_case.stl -D 'PART="case"' smartlux_case.scad
//   openscad -o smartlux_lid.stl  -D 'PART="lid"'  smartlux_case.scad
//
// NOTE: every value marked "MEASURE" is an assumption for a common
// module. Check it with calipers on the real part before printing.
// =====================================================================

/* [Part to render] */
PART = "case"; // [case:Case only, lid:Lid only, both:Exploded preview]

/* [Cavity] */
inner_w = 110.0;   // inner width  (X)
inner_d = 75.0;    // inner depth  (Y)
inner_h = 40.0;    // inner height (Z, above the floor)

/* [Shell] */
wall     = 2.4;
floor_t  = 2.4;
corner_r = 4.0;    // outer vertical corner radius
lid_t    = 2.4;    // lid plate thickness
tol      = 0.3;    // general clearance for parts passing through holes
embed    = 0.3;    // overlap so bosses/standoffs fuse with floor/lid

// ---------- derived shell dimensions ----------
outer_w = inner_w + 2 * wall;          // 114.8
outer_d = inner_d + 2 * wall;          //  79.8
case_h  = floor_t + inner_h;           //  42.4  rim height
total_h = case_h + lid_t;              //  44.8
inner_r = max(corner_r - wall, 0.8);   // inner corner radius

/* [Lid fastening: 4 x M3 screws into corner bosses]
   Chosen instead of a snap-fit lip: screws are serviceable, the lid can
   be opened many times during calibration and no flexible lip is needed. */
boss_d       = 7.0;    // boss outside diameter
boss_inset   = 8.0;    // boss centre, measured from the INNER wall face
boss_hole_d  = 2.8;    // M3 self-tapping pilot (PETG); 4.6 for heat-set
boss_hole_h  = 9.0;    // usable thread depth
screw_hole_d = 3.4;    // clearance hole in the lid
cs_d         = 6.2;    // countersink diameter (DIN 963 M3 head 5.6)
cs_h         = 1.6;    // countersink depth
rib_h        = 10.0;   // corner gusset height (rigidity while printing)

boss_x0 = wall + boss_inset;           // 10.4
boss_y0 = wall + boss_inset;           // 10.4

/* [ESP32 DevKit 55 x 28, on the floor, 4 x M3] */
esp_w  = 55.0;  esp_d  = 28.0;         // board outline
esp_cx = 43.5;  esp_cy = 26.0;         // board centre
esp_hole_dx = 48.5;  // MEASURE: mounting hole spacing (1.9 in boards)
esp_hole_dy = 22.9;  // MEASURE
esp_stand_h = 5.0;   // floor -> PCB
esp_stand_d = 5.0;
esp_pilot_d = 2.8;

/* [SSD1306 OLED 27 x 27, mounted under the lid, 4 x M2] */
oled_cx = 43.5;  oled_cy = 26.0;
oled_hole_dx = 23.5;  // MEASURE
oled_hole_dy = 23.5;  // MEASURE
oled_stand_h = 3.0;
oled_stand_d = 5.0;
oled_pilot_d = 1.6;
oled_win_w   = 26.0;  // visible glass window
oled_win_h   = 14.0;

/* [INA219 26 x 21, on the floor, 2 x M2] */
ina_cx = 86.0;  ina_cy = 58.0;
ina_hole_dx = 20.0;  // MEASURE
ina_stand_h = 5.0;
ina_stand_d = 4.5;
ina_pilot_d = 1.6;

/* [HC-SR501 PIR 32 x 24, mounted under the lid, 4 x M2.5] */
pir_cx = 87.0;  pir_cy = 27.0;
pir_hole_dx = 26.0;  // MEASURE
pir_hole_dy = 18.0;  // MEASURE
pir_stand_h = 3.0;
pir_stand_d = 5.5;
pir_pilot_d = 2.0;
pir_dome_d  = 24.0;  // fresnel dome opening

/* [Lid: button + LDR] */
btn_cx = 60.0;  btn_cy = 62.0;  btn_d = 12.0;      // panel button
ldr_cx = 32.0;  ldr_cy = 62.0;  ldr_d = 5.0;       // LDR window
ldr_pipe_d = 8.0;  ldr_pipe_h = 1.6;               // light-pipe recess

/* [Side cutouts] */
usb_w = 12.0;  usb_h = 8.0;         // left wall (X = 0)
usb_cy = 26.0; usb_cz = 10.9;       // floor + standoff + 3.5
jack_w = 9.0;  jack_h = 11.0;       // right wall (X = outer_w)
jack_cy = 58.0; jack_cz = 12.0;
vent_len = 40.0;  vent_h = 3.0;     // back wall (Y = outer_d)
vent_x   = 45.0;                    // vent slot centre (X)
vent_z   = [12.0, 19.0, 26.0];      // vent slot centres (Z)

/* [Cable-tie tab on the floor] */
tie_x = 40.0;  tie_y = 44.0;
tie_w = 12.0;  tie_d = 3.0;  tie_h = 5.0;
tie_slot_w = 6.0;  tie_slot_h = 1.6;  tie_slot_z = 2.2;

/* [Optional embossed label, front wall] */
label_on    = true;   // set false if OpenSCAD has no usable font
label_text  = "SMARTLUX";
label_size  = 7.0;
label_depth = 0.6;
label_x     = outer_w / 2;
label_z     = 7.0;

/* [Lid locator ribs] */
tab_cx = 43.5;  tab_w = 24.0;  tab_d = 3.0;  tab_h = 1.2;

$fn = 48;

// =====================================================================
// 2D helpers
// =====================================================================

// Rounded rectangle, anchored at the origin, w along X, d along Y.
module rrect(w, d, r) {
    hull() {
        for (p = [[r, r], [w - r, r], [r, d - r], [w - r, d - r]])
            translate(p) circle(r = r, $fn = 32);
    }
}

// Rounded box, base at z = 0.
module rbox(w, d, h, r) {
    linear_extrude(height = h) rrect(w, d, r);
}

// Rectangular hole with a chamfer on the TOP face.
// w along X, d along Y, hole spans z = 0 .. h, chamfer width c.
module rect_hole_chamfer(w, d, h, c) {
    translate([-w / 2, -d / 2, -0.01])
        cube([w, d, h - c + 0.01]);
    hull() {
        translate([0, 0, h - c - 0.01]) cube([w, d, 0.02], center = true);
        translate([0, 0, h + 0.01])     cube([w + 2 * c, d + 2 * c, 0.02], center = true);
    }
}

// =====================================================================
// Fasteners / standoffs
// =====================================================================

// Standoff, free end at +Z, pilot hole drilled from that free end
// and stopped 1 mm above the floor so the case stays sealed.
module standoff_up(h, d_out, d_hole) {
    difference() {
        cylinder(h = h, d = d_out, $fn = 32);
        translate([0, 0, 1.0]) cylinder(h = h - 1.0, d = d_hole, $fn = 24);
    }
}

// Same standoff but hanging down (used under the lid).
module standoff_down(h, d_out, d_hole) {
    rotate([180, 0, 0]) standoff_up(h, d_out, d_hole);
}

// M3 corner boss, floor -> rim, blind pilot hole from the rim downwards.
module screw_boss() {
    h = inner_h + embed;   // 0.3 mm sunk into the floor so it fuses
    difference() {
        cylinder(h = h, d = boss_d, $fn = 48);
        translate([0, 0, h - boss_hole_h])
            cylinder(h = boss_hole_h + 0.2, d = boss_hole_d, $fn = 24);
    }
}

// Triangular gusset boss -> inner corner, keeps the tall boss rigid.
module corner_rib(p) {
    cx = (p[0] < outer_w / 2) ? wall + 0.4 : outer_w - wall - 0.4;
    cy = (p[1] < outer_d / 2) ? wall + 0.4 : outer_d - wall - 0.4;
    hull() {
        translate([p[0], p[1], floor_t - embed])
            cylinder(h = rib_h, d = boss_d, $fn = 32);
        translate([cx - 1, cy - 1, floor_t - embed]) cube([2, 2, rib_h]);
    }
}

// Standoff standing on the case floor (0.3 mm sunk in, same height).
module floor_standoff(h, d_out, d_hole) {
    translate([0, 0, floor_t - embed]) standoff_up(h + embed, d_out, d_hole);
}

// =====================================================================
// Position tables
// =====================================================================

function boss_positions() = [
    [boss_x0,            boss_y0],
    [outer_w - boss_x0,  boss_y0],
    [boss_x0,            outer_d - boss_y0],
    [outer_w - boss_x0,  outer_d - boss_y0]
];

function esp_positions() = [
    [esp_cx - esp_hole_dx / 2, esp_cy - esp_hole_dy / 2],
    [esp_cx + esp_hole_dx / 2, esp_cy - esp_hole_dy / 2],
    [esp_cx - esp_hole_dx / 2, esp_cy + esp_hole_dy / 2],
    [esp_cx + esp_hole_dx / 2, esp_cy + esp_hole_dy / 2]
];

function oled_positions() = [
    [oled_cx - oled_hole_dx / 2, oled_cy - oled_hole_dy / 2],
    [oled_cx + oled_hole_dx / 2, oled_cy - oled_hole_dy / 2],
    [oled_cx - oled_hole_dx / 2, oled_cy + oled_hole_dy / 2],
    [oled_cx + oled_hole_dx / 2, oled_cy + oled_hole_dy / 2]
];

function pir_positions() = [
    [pir_cx - pir_hole_dx / 2, pir_cy - pir_hole_dy / 2],
    [pir_cx + pir_hole_dx / 2, pir_cy - pir_hole_dy / 2],
    [pir_cx - pir_hole_dx / 2, pir_cy + pir_hole_dy / 2],
    [pir_cx + pir_hole_dx / 2, pir_cy + pir_hole_dy / 2]
];

function ina_positions() = [
    [ina_cx - ina_hole_dx / 2, ina_cy],
    [ina_cx + ina_hole_dx / 2, ina_cy]
];

// =====================================================================
// CASE
// =====================================================================

module case_body() {
    difference() {
        rbox(outer_w, outer_d, case_h, corner_r);
        // cavity: inner_w x inner_d, from the top of the floor upwards
        translate([wall, wall, floor_t])
            rbox(inner_w, inner_d, inner_h + 1, inner_r);
    }
}

// Small bridge on the floor for a 2.5 mm cable tie (keeps wiring tidy).
module cable_tie_tab() {
    translate([tie_x, tie_y, floor_t - embed]) difference() {
        cube([tie_w, tie_d, tie_h]);
        translate([tie_w / 2 - tie_slot_w / 2, -1, tie_slot_z])
            cube([tie_slot_w, tie_d + 2, tie_slot_h]);
    }
}

// Embossed text on the front wall (y = 0), raised by label_depth.
module label() {
    translate([label_x, label_depth, label_z]) rotate([90, 0, 0])
        linear_extrude(height = 2 * label_depth)
            text(label_text, size = label_size,
                 halign = "center", valign = "center");
}

// Side / wall openings. All are through-holes in a 2.4 mm wall, none
// severs a wall and none touches a corner boss.
module case_cutouts() {
    // USB (left wall): 12 + tol wide, 8 + tol tall, at the ESP32 short edge
    translate([-1, usb_cy - (usb_w + tol) / 2, usb_cz - (usb_h + tol) / 2])
        cube([wall + 2, usb_w + tol, usb_h + tol]);

    // DC power jack (right wall), next to the INA219
    translate([outer_w - wall - 1, jack_cy - (jack_w + tol) / 2,
               jack_cz - (jack_h + tol) / 2])
        cube([wall + 2, jack_w + tol, jack_h + tol]);

    // Ventilation slots (back wall), 3 x 40 x 3, near the regulators
    for (z = vent_z)
        translate([vent_x - vent_len / 2, outer_d - wall - 1, z - vent_h / 2])
            cube([vent_len, wall + 2, vent_h]);
}

module case() {
    difference() {
        union() {
            case_body();
            // 4 x M3 corner bosses (lid screws)
            for (p = boss_positions()) {
                translate([p[0], p[1], floor_t - embed]) screw_boss();
                corner_rib(p);
            }
            // ESP32 DevKit standoffs (4 x M3)
            for (p = esp_positions())
                translate([p[0], p[1], 0])
                    floor_standoff(esp_stand_h, esp_stand_d, esp_pilot_d);
            // INA219 standoffs (2 x M2)
            for (p = ina_positions())
                translate([p[0], p[1], 0])
                    floor_standoff(ina_stand_h, ina_stand_d, ina_pilot_d);
            cable_tie_tab();
            if (label_on) label();
        }
        case_cutouts();
    }
}

// =====================================================================
// LID
// z = 0 is the INNER face; the outer face is at z = lid_t.
// For printing, flip it outer-face-down in the slicer (no supports).
// =====================================================================

// Two ribs that drop into the cavity and align the lid before screwing.
module lid_tabs() {
    translate([tab_cx - tab_w / 2, wall + tol / 2, -tab_h])
        cube([tab_w, tab_d, tab_h + embed]);
    translate([tab_cx - tab_w / 2, outer_d - wall - tol / 2 - tab_d, -tab_h])
        cube([tab_w, tab_d, tab_h + embed]);
}

module lid() {
    difference() {
        union() {
            rbox(outer_w, outer_d, lid_t, corner_r);

            // OLED standoffs under the lid (4 x M2)
            for (p = oled_positions())
                translate([p[0], p[1], embed])
                    standoff_down(oled_stand_h + embed, oled_stand_d, oled_pilot_d);

            // PIR standoffs under the lid (4 x M2.5)
            for (p = pir_positions())
                translate([p[0], p[1], embed])
                    standoff_down(pir_stand_h + embed, pir_stand_d, pir_pilot_d);

            lid_tabs();
        }

        // 4 x M3 screws through the lid into the case bosses
        for (p = boss_positions()) translate([p[0], p[1], 0]) {
            translate([0, 0, -1])
                cylinder(h = lid_t + 2, d = screw_hole_d, $fn = 32);
            translate([0, 0, lid_t - cs_h])
                cylinder(h = cs_h + 1, d1 = screw_hole_d, d2 = cs_d, $fn = 32);
        }

        // OLED window 26 x 14, chamfered on the outside face
        translate([oled_cx, oled_cy, 0])
            rect_hole_chamfer(oled_win_w, oled_win_h, lid_t, 0.8);

        // 12 mm panel button
        translate([btn_cx, btn_cy, -1])
            cylinder(h = lid_t + 2, d = btn_d + tol, $fn = 64);

        // HC-SR501 fresnel dome (24 mm + tol)
        translate([pir_cx, pir_cy, -1])
            cylinder(h = lid_t + 2, d = pir_dome_d + tol, $fn = 64);

        // LDR: 5 mm window + 8 mm light-pipe recess on the inner face
        translate([ldr_cx, ldr_cy, -1])
            cylinder(h = lid_t + 2, d = ldr_d, $fn = 32);
        translate([ldr_cx, ldr_cy, -1])
            cylinder(h = ldr_pipe_h + 1, d = ldr_pipe_d, $fn = 32);
    }
}

// =====================================================================
// Assembly preview
// =====================================================================

module assembly() {
    case();
    translate([0, 0, case_h + 12]) lid();
}

// =====================================================================
// Top-level selector
// =====================================================================

echo(str("SMARTLUX enclosure: ", outer_w, " x ", outer_d, " x ", total_h,
         " mm overall; cavity ", inner_w, " x ", inner_d, " x ", inner_h));

if (PART == "case") case();
else if (PART == "lid") lid();
else if (PART == "both") assembly();
else echo(str("PART = '", PART, "' is invalid — use case | lid | both"));
