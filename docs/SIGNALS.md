# Signals
Description of internal signals.
This is useful for debugging.

## Fuzzing
In this scenario the controller periodically transmit
with the intention of seeing how the door responds.
- RTR
- data length 8
- no data

There are two subcases
- door is inactive and tranmit frames are not acked
- door is active and tanmit frames are acked

### No ack
The MCP25625 normally will retry the transmit automstically.
Several error states become active.

What has been observed:
- retransmission occurs almost immediately, 111 us
- about 21 us gap
- MERR error is set
- interrupt ICOD=0 which is MERR

The ISR is written to clear MERRF and clear TQEN.
Clearing TQEN prevents retransmission.
> Since retransmission occurs so quickly, clearing TQEN doesn't
> happen until the second transmit frame has been started.
> Hence there are two un-acked transmit frames.

In one-shot mode, OSM, retransmission is not automatically attempted.

### Ack
With the door control turned on, the fuzzing transmit frames are being acked.
No errors are set.
> There is still a periodic bus-off, which will be investigate in issue #11.


