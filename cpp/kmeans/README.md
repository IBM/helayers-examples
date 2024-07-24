# K-Means Example

This example demonstrates a k-means computation. The demo uses the low level HElayers HeModel API that allows finer control than the regular API demonstrated in other demos.

An `helayers` Context is first generated, and then saved both *with* (client side) and *without* (sever side) the private key.  Subsequent flows are split between client and server function.

First, the KMeans instance is generated from the k-means centers file.  The centers are a list of the centroids for the k-means computation, each of which is a vector of doubles representing the coordinates of the centroid.  The list of centroids is converted to a Boost tensor.

The ctor used is:

```
KMeans(HeContext& he, const boost::numeric::ublas::tensor<double> centroids, int ncenters2, int dimension2)
```

Where

* `he` is the HeContext
* `centroids` is the tensor of the centers
* `ncenters2` is the number of centers rounded up to the next power of two
* `dimension2` is the dimensionality of each point rounded up to the next power of two

These parameters are necessary because of the formatting of the data required.

Next the input data is encoded on the client side.  The input data for each point is a vector of doubles, and the total input data is a vector of those vectors.  The input data is read as a Boost tensor, and passed to the `encodeEncryptTestData` static method of the LinearRegression class.  It is necessary to pass the TTShape value for the KMeans instance to the `encodeEncryptTestData` method to propagate the centroid dimensions.  The encrypted test data is written to a file.

On the server side, the `predict` method is called to do the actual linear regression.  The input is a CTileTensor containing the encrypted input data, and the output is another CTileTensor.  The encrypted results are written to a file.

Back on the client side, the `decryptResults` static method is called to decrypt the data.  The result is a Boost tensor of the regression values.  You must pass the actual number of centers, the dimension of each point, and the actual number of inputs to `decryptResults`.  

## Build

Change directory to the example's home directory, then execute:

    cmake .
    make kmeans_seal_low_level_api

## Run

Run K-Means example:

    ./kmeans_seal_low_level_api

