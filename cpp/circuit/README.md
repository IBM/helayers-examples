# circuit Tutorials
CircLayer is a layer in HElayers that converts an FHE code into an arithmetic circuit, performs manipulations and optimization on the circuit and then executes the circuit in the gate level.
Treating the code as an arithmetic circuit at the gate level opens the gate to many optimizations and features.

## Build
Change directory to the tutorial's home directory, then execute:

    cmake .
    make

## Basic logging
This tutorial shows a basic example where we build a small circuit and log it onto stdout.
Open `tut_1_basics_log.cpp` and explore the code comments. Run the basic logging tutorial by executing:

    ./tutorial_circuit mockup 1

## Basic running
This tutorial shows a basic example where we build a small circuit and then run it.
Open `tut_2_basics_run.cpp` and explore the code comments. Run the basic running tutorial by executing:

    ./tutorial_circuit mockup 2

## Plaintext
This tutorial shows a basic example where we build a small circuit that involves operations between ciphertexts and plaintexts and then run it.
The plaintext values involved in the computation will be embedded inside the circuit as constant (visible) parameters inside it.
Open `tut_3_run_plaintext.cpp` and explore the code comments. Run the plaintext tutorial by executing:

    ./tutorial_circuit mockup 3

## Parameters
This tutorial shows a basic example where we build a small circuit where the computation involves constant parameters that are encrypted.These parameters are not part of the input, and are embedded inside the circuit, same as in tutorial 3. However, since they should be hidden, they are encrypted and stored in a separate object.
As a real-life motivating example, consider a neural network circuit. The input to the network is provided by the user at run-time, but the network parameters are considered part of the circuit itself. We can choose to keep them in plaintext as in tutorial 3, or encrypt them as shown here.
Open `tut_4_run_params.cpp` and explore the code comments. Run the params tutorial by executing:

    ./tutorial_circuit mockup 4

## Tile Tensors
This tutorial shows a basic example where we build a small circuit that uses tile tensors and run it.
Open `tut_5_run_tile_tensors.cpp` and explore the code comments. Run the tile tensors tutorial by executing:

    ./tutorial_circuit mockup 5
