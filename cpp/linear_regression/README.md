# Linear Regression Example

This example demonstrates linear regression. The demo uses the low level HElayers HeModel API that allows finer control than the regular API demonstrated in other demos.

An `helayers` Context is first generated and saved. The secret key is saved separately for the client side.  Subsequent flows are split between client and server function.

First, the LinearRegression instance is generated from the `weights` and `biases` files.  The weights are a linear array of doubles representing the coefficients for the regression.  The biases are a single double representing the offset for the regression.  These sets of values are converted to Boost tensors and passed to the LinearRegression ctor.

Next the input data is encoded on the client side.  The input data for each point is a vector of doubles, and the total input data is a vector of those vectors.  The input data is read as a Boost tensor, and passed to the `encodeEncrypt` method of the ModelIoEncoder class which is generated from the model.  The encrypted test data is written to a file.

On the server side, the `predict` method is called to do the actual linear regression.  The input is a CTileTensor containing the encrypted input data, and the output is another CTileTensor.  The encrypted results are written to a file.

Back on the client side, the `decryptResults` static method is called to decrypt the data.  The result is a Boost tensor of the regression values.

## Build
Change directory to the example's home directory, then execute:

    cmake .
    make linear_regression_low_level_api

## Run

Run Linear Regression example:

    ./linear_regression_low_level_api

