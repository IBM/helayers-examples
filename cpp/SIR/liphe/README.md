# liphe
Library of Practical Homomorphic Encryption algorithms.
===

Intruduction
---

This C++ library aims at helping researchers and developers use Homomorphic Encryption and Fully Homomorphic Encryption efficiently in practical projects.
The library provides non-trivial algorithms that are optimized to work better in FHE model.

The BinarySearch (SPiRiT) algorithm was described in the paper:

> **Secure Search on Encrypted Data via Multi-Ring Sketch** *Adi Akavia, Dan Feldman and Hayim Shaul*. To appear in CCS 2018.

Requirements
---
The library requires a Linux environment, a c++11 compiler. In addition it requires the HElib, NTL and GMP libraries.

Installation
---
To compile the library, edit the Makefile and update the directories of of the required libraries.

then run
```
cd src
make
```

Testing
---
To compile the tests:

```
cd tests
```
Then edit the Makefile to point the directories of the HElib, NTL and GMP libraries. Then compile:

```
make
```

and run the test suite for the HElib implementation

```
./test_helib
```


To test the Binary Search (SPiRiT) algorithm run
```
./test_spirit_helib
```


The tests were successful on Ubuntu 16 with
- g++ 5.4.0
- HElib
- NTL 10.5.0
- GMP 6.1.0



Feedback / Citation
---
Please send any feedback to Hayim Shaul (<hayim@csail.mit.edu>, <hayim.shaul@gmail.com>).
If you use this library please cite it.
If an algorithm you used was described in a paper, please cite that paper as well.

License
---
The software is released under the MIT License as detailed in `LICENSE`.



