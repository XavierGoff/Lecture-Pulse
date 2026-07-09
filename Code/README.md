# Code

Final system (three parts):

- **pad-firmware/ESP32_StudentStation.ino** — per-pad ESP32 firmware. Buttons: GPIO13 happy / GPIO12 neutral / GPIO14 sad / GPIO27 question; GPIO26 phone-slot limit switch (activates the station), GPIO25 status LED. Sends JSON button events to the bridge over Wi-Fi WebSockets with auto-reconnect. Set `STUDENT_ID`, Wi-Fi credentials and `BRIDGE_HOST` per board.
- **bridge/serial_bridge.py** — Python bridge: auto-detects the hub's USB serial port (CP210x/CH340/FTDI), maintains the room state `{score, question}` and rebroadcasts it to browser clients over a local WebSocket server (port 8765).
- **overlay/classroom-mood-bar.user.js** — Tampermonkey userscript: draws the live mood bar + question indicator over any page (i.e. the lecture slides), fed from the bridge WebSocket.

Run order: flash pads → start `serial_bridge.py` → enable the userscript in Chrome.

`prototypes/` documents the four development stages that led here.
