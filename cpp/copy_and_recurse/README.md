## Range Query

This directory contains an example that demonstrates range queries over encrypted databases using the copy-and-recurse method, introduced in
> Hayim Shaul, Eyal Kushnir, and Guy Moshkowich. 2024. Secure Range-Searching Using Copy-And-Recurse. PoPETs 2024. 
>
> **NOTE:** When using this code in a paper, please cite this PETS paper.

In the example we consider the scenario of a data-owner who encrypts its database and uploads it to a server. In a later time the same client wishes to perform some range query over the encrypted database. For example, the client wishes to find out how many elements in the database are contained in a certain range with respect to some specific column in the database. Answering this query has a lower bound of $\Omega(n)$ and using copy-and-recurse the FHE-code performs $O(\log n)$ comparisons (like the naive algorithm) with an overhead of $O(n)$ multiplications. See [PETS'24](https://eprint.iacr.org/2023/983.pdf) for more details.

## Build
Change directory to the examples/cpp/copy_and_recurse directory, then execute:

    cmake .
    make

## Run and Validate
Run the examples 

    ./count_query_example 
    ./emptiness_query_example 

The demos would by default compute a query over a database of 16 elements. You may ask to compare more elements by using the following optional flag:

* `elements` - int, determines the number of samples to compare from the parties' databases

For Example run the following command to compute a range query over a database of 500 elements:

    ./count_query_example --elements 500

Samples are randomly generated, so any number of samples can be provided.