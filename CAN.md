https://hackaday.io/project/6288/instructions

https://www.vector.com/us/en/know-how/protocols/sae-j1939/#c94570

[Very good technical overview of CAN](https://training.ti.com/automotive-can-overview#:~:text=The%20CAN%20bus%20allows%20for,expensive%20components%20in%20a%20car.)

More in-depth info on J1939, PGN, SPN
https://www.csselectronics.com/pages/j1939-explained-simple-intro-tutorial

[How to query ECUs on VW](https://forum.macchina.cc/t/how-to-read-vw-can-bus/655)

Fram a chatGPT query:
- engine speed
- from ECM
- standard PID "PID 01 0C" (Engine RPM)
- 2 bytes, big-endian

## Frame, MCP25625, WS frame

### on the wire
start
sid[10:0]   TB0SIDH=sid[10:3] TB0SIDL=sid[2:0]
rtr         TXB0DLC[6] aka SSR
ide         TB0SIDL[3]
eid[17:0]   TXB0SIDL=eid[17:16] TXB0EID8=eid[15:8] TXB0ID0=EID[7:0]
--- aka RTR
r0
--- ext r1
dlc[3:0]    TXB0DLC[3:0]
data[]      TB0D[]
crc[14:0]
crcdelim
ack
ackdelim
eof[6:0]

### register order
SIDH
SIDl
EID8
EID0
DLC
D0 - D7

### c++ model
```
struct {
    uint32_t identifier;
    uint8_t dataLength;
    uint8_t data[8]; // or pointer
    bool ide;
    bool rtr;
}
```