# Door ECM
Information about connecting the door control module

## Module identification
- 5Q0 959 592 D
- HW REF 5Q0 959 592 B
- marked LL andf RL, but not sure what that means
- some parts say front right
- handwritten B8/8 PK
- purchased $14.75 + S&H Nov 18, 2023 on Ebay

## Receive
When the module is powered up it emits some frames for five seconds.
The CAN bus is at 500 kbps.

| Count | Period | ID      | Flags | Data |
| ---: | ------: | --------: | --- | --- 
| 20 | 189 | 1b00004b | E | 4b 00 01 04 04 00 00 00 |
| 50 | 98 | 3d1 | | 06 00 00 00 84 00 00 00
| 10 | 447 | 17f0004b | E | 20 4b 00 00 00 00 00 80

> The "E" flag indicates an extended, 29-bit frame ID (CAN 2.0B)

## door control hardware
There are 6 logic chips:
- NEC EX2-N15: DPDT relay for regulator motor
- STL99PM626XP: power management with LIN and high-speed CAN
- ST 2904 GZ404: dual op-amp? maybe pinch detection
- NEC/Renesas 70f3624: MCU
- ST L99DZ70XP: 6-bridge driver for door lock, mirror motors
- TI CM0510B: 8-channel analog mux/demux, switches?

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

## MCU
- Renesas?
- 70f3624
- 1452EEC08
- (A)
- SGP

NEC?
Renesas [V850ES](https://www.renesas.com/us/en/document/mah/v850esfx3-user-manual-hardware?r=1055686) ?
32 bit microcontroller
with 2 CAN controllers

from chatgpt:
TXD (Transmit): Pin P1_6, IC 22 TXDD0
RXD (Receive): Pin P1_7, IC 23 RXDD0
[carpro tool programmer](https://carprotool.com/download/Pinouts/CarProTool%20Programmer/CPT%20NEC%20V850/NEC%20V850%2064%20PINs.PNG)

## TI CM051BQ
CD405xB
8 channel analog mux/demux ?
There are pads for an extra one, maybe for the driver's side?

## connectors
quick connect
2.77 x 0.83 mm

## Motors
- 2016 Passat window regulator: 5-15 A
- JGY-370: 12V45RPM 60-500 mA

## wiring
see BCM.png
L to R

### S001 (6)
(3) (6)
- 3 - window regulator motor +
- 6 - window regulator motor-

### S002 (20)
- (20) (19)
- (18..10)
- (9..1) edge
- 20 - 31/B-
- 19 - 30/B+
- 18 - NC
- P-15 CANH
- P-14 CANL
- 13 - central locking motor
- 12 - central locking safelock motor
- 11 - common for 13, 12
- 10 - ??? 250
- 9 - NC
- 6 - central locking switch
- 5 - door contact switch
- P-10 LIN bus

### S003 (16)
- (16..9)
- (8..1)
- 16 - side mirror position horizontal
- 15 - heated exterior mirror
- 14 - side mirror position com a
- 13 - side mirror position vertical
- 11 - turn signal repeater
- 10 - side mirror fold-in motor
- 9 - side mirror fold-in motor
- 8 - side mirror motor horizontal
- 7 - side mirror motor vertical
- 6 - anti-dazzle exterior mirror -
- 5 - side mirror motor com
- 6 - anti-dazzle exterior mirror +
- 4 - side mirror position com b
- 3 - heated exterior mirror
- 1 - entry light exterior mirror

verstellmotor x3

### S004 (32)
- bottom to top
- [32..17]
- [16..1]
- taster
- P-23 heckdeckelentriegelung
- 32 - window regulator switch
- 28 - xxx
- 16 - side door warning lamp
- 13 - xxx interior locking button
- 8 - door background lighting
- 5 - interior door handle lamp (rhd)
- 5 - gnd
- 4 - window regulator lamp
- 3 - interior door handle lamp (lhd)

## Links and References
[Supplier of industrial CAN modules](https://www.ametekvis.com/products/can-control-modules)

[Passat B8 Wiring Diagrams](https://www.scribd.com/document/431058825/Vw-Passat-b8-Wiring-Diagrams-Eng)

[downloaded]()
