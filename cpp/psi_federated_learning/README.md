## Private Set Intersection for Federated Learning demo

This directory contains two examples that demonstrate a Private Set Intersection (PSI) protocol between several parties, using the help of a third party called the Aggregator. This PSI protocol can be used, for example, as part of a Vertical Federated Learning (VFL) protocol. To make things more concrete, we will focus on this use case in the following demos.

Side note - The separate Entity Resolution (ER) demo deals with a similar challenge of identifying similar records between the participants. The main differences between the protocols are that in this protocol we require a unique identifier (UID) for every sample, whereas in the ER protocol there is no such requirement (in fact, the ER protocol is particularly suitable for the use-cases where there are no UIDs). Furthermore, this protocol hides from the participants which samples are in the intersection, and even how much samples are in the intersection, whereas the ER protocol is designed to reveal these samples to the participants. 
 
In VFL, the dataset is partitioned vertically between the parties, meaning that each party possesses a distinct subset of the features of the dataset. Furthermore, in most scenarios, each party possesses only a (non-distinct) subset of the samples, meaning that a given sample may appear in one party's dataset but not in the other's. Therefore, a major part of many VFL protocols is the alignment part, where the parties identify their shared samples, and align them to appear in the same order. In some scenarios, the parties want to perform this alignment process in a privacy-preserving way. The following examples are about solving this issue.

In the first example, psi_federated_learning, the parties Alice and Bob both have their own DB with private samples. Each sample contains a unique identifier (UID) and number of features, each is a real number in range [-1,1]. The objective is that Alice will get by the end of the protocol a CTileTensor (an encrypted tensor) under the aggregator' key, containing only the samples which are in the intersection, in a way such that no party learns anything about the other party's samples or about the intersection. For example, Alice shouldn't learn whether a specific sample is in the intersection or not, and the aggregator shouldn't learn anything about Alice's and Bob's samples. The CTileTensor then can be further used by Alice in some learning algorithm. The result then can be sent to the aggregator who can decrypt it and send it to Alice and Bob.

The second example, psi_multiple_parties, demonstrates a Private Set Intersection (PSI) protocol between more than two parties.

## Build
Change directory to the examples/psi_federated_learning directory, then execute:

    cmake .
    make

## Run and Validate
Run the example 

    ./psi_federated_learning

or

    ./psi_multiple_learning

The demos would by default compare just 3 samples of each party, and will take 8 minutes for the first demo to run and 13 minutes for the second demo.
You may ask to compare more samples by using the num_samples optional flag:

* `num_samples` - int, determines the number of samples to compare from the parties' databases
* `verbose` - sets the verbosity level to high verbosity

For Example run the following command to compare 5 samples from Alice's DB with 5 samples from Bob's DB (takes 9 minutes to run):

    ./psi_federated_learning --num_samples 5

Samples are randomly generated, so any number of samples can be provided. The time performance is O(num_samples^2) seconds, e.g., 100 samples will take 3 hours.