# CAD

## Enclosure design

The enclosure evolved through three forms:

1. **V1 — box + phone insert.** A large rectangular case with a slot underneath to insert the phone and four button holes on top. Functionally complete, but bulky, material-inefficient, and visually wrong for the audience.
2. **V2 — cuboid + integrated phone holder.** Smaller, with top-face button openings — but packaging the electronics *and* the phone still forced an oversized body.
3. **Final — compact game-controller form.** Chosen after user testing showed the familiar, leisure-associated shape measurably increased willingness to engage. The body shrinks around the electronics with dedicated internal mounts for the ESP32, LiPo Amigo Pro and battery.

Key engineering features:

- **Heat-set threaded inserts** + countersunk M3 screws — strong clamping with a seamless top face
- **Phone slot** whose underside actuates the GPIO 26 limit switch
- **USB cutout** positioned so the ESP32 can be flashed/charged fully assembled
- Button apertures sized for the illuminated switches, with the larger rounded question button user-tested against alternatives

## Files

**CAD:**

- [`Full Assembly.step`](Full%20Assembly.step) — complete final assembly, portable STEP format
- [`Casing.stl`](Casing.stl) — base shell (3D-print ready)
- [`Top Cover.stl`](Top%20Cover.stl) — top plate with button apertures
- [`PhonePlacementPad.stl`](PhonePlacementPad.stl) — phone-slot pad that actuates the limit switch

**Renders:**

- [`CADimage-casing-v1.jpg`](CADimage-casing-v1.jpg) — V1 box with phone insert
- [`CADimage-casing-v2.png`](CADimage-casing-v2.png) — V2 compact game-controller form
- [`CADimage-base-shell.png`](CADimage-base-shell.png) — final base shell: component mounts, heat-set bosses, USB cutout
