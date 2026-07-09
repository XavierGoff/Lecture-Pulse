# Festival judging system

Built for the Great Exhibition Road Festival stand: a fully wireless demo needing no on-site Wi-Fi.

- **Judge_Slave-Student1/, Judge_Slave-Student2/** — wireless judge pads sending votes over **ESP-NOW**
- **Master_Relay/** — receives ESP-NOW messages, runs the same scoring engine (smoothed score, question window), and streams `SCORE:`/`QUESTION:` over USB serial to the live display
