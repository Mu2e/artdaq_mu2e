#!/bin/bash
#This script sets up TRACE logging for the Mu2e Pilot System.

source /mu2e/ups/setup
setup TRACE v3_05_00a -qe7

export TRACE_FILE=/tmp/trace_buffer_$USER
export TRACE_NAME=MU2EDEV

# Trace everything
${TRACE_BIN}/trace_cntl lvlset `bitN_to_mask "1-5"` 0 0;
# Trace much less
#${TRACE_BIN}/trace_cntl lvlset 0xF 0 0;
# Trace most things to memory buffer, also output to stdout
#${TRACE_BIN}/trace_cntl lvlset `bitN_to_mask "1-10"` `bitN_to_mask "1-4"` 0;
trace_cntl modeM 1
#trace_cntl modeS 1

