import tensorflow as tf
import tensorflow.keras


class PolyActivation(tensorflow.keras.layers.Layer):
    
    def __init__(self, coefs):
        super(PolyActivation, self).__init__()
        self.coefs = coefs
    
    def call(self, inputs):
        return tf.math.polyval(self.coefs, inputs)
    
    def get_config(self):
        config = super().get_config()
        config["coefs"] = self.coefs
        return config


class SquareActivation(PolyActivation):
    
    def __init__(self):
        super(SquareActivation, self).__init__([1., 0., 0.])
