# Bench bring-up checklist

1. Verify board revision, supply rails, current limit, common ground and manual
   emergency stop before attaching an animal-facing mechanism.
2. Populate pinmap.csv from the schematic and CubeMX alternate-function checks.
3. Probe pressure output while powered at its intended voltage; verify ADC range,
   saturation and disconnected behaviour.
4. Drive ULN2003 with no load, then a restrained motor; capture all four phase
   waveforms and measure current before fitting the mechanism.
5. Capture IR active level and pulse width with a logic analyser. Add the second
   sensor only after its identity and geometry are known.
6. Confirm MLX90640 subpage order, HTU21D CRC and shared-bus rise time.
7. Verify Sharp P3502 command bytes, row address, bit order, VCOM flipping and
   DISP/EXTMOD/EXTIN state at low refresh.
8. Verify SDMMC mode, voltage and cache-safe DMA memory. Remove/reinsert the card
   while the motor task is active.
9. Connect STM32 and ESP through a current-limited 3.3 V UART, test 115200 first,
   then negotiate a higher rate only after error counters stay at zero.
10. Exercise MQTT TLS, broker restart, device reset and OTA interruptions with the
    motor mechanically safe. Hardware success is recorded separately from Host.

