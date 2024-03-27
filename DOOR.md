# Door ECM
Information about connecting the door control module

## Receive
When the module is powered up it emits some frames for a few seconds. The CAN bus is at 500 kbps.



| ID      | Period | Flags | Data |
| -----------: | ----------- | --- | --- 
| 1b00004b | 189 | E | 4b 00 01 04 04 00 00 00 |
| 3d1 | 98 | | 06 00 00 00 84 00 00 00
| 17f0004b | 447 | E | 20 4b 00 00 00 00 00 80

> The "E" flag indicates an extended, 29-bit frame ID (CAN 2.0B)
