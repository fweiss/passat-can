# Perfomance
Observations and ideas for improving performance

## interrupt
reading five registers and clearing the interrupt flag
about 250 us INT L to H
interrupt latency 40 us
each read is 24 clocks @ 10 ns
time between reads 42 us

expect about 80 us by doing an array read
