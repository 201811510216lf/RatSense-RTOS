# Engineering decisions

| Decision | Reason |
|---|---|
| COBS plus zero delimiter | bounded resynchronization after a dropped byte |
| explicit little-endian codec | no compiler-layout or packing dependence |
| fixed 512-byte payload limit | bounds parser memory and queue pressure |
| motor disabled on boot | prevents automatic feeding after reset |
| feed success requires IR | motor motion alone cannot claim a pellet |
| single-writer event log | avoids FatFs contention with safety tasks |
| synthetic RQ-GPR only | reference model and training data were unavailable |
| official ESP UART buffered driver | no unverified DMA API is invented |
| OTA safe window | training, motor and pending writes must be quiescent |
| no guessed GPIO/flash geometry | wrong hardware assumptions can damage equipment |

