# Logistic Regression Prediction Demo - Credit Card Fraud Detection

This demo demonstrates running logistic regression model inference when both model and data are encrypted.

## Build
Change directory to the example's home directory, then execute:

    cmake .
    make logistic_regression

## Run and Validate
Run the example:

    ./logistic_regression


# Logistic Regression Training Demo - Credit Card Fraud Detection

This example demonstrates how an encrypted logistic regression (LR) model can
be trained with encrypted data. Predictions are also carried for validation of
the trained model. Prediction results are encrypted and sent back to the data
owner to be decrypted and assessed.

## Build
Change directory to the example's home directory, then execute:

    cmake .
    make logistic_regression_training

## Run and Validate
Run the example:

    ./logistic_regression_training


## References:

<sub><sup> 1.	Andrea Dal Pozzolo, Olivier Caelen, Reid A. Johnson and Gianluca Bontempi. Calibrating Probability with Undersampling for Unbalanced Classification. In Symposium on Computational Intelligence and Data Mining (CIDM), IEEE, 2015 </sup></sub>

<sub><sup> 2.	Dal Pozzolo, Andrea; Caelen, Olivier; Le Borgne, Yann-Ael; Waterschoot, Serge; Bontempi, Gianluca. Learned lessons in credit card fraud detection from a practitioner perspective, Expert systems with applications,41,10,4915-4928,2014, Pergamon </sup></sub>

<sub><sup> 3.	Dal Pozzolo, Andrea; Boracchi, Giacomo; Caelen, Olivier; Alippi, Cesare; Bontempi, Gianluca. Credit card fraud detection: a realistic modeling and a novel learning strategy, IEEE transactions on neural networks and learning systems,29,8,3784-3797,2018,IEEE </sup></sub>

<sub><sup> 4.	Dal Pozzolo, Andrea Adaptive Machine learning for credit card fraud detection ULB MLG PhD thesis (supervised by G. Bontempi) </sup></sub>

<sub><sup> 5.	Carcillo, Fabrizio; Dal Pozzolo, Andrea; Le Borgne, Yann-Aël; Caelen, Olivier; Mazzer, Yannis; Bontempi, Gianluca. Scarff: a scalable framework for streaming credit card fraud detection with Spark, Information fusion,41, 182-194,2018,Elsevier </sup></sub>

<sub><sup> 6.	Carcillo, Fabrizio; Le Borgne, Yann-Aël; Caelen, Olivier; Bontempi, Gianluca. Streaming active learning strategies for real-life credit card fraud detection: assessment and visualization, International Journal of Data Science and Analytics, 5,4,285-300,2018,Springer International Publishing </sup></sub>

<sub><sup> 7.	Bertrand Lebichot, Yann-Aël Le Borgne, Liyun He, Frederic Oblé, Gianluca Bontempi Deep-Learning Domain Adaptation Techniques for Credit Cards Fraud Detection, INNSBDDL 2019: Recent Advances in Big Data and Deep Learning, pp 78-88, 2019 </sup></sub>

<sub><sup> 8.	Fabrizio Carcillo, Yann-Aël Le Borgne, Olivier Caelen, Frederic Oblé, Gianluca Bontempi Combining Unsupervised and Supervised Learning in Credit Card Fraud Detection Information Sciences, 2019 </sup></sub>

<sub><sup> 9.	Yann-Aël Le Borgne, Gianluca Bontempi Machine Learning for Credit Card Fraud Detection - Practical Handbook </sup></sub>
