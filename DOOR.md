# Door ECM
Information about connecting the door control module

## Receive
When the module is powered up it emits some frames for five seconds.
The CAN bus is at 500 kbps.

| Count | Period | ID      | Flags | Data |
| ---: | ------: | --------: | --- | --- 
| 20 | 189 | 1b00004b | E | 4b 00 01 04 04 00 00 00 |
| 50 | 98 | 3d1 | | 06 00 00 00 84 00 00 00
| 10 | 447 | 17f0004b | E | 20 4b 00 00 00 00 00 80

> The "E" flag indicates an extended, 29-bit frame ID (CAN 2.0B)
