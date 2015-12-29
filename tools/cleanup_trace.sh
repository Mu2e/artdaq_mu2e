#!/bin/bash
startTime=$1

mkdir -p /home/mu2edaq/daqlogs/traceBuffers/$startTime

rgang --do-local "mu2edaq0{1,4-8}" 'mv /tmp/trace_buffer_$USER /home/mu2edaq/daqlogs/traceBuffers/$startTime/`hostname`.trace'
