# ARIMA Time Series Inference Using FHE


## Introduction
 
This example demonstrates FHE training of an ARIMA(1,1,1) model on encrypted time series values. The train model is also used to predict the next value of the time series. The FHE prediction is compared with the plain ARIMA(1,1,1) prediction and the prediction results are shown to be close.

## Build

Change directory to the example's home directory, then execute:

    cmake .
    make

## Run

Run the ML inference over the encrypted dataset:

    ./arima_example
