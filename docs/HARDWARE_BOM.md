# Hardware BOM and open items

| Item | Status | Verification needed |
|---|---|---|
| STM32H743ZIT6 / NUCLEO-H743ZI2 | 待购 | board revision, CubeMX pinout, flash geometry |
| ESP32-S3 development board | 待购 | module variant, flash/PSRAM, UART pins |
| FSR RP-C7.6-ST-LF2 + voltage/resistance module | 已有/待核实 | supply, analog output range, divider and protection |
| 28BYJ-48 5 V five-wire motor | 已有/待核实 | exact coil order, gear ratio, current |
| ULN2003 or equivalent array | 待购/待核实 | confirm four low-side channels and 5 V supply |
| Adafruit P3502 Sharp Memory LCD 168x144 | 待核实 | DISP/EXTMOD/EXTIN/VCOM polarity and level |
| MLX90640 32x24 | 待核实 | exact module, I2C pullups, refresh rate and FoV |
| HTU21D | 待核实 | address, CRC and shared-bus electrical levels |
| IR pellet detector | 待核实 | active polarity, pulse width, mounting and second channel |
| microSD socket/card | 待核实 | SDMMC 4-bit versus SPI, voltage and pinout |
| 5 V motor supply | 待购 | current margin, fuse, brownout isolation |
| 3.3 V logic supply | 待购 | ESP Wi-Fi peak current, decoupling |
| SWD/JTAG and logic analyser | 待购 | target flash and waveform evidence |

No GPIO numbers are asserted in this repository. The placeholder board instance
structure must be populated from the exact schematic and CubeMX project.

