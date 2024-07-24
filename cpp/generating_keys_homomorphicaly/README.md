# Generating rotation keys homomorphicaly example

This example demonstrates how to initialize a context that contain few rotation keys in the client and generate the rest of the rotation keys later on in the server. As rotation keys size is in the hundreds of Megabytes - this method enable reducing network traffic significantly.The compression ratio for the rotation keys can be configured in the config requirement. The default is about 50% reduction. For bootstrapping rotation keys there is a fix reduction of about 50%. By defualt the power of 2 rotations keys are generated. Note there is a tradeoff between network traffic and server runtime because smaller ratio will generate less rotation keys in the client and reduce network traffic by that ratio but it will require more homomorphic computation at the server side to generate the rest of the rotation keys.
There are 4 examples:
1. Generating default powers of 2 rotation keys homomorphicaly by ratio.
2. Generating custom rotation keys homomorphicaly by ratio.
3. Generating bootstrap and default rotation keys.
4. Generating bootstrap and custom rotation keys.

## Build

Change directory to the example's home directory, then execute:

    cmake .
    make

## Run

Run example:

    ./generating_keys_homomorphicaly_example
