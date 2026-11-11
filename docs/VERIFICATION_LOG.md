# Verification log

The final consolidated Host validation was run on 2026-11-11 (Asia/Shanghai).

Build:

    cmd /c call Visual Studio 2022 VsDevCmd.bat -arch=x64
    cmake -S . -B build/host-vs -G "Visual Studio 17 2022" -A x64 -DRATSENSE_PLATFORM=HOST_SIM
    cmake --build build/host-vs --config Debug --parallel 4

The build produced ratsense_core.lib, ratsense_sim.exe,
ratsense_gateway_sim.exe, uart_frame_tool.exe, lcd_preview.exe,
log_decoder.exe and ratsense_unit_tests.exe under build/host-vs/Debug.
The concise build record is out/verification/build_host_v1.txt.

CTest passed 1/1 test. The full output is in
out/verification/ctest_host_v1.txt. All 19 deterministic simulator scenarios
passed; output is in out/verification/scenarios.txt. Python tests passed 6/6 and
compileall passed; output is in out/verification/python_tests.txt and
out/verification/python_compileall.txt.

ASan/UBSan, ARM target compilation, ESP-IDF compilation, Docker broker/API
end-to-end, and physical HIL remain skipped with explicit statuses in
docs/VERIFICATION_MATRIX.md.
