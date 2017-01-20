#!/bin/bash
#This script sets up TRACE logging for the Mu2e Pilot System.

source /mu2e/ups/setup
setup TRACE v3_06_07 -qe10:prof

export TRACE_FILE=/tmp/trace_buffer_$USER
#export TRACE_FILE=/proc/trace/buffer

export TRACE_NAME=KERNEL
${TRACE_BIN}/trace_cntl lvlset `bitN_to_mask "0-22"` 0 0;
${TRACE_BIN}/trace_cntl lvlclr `bitN_to_mask "4"` 0 0
${TRACE_BIN}/trace_cntl lvlclr `bitN_to_mask "17-18"` 0 0
export TRACE_NAME=MU2EDEV

# Trace everything
${TRACE_BIN}/trace_cntl lvlset `bitN_to_mask "1-31"` 0 0;
# Trace much less
#${TRACE_BIN}/trace_cntl lvlset 0xF 0 0;
# Trace most things to memory buffer, also output to stdout
#${TRACE_BIN}/trace_cntl lvlset `bitN_to_mask "1-10"` `bitN_to_mask "1-4"` 0;
trace_cntl modeM 1
#trace_cntl modeS 1

