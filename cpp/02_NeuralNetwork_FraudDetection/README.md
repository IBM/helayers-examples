# Neural Network Inference for Fraud Detection Using FHE


## Introduction
 
This example demonstrates a use case in the finance domain as well as demonstrating encrypted machine learning. We will demonstrate how we can use FHE along with neural networks (NN) to carry out predictions for fraud detection while keeping the data, the NN model and the prediction results encrypted at all times. The neural network and dataset determine fraudulent activities based on anonymized transactions. 

This example showcases how you are able to utilize the processing power of an untrusted environment while preserving the privacy of your sensitive data. The demonstration is split into a privileged client that has access to unencrypted data and models, and an unprivileged server that only performs homomorphic computation in a completely encrypted fashion. The data and the NN model are encrypted in a trusted client environment and then are used to carry out predictions in an untrusted or public environment. Finally, the prediction results return encrypted and can only be decrypted by the data owner in the trusted environment. The concept of providing fully outsourced, but fully encrypted computation to a cloud provider is a major motivating factor in the field of FHE. This use case example shows the capability of the SDK to build such applications.

**NOTE: while the client and server are not literally separated (nor demonstrating true remote cloud computation), the concepts generalize. One can imagine running the trusted code on local environment and the prediction code on a less trusted environment like the cloud. Additionally, we are working on FHE cloud that simplifies a lot of this.**

#### This demo uses the Credit Card Fraud Detection dataset, originally taken from: https://www.kaggle.com/mlg-ulb/creditcardfraud
This dataset contains actual anonymized transactions made by credit card holders from September 2013 and is labeled for transactions being fraudulent or genuine. See references at the bottom of the page. The network’s architecture is based on a notebook implemented by the Kaggle community (https://www.kaggle.com/omkarsabnis/credit-card-fraud-detection-using-neural-networks), while some changes was applied to make the network's architecture FHE friendly.

## Use case

Global credit card fraud is expected to reach $35B by 2025 (Nilson Report, 2020) and since the beginning of the COVID-19 pandemic, 40% of financial services firms saw an increase in fraudulent activity (LIMRA, 2020). As well as volume effects, COVID-19 has worsened the false positive issue for over two-thirds of institutions (69%). A key challenge for many institutions is that significant changes in consumer behavior have often resulted in existing fraud detection systems wrongly identifying legitimate behavior as suspected fraud (Omdia, 2021).

With FHE, you are now able to unlock the value of regulated and sensitive PII data in the context of a less trusted cloud environment by performing AI, machine learning, and data analytic computations without ever having to decrypt. By training your AI models with additional sensitive data, you are able to achieve higher accuracy in fraud detection and reduce the false positive rate while also utilizing the many benefits of cloud computing.

FHE can also help to support a zero trust strategy and can implement strong access control measures by keeping the data, the models that process the data and the results generated encrypted and confidential; only the data owner has access to the private key and has the privilege to decrypt the results.


## Build

Change directory to the example's home directory, then execute:

    cmake .
    make

## Run

Run the ML inference over the encrypted dataset:

    ./NeuralNetwork_FraudDetection


# References

1.	Andrea Dal Pozzolo, Olivier Caelen, Reid A. Johnson and Gianluca Bontempi. Calibrating Probability with Undersampling for Unbalanced Classification. In Symposium on Computational Intelligence and Data Mining (CIDM), IEEE, 2015
2.	Dal Pozzolo, Andrea; Caelen, Olivier; Le Borgne, Yann-Ael; Waterschoot, Serge; Bontempi, Gianluca. Learned lessons in credit card fraud detection from a practitioner perspective, Expert systems with applications,41,10,4915-4928,2014, Pergamon
3.	Dal Pozzolo, Andrea; Boracchi, Giacomo; Caelen, Olivier; Alippi, Cesare; Bontempi, Gianluca. Credit card fraud detection: a realistic modeling and a novel learning strategy, IEEE transactions on neural networks and learning systems,29,8,3784-3797,2018,IEEE
4.	Dal Pozzolo, Andrea Adaptive Machine learning for credit card fraud detection ULB MLG PhD thesis (supervised by G. Bontempi)
5.	Carcillo, Fabrizio; Dal Pozzolo, Andrea; Le Borgne, Yann-Aël; Caelen, Olivier; Mazzer, Yannis; Bontempi, Gianluca. Scarff: a scalable framework for streaming credit card fraud detection with Spark, Information fusion,41, 182-194,2018,Elsevier
6.	Carcillo, Fabrizio; Le Borgne, Yann-Aël; Caelen, Olivier; Bontempi, Gianluca. Streaming active learning strategies for real-life credit card fraud detection: assessment and visualization, International Journal of Data Science and Analytics, 5,4,285-300,2018,Springer International Publishing
7.	Bertrand Lebichot, Yann-Aël Le Borgne, Liyun He, Frederic Oblé, Gianluca Bontempi Deep-Learning Domain Adaptation Techniques for Credit Cards Fraud Detection, INNSBDDL 2019: Recent Advances in Big Data and Deep Learning, pp 78-88, 2019
8.	Fabrizio Carcillo, Yann-Aël Le Borgne, Olivier Caelen, Frederic Oblé, Gianluca Bontempi Combining Unsupervised and Supervised Learning in Credit Card Fraud Detection Information Sciences, 2019
9.	Yann-Aël Le Borgne, Gianluca Bontempi Machine Learning for Credit Card Fraud Detection - Practical Handbook
