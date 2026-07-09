"""
serial_bridge.py (updated)
Reads from Arduino, sends JSON {score, question} to slideshow via WebSocket.

pip install pyserial websockets
python serial_bridge.py
"""

import asyncio
import serial
import serial.tools.list_ports
import websockets
import json
import sys

SERIAL_PORT = None
BAUD_RATE   = 9600
WS_HOST     = "localhost"
WS_PORT     = 8765

connected_clients = set()
state = {"score": 0.0, "question": False}

def find_arduino():
    for port in serial.tools.list_ports.comports():
        desc = (port.description or "").lower()
        mfr  = (port.manufacturer or "").lower()
        if any(k in desc or k in mfr for k in ["arduino", "ch340", "cp210", "ftdi", "acm"]):
            print(f"[Bridge] Found Arduino on {port.device}")
            return port.device
    return None

async def ws_handler(websocket):
    connected_clients.add(websocket)
    print(f"[Bridge] Client connected ({len(connected_clients)} total)")
    await websocket.send(json.dumps(state))
    try:
        await websocket.wait_closed()
    finally:
        connected_clients.discard(websocket)

async def broadcast():
    if connected_clients:
        msg = json.dumps(state)
        await asyncio.gather(*(ws.send(msg) for ws in connected_clients), return_exceptions=True)

async def read_serial(port):
    print(f"[Bridge] Opening {port} at {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
    except serial.SerialException as e:
        print(f"[Bridge] ERROR: {e}"); sys.exit(1)

    print("[Bridge] Ready.")
    loop = asyncio.get_event_loop()

    while True:
        line = await loop.run_in_executor(None, ser.readline)
        if not line: continue
        text = line.decode("utf-8", errors="ignore").strip()
        if not text: continue

        changed = False
        if text.startswith("SCORE:"):
            try:
                state["score"] = round(float(text.split(":")[1]), 2)
                changed = True
                print(f"[Bridge] Score -> {state['score']}")
            except ValueError: pass
        elif text.startswith("QUESTION:"):
            try:
                state["question"] = bool(int(text.split(":")[1]))
                changed = True
                print(f"[Bridge] Question -> {state['question']}")
            except ValueError: pass

        if changed:
            await broadcast()

async def main():
    port = SERIAL_PORT or find_arduino()
    if not port:
        print("[Bridge] No Arduino found."); sys.exit(1)
    print(f"[Bridge] WS server on ws://{WS_HOST}:{WS_PORT}")
    async with websockets.serve(ws_handler, WS_HOST, WS_PORT):
        await read_serial(port)

if __name__ == "__main__":
    try: asyncio.run(main())
    except KeyboardInterrupt: print("\n[Bridge] Stopped.")
