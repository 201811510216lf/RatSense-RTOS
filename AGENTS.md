# RatSense-RTOS agent continuation notes

This repository is an autonomous embedded-system implementation for the RatSense project. The authoritative requirements are in RatSense_Codex_一键开发操作手册.md; this file records execution rules and continuation state.

## Working rules

- Keep the Host implementation deterministic and runnable without boards.
- Never label a Host simulation as target or hardware verification.
- Do not add credentials, private keys, Wi-Fi passwords, MQTT tokens, or device certificates.
- Keep dangerous actuator actions behind an explicit local training/safety state.
- Every new module needs a test or a documented hardware-only verification step.
- Update docs/PROGRESS.md, docs/VERIFICATION_MATRIX.md, and docs/NEXT_ACTIONS.md as work advances.

## Current continuation point

The workspace started empty except for the operating manual. Reference repository access was attempted on
2026-11-11 but DNS/network resolution failed. The implementation is therefore based only on the requirements in the
manual and is deliberately marked BLOCKED_EXTERNAL where source/model/toolchain evidence is required.

The Host code, protocol codec, simulator, server skeleton, and target-port skeleton are implemented in this
workspace. A Visual Studio/MSVC Host build and CTest run passed on 2026-11-11; the evidence is under
out/verification. No physical board is available, so all STM32/ESP hardware interactions remain NEEDS_HARDWARE until
HIL verification is performed.

