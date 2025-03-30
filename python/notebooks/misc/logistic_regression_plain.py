import numpy as np
import time


class LogisticRegression:
    """
    Set the default learning rate to 0.1 and default number of iterations to 5
    """

    def __init__(self, learning_rate=0.1, n_iters=5, weights=None, bias=None):
        self.lr = learning_rate
        self.n_iters = min(
            n_iters, 5
        )  # force 5 to be the maximum number of iterations used
        self.weights = weights
        self.bias = bias
        self.y_predicted_probabilities = None

    """ 
    Fit method to train the LR model.
    Set weights vector to a given or to a zero vector with a size equal to the number of features.

    """

    def fit(self, X, y):
        n_samples, n_features = X.shape
        # init parameters
        if self.weights == None:
            self.weights = np.zeros(n_features)
        if self.bias == None:
            self.bias = 0
        begin = time.time()
        self.weights = self.weights.reshape(-1, 1)
        for _ in range(self.n_iters):
            # approximate y with linear combination of weights and x, plus bias
            linear_model = np.dot(X, self.weights) + self.bias
            # use the polynomial sigmoid
            y_predicted = self._poly_sigmoid(linear_model)
            # compute gradients
            dw, db = self._gradients(X, y_predicted, y)
            # update parameters
            self.weights -= self.lr * dw
            self.bias -= self.lr * db
        self.y_predicted_probabilities = y_predicted
        end = time.time()
        print(f"\nTime taken for training is {end - begin} seconds")

    """
    HE-friendly activation:
    Polynomial degree 3 approximation for the sigmoid function
    f(X) = 0.5 + 1.20096/8 * x - 0.81562/512 * x^3
    """

    # Approximated poly sigmoid
    def _poly_sigmoid(self, x):
        x2 = x * x
        x3 = x2 * x
        sig3 = 0.5 + np.dot(0.15, x) - np.dot(0.001593, x3)
        return sig3

    # Return computed derived weights (dw) and derived bias (db)
    def _gradients(self, X, ypred, y):
        return 2.0 * np.dot(X.T, (ypred - y)) / y.shape[0], np.sum(ypred - y) / y.shape[0]

    def predict(self, x_test):
        linear_model = np.dot(x_test, self.weights) + self.bias
        y_predicted = self._poly_sigmoid(linear_model)
        return np.array([1 if obj >= 0.5 else 0 for obj in y_predicted])
