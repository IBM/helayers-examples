## Range Query

This directory contains an example that demonstrates range query over an encrypted database. In the example we consider the scenario of a data-owner who encrypts its database and uploads it to a server. In a later time the same client wishes to perform a range query over the encrypted database. Namely, the client wishes to find out how many elements in the database are contained in a certain range, with respect to some specific column in the database.

## Build
Change directory to the examples/cpp/query_database directory, then execute:

    cmake .
    make

## Run and Validate
Run the example 

    ./range_query_example 


The demo would by default compute a range query over a database of 16 elements and a single column.
You may ask to compare more samples by using the num_elements optional flag:

* `num_elements` - int, determines the number of samples to compare from the parties' databases

For Example run the following command to compute a range query over a database of 500 elements:

    ./range_query_example --num_elements 500

Samples are randomly generated, so any number of samples can be provided.
