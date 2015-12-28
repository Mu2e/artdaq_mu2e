#!/bin/bash
startTime=$1

mkdir -p /home/mu2edaq/daqlogs/traceBuffers/$startTime
cd /home/mu2edaq/daqlogs/traceBuffers/$startTime

pslurp -h /home/mu2edaq/pilotSystem.txt /tmp/trace_buffer_mu2edaq .
for host in `cat /home/mu2edaq/pilotSystem.txt`;do
    mv $host/trace_buffer_mu2edaq $host.trace
    rmdir $host
done
pssh -h /home/mu2edaq/pilotSystem.txt "rm /tmp/trace_buffer_mu2edaq"
