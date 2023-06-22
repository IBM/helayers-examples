#!/usr/bin/env python3

# MIT License
#
# Copyright (c) 2020 International Business Machines
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# This short script demonstrates computing Euclidean
# distance under HE.
# It optimizes resource usage assuming the server only computes Euclidean distance.

# Note that we compute the Euclidean distance squared, which is much easier to compute
# under HE than the actual Euclidean distance. The two are monotonically related, and therefore
# For many purposes it doesn't matter.

try:
    import pyhelayers
except ImportError:
    import pyhelayerslite as pyhelayers
print(pyhelayers.__name__, pyhelayers.VERSION)

import numpy as np
import math
import time


# We initialize the context.
# Since our computation is shallow, we can configure low depth
# And small number of slots.
print("Initializing context . . .")
requirement = pyhelayers.HeConfigRequirement(
    num_slots = 2048, # Number of slots per ciphertext
    multiplication_depth = 1, # Allow 1 levels of multiplications
    fractional_part_precision = 29, # Set the precision to 1/2^29.
    integer_part_precision = 11, # Set the largest number to 2^11.
    security_level = 128)


# This step inits the context and generates the keys.
# By default, multiple keys are generated allowing arbitrary computations.
# For our purposes we customize the generation for just the keys we want:
# We assume our vectors have 128 elements. Therefore to compute the sum (of squares)
# we need rotation keys that are powers of 2 up to (excluding) 128.
# Also, we exclude the conjugation key.
# By default, a relinearization key needed for multiplication is included.
he_context = pyhelayers.DefaultContext()
pf1=pyhelayers.PublicFunctions()
pf1.rotate=pyhelayers.RotationSetType.CUSTOM_ROTATIONS
pf1.set_rotation_steps([1,2,4,8,16,32,64])
pf1.conjugate=pyhelayers.ConjugationSupport.FALSE
requirement.public_functions=pf1
he_context.init(requirement)


# Save the eval key to be used on the server side.
# To reduce it to the bare minimum we turn off the encryption key as well.
pf2=requirement.public_functions
pf2.encrypt=False
evalBuf=he_context.save_to_buffer(pf2)
print('Eval key: {:.3f} MB'.format(len(evalBuf)/1024/1024))

# For the client side:
# We just need the encryption key, so we clear all others.
# (The secret key is saved separately, see below).
pf3=pyhelayers.PublicFunctions()
pf3.clear()
pf3.encrypt=True
encryptBuf=he_context.save_to_buffer(pf3)
print('Encrypt key: {:.3f} MB'.format(len(encryptBuf)/1024/1024))

# Save the secret key for the client side:
secBuf=he_context.save_secret_key(True)
print('Secret key: {} Bytes'.format(len(secBuf)))

# We now start a fresh run on the client side.
# We clear the context we created. The client will load what it needs.
he_context=None

# Create our sample vectors
v1=np.random.normal(size=128)
v2=np.random.normal(size=128)
euc=np.linalg.norm(v1-v2)
print("Euclidean distance is {:.9f}".format(float(euc)))

# Encrypt vectors.
# We load the context with the encryption key for this purpose.
enc_he_context=pyhelayers.DefaultContext()
enc_he_context.load_from_buffer(encryptBuf)
encoder = pyhelayers.Encoder(enc_he_context)
c1 = encoder.encode_encrypt(v1)
c2 = encoder.encode_encrypt(v2)

# Save the encrypted vectors and clear memory.
c1Buf = c1.save_to_buffer()
c2Buf = c2.save_to_buffer()
enc_he_context = None
c1 = None
c2 = None

#
# Server side: compute Euclidean distance
#

# Load the context with the keys required for evaluation: namely the rotation keys
# And the relinearization key for the multiplication.
eval_he_context=pyhelayers.DefaultContext()
eval_he_context.load_from_buffer(evalBuf)

# Load the encrypted vectors
server_c1 = pyhelayers.CTile(eval_he_context)
server_c1.load_from_buffer(c1Buf)
server_c2 = pyhelayers.CTile(eval_he_context)
server_c2.load_from_buffer(c2Buf)

# Compute the euclidean distance
print("And now homomorphically . . . ")
start = time.time()
server_c1.sub(server_c2)
server_c1.square()
# This sums the first 128 elements in the ciphertext
server_c1.inner_sum(1,128)
end = time.time()

#  Save result and clear memory
print("Duration of homomorphic Euclidean distance calculation: {:.6f} seconds".format(end - start))
encResBuf = server_c1.save_to_buffer()
eval_he_context = None
server_c1 = None
server_c2 = None

# Back on the client side, which we assume is the secure side
# We now load the context we prepared for the client side,
# And also load the secret key for decryption.
secure_he_context=pyhelayers.DefaultContext()
secure_he_context.load_from_buffer(encryptBuf)
secure_he_context.load_secret_key(secBuf,True)

# Load encrypted result and decrypt it.
encRes = pyhelayers.CTile(secure_he_context)
encRes.load_from_buffer(encResBuf)
secure_encoder = pyhelayers.Encoder(secure_he_context)
res=secure_encoder.decrypt_decode_double(encRes)

# The HE computation skipped the square root.
# Under HE it's easier to compute the Euclidean distance
# squared, so we compute the root now after decryption.
he_euc=math.sqrt(res[0])
print("HE Euclidean distance is {:.9f}".format(he_euc))

# Verify that the relative difference is less than 0.1%
relative_diff = abs(he_euc - euc) / euc
assert relative_diff < 0.001
