# Prototype rundowns

Four functional stages, oldest to newest. Each was wired in TinkerCad first (`../../Electronics/`), then built and photographed (`../../Images/`).

## 01 — First button tests (`01-first-button-tests/`)

Arduino R4, six momentary switches on D2–D7 with `INPUT_PULLUP`, polled every 100 ms and printed over serial. No logic — the point was to validate the input wiring approach (active-low, no external resistors) and get a feel for bounce behaviour before designing the debounce strategy.

## 02 — Student-cycle prototype (`02-student-cycle/`)

One 4-button mood set plus prev/next buttons cycling an "active student" pointer across four virtual students. This stage introduced everything that survived to production:

- **Per-student vote store** — each student's latest vote (+10 / 0 / −10) with timestamp
- **Room score** — mean of all students who have voted, exponentially smoothed (`score += (target − score) × 0.05`) at 10 Hz so the display moves credibly
- **Question logic** — presses logged with timestamps; alert goes active only when ≥ 2 fall inside a 10 s sliding window, filtering accidental presses
- **`SCORE:` / `QUESTION:` serial protocol** consumed by `serial_bridge.py`, which rebroadcasts to `slideshow.html` over WebSocket
- LED-matrix smiley/neutral/sad/question faces as an at-a-glance debug display

## 03 — Separate stations (`03-separate-stations/`)

Three fixed student stations (4 buttons each) across 12 GPIOs on one board — the first genuinely simultaneous multi-student input. Same engine, now stress-tested against concurrent presses; question threshold logic gained expiry cleanup of the press log. Display path unchanged (bridge → slideshow), proving the protocol held up.

## 04 — Final pads (`../pad-firmware/`)

The engine moved off the breadboard: one ESP32 per student, Wi-Fi WebSocket client with 3 s auto-reconnect, JSON event schema (`{studentId, type, mood, active}`), phone-slot limit-switch activation and status LED. Scoring moved into the Python bridge, so pads stay stateless and cheap.
