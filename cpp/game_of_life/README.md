# Conway's game of life Example

This example demonstrates how the use of a cleanup procedure in CKKS enables deep computation of Conway's game of life for unlimited rounds. This computation involves hundreds of bootstrapping operations without errors when compared to plaintext version of the computation. Without the clean up procedure the computation fails after few rounds.

## Build

Change directory to the example's home directory, then execute:

    cmake .
    make

## Run

Run example:

    ./game_of_life
