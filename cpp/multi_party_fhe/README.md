## Multi Party FHE

This example demonstrates a use of a multi-party FHE setting. In this setting, a
set of parties wish to compute some function over their secret data while not
revealing it to any of the other parties. Using a regular public-key setting
will not be secure, since it requires the parties to trust the holder of the
secret key (whether it is one of the parties or a "trusted" third party). In the
multi-party FHE setting, none of the parties has a hold on the secret key.
Instead, each party has its own secret key (therefore, it will also be called a
"key-owner" later on). The public keys (which includes the encryption key and
the evaluation keys) are generated in a initialization protocol (a.k.a
InitProtocol) between the parties. To decrypt a ciphertext, each of the parties
(key-owners) needs to give its consent and to take part in a decryption protocol
(a.k.a. DecryptProtocol).

In the following example we consider the case of 2 data owners - Alice and Bob - and
a server. Alice and Bob each has a secret vector, and they wish to compute the inner product of their vectors using the server.

## Build
Change directory to the example's home directory, then execute:

    cmake .
    make multi_party_fhe

## Run and Validate
Run the example:

    ./multi_party_fhe