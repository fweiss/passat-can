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

# door control hardware

## NEC EX2-N15
relay
2 x SPDT
so can function as DPDT
but with separate relay coils

contacts around 30 A, 4 mOhm

coil 160 ohm, ~75 mA

## STL99PM626XP
power management with LIN and high-speed CAN

uses SPI interface.

On the circuit board, it looks like pins 34 and 35 go to the relay.
Indeed, they are the REL1 and REL2 signals, low-side driver output 2 ohm.
Looks like up to 200 mA.
Control register 1, bit 6, 7.

## ST GZ505
dual op amp ?
maybe for "pinch" detection

## ST L99DZ70XP
Door actuator driver with 6 bridges for double door lock control, mirror fold and mirror axis control, highside driver for mirror
SPI interface

## 70f3624
Renesas [V850ES](https://www.renesas.com/us/en/document/mah/v850esfx3-user-manual-hardware?r=1055686) ?
32 bit microcontroller
with 2 CAN controllers

## TI CM051BQ
CD405xB
8 channel analog mux/demux ?

## connectors
quick connect
2.77 x 0.83 mm

## wiring
see BCM.png
L to R

### S001 (6)
(3) (6)
3 - motor+
6 - motor-

### S002 (20)
(20) (19)
(18..10)
(9..1) edge
20 - 31/B-
19 - 30/B+
18 - NC
9 - NC
P-10 LIN bus
P-14 CANL
P-15 CANH

### S003 (16)
(16..9)
(8..1)
verstellmotor x3

### S004 (32)
bottom to top
[32..17]
[16..1]
taster
P-23 heckdeckelentriegelung
