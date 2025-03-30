# Bitwise comparison

This example demonstrates how to compare 2 numbers a,b that are given as bit-arrays where every bit is encrypted in a different ciphertext. This example computes 2 (ciphertext) bits: the first holds 1 if a=b and 0 otherwise; the other holds 1 if a < b  and 0 otherwise.

## Build

Change directory to the example's home directory, then execute:

    cmake .
    make

## Run

Run example:

    ./bitwise_cmp
