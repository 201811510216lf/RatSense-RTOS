# Power and grounding plan

The motor supply is separate from the 3.3 V logic rail. ULN2003 ground, STM32
ground, ESP ground, sensor ground and SD ground share a low-impedance reference at
the confirmed star point. The motor rail needs bulk capacitance and a local
flyback/current path; motor transients must not collapse the ESP rail.

The FSR interface is not assumed to be 3.3 V safe. Measure the conversion module
output with the intended supply and add a protected 3.3 V divider/buffer if
required. Never connect an unverified 5 V analog node to an STM32 ADC.

The ESP32-S3 supply must meet Wi-Fi peak current and use short decoupling paths.
Add a fuse or current-limited bench supply during bring-up. An external animal
device must have a manual power cut, an alternative hand-feeding path, and an
emergency stop.

No current, thermal, brownout or long-run power claim is made until HIL testing.

