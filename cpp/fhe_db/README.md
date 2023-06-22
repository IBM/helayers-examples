# FHE Database Example

This example demonstrates computations over an encrypted database.  

The table is loaded from examples/data/db/txsmillion11Bits.csv, only to be encrypted with a secure SEAL CKKS configuration. The table contains two "hybrid bitwise" columns ('tx_id' and 'client_id') with 11 mockup bits and a third numerical column ('tx_sum'), on which operations will be performed.

The following operations are performed on the encrypted table:
* COUNT * WHERE <col> IS_EQUAL <val>
* COUNT * WHERE <col> IS_GREATER <val>
* SUM <col1> WHERE <col2> IS_EQUAL <val>
* SUM <col1> WHERE <col2> IS_GREATER <val>
* AVG <col1> WHERE <col2> IS_EQUAL <val>
* AVG <col1> WHERE <col2> IS_GREATER <val>
* STDDEV <col1> WHERE <col2> IS_EQUAL <val>
* STDDEV <col1> WHERE <col2> IS_GREATER <val>

Some post processing is then performed on the encrypted query results to get the final answers. This post processing includes decryption and summation over the ciphertexts' slots, and is performed by Table::postprocess*() functions.


## Build

Change directory to the example's home directory, then execute:

    cmake .
    make fhe_db

## Run

Run fhe_db example:

    ./fhe_db

