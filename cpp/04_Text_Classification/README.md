# Text classification Using FHE for the *20 Newsgroups* dataset

## Introduction
This tutorial is based on the 20-newsgroups dataset and classifies a text snippet to its relevant category (snippet is represented as a bag of words vector with tf/idf scores). The tutorial is composed of two parts: the first part shows how to train the unencrypted model and the second part shows how to transform the plaintext model into an encrypted one and how to perform an FHE inference with it. The classifier model is a neural network (NN) with a single hidden layer and polynomial activation. In this tutorial, we used only 4 out of 20 available categories.  

#### This demo uses the 20 Newsgroups dataset, originally taken from: http://kdd.ics.uci.edu/databases/20newsgroups/20newsgroups.html

This data set consists of 20000 messages taken from 20 newsgroups.

## Use case
A potential use case for text classification is sentiment analysis. For example, take a scenario where a call center has the texts from customer calls and they would like to categorize it (e.g. loans, general complaints, investments, etc.) but the calls are considered to be sensitive data. With FHE, you can encrypt the text and categorize the calls in order to analyze customer sentiment - all while preserving customer privacy.


## Build

Change directory to the example's home directory, then execute:

    cmake .
    make

## Run

Run the ML inference over the encrypted dataset:

    ./Text_Classification

    <br>
