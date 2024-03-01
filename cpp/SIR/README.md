# About

This code implements feature selection for Ridge regression as described in:

"Privacy Preserving Feature Selection for Sparse Linear Regression"
by Adi Akavia, Ben Galili, Hayim Shaul, Mor Weiss and Zohar Yakhini.


The code uses the SEAL and HElib libraries for FHE implementation.

# To Compile

```
cd liphe/src
make
cd ../../sir
make sir_fhe
```

# To Run

## Generating synthetic data
Generate data with 4 features and 784 records.
```
python ../datasets/generate_data.py 4 784 ../datasets/d4x1.csv
```

## Running the full system


Select 9 features from the given 40 features in a dataset with 784 records.
```
./sir_fhe --r=3 --primeBits=30 --primeNumber=42 --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d40x1.csv --s=9
```

Select 2 features from the given 4 features in a dataset with 784 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d4x1.csv --primeNumber=6
```





## Running with different dataset size


Select 2 features from the given 4 features in a dataset with 200,704 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d4x256.csv --primeNumber=6
```

Select 2 features from the given 4 features in a dataset with 401,408 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d4x512.csv --primeNumber=7
```

Select 2 features from the given 4 features in a dataset with 802,816 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d4x1024.csv --primeNumber=9
```

Select 2 features from the given 10 features in a dataset with 1,568 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x2.csv --primeNumber=12
```

Select 2 features from the given 10 features in a dataset with 784 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10csv --primeNumber=12
```

Select 2 features from the given 10 features in a dataset with 3,136 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x4.csv --primeNumber=12
```

Select 2 features from the given 10 features in a dataset with 6,272 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x8.csv --primeNumber=14
```

Select 2 features from the given 10 features in a dataset with 12,544 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x16.csv --primeNumber=15
```

Select 2 features from the given 10 features in a dataset with 50,176 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x64.csv --primeNumber=16
```

Select 2 features from the given 10 features in a dataset with 100,352 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x128.csv --primeNumber=17
```

Select 2 features from the given 10 features in a dataset with 200,704 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x256.csv --primeNumber=18
```

Select 2 features from the given 10 features in a dataset with 401,408 records.

```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x512.csv --primeNumber=18
```

Select 2 features from the given 10 features in a dataset with 802,816 records.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19
```



# Running with different data owners

All these experiments were done on a dataset with 10 features out of which 4 were selected.


Running with 500 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=500
```

Running with 200 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=200
```

Running with 700 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=700
```

Running with 100 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=100
```

Running with 300 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=300
```

Running with 400 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=400
```

Running with 600 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=600
```

Running with 800 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=800
```

Running with 900 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=900
```

Running with 1000 data owners.
```
./sir_fhe --r=3 --p=1056342016 --primeBits=30  --sealChainLength=7 --mSeal=8192 --sealChainBits=25 --in=../datasets/d10x1024.csv --primeNumber=19 --do=1000
```
