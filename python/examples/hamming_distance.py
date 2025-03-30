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

# This short script demonstrates Hamming distance under HE for biometric authentication. 
# We consider the model of three entities: a certificate authority, an identity verifier and an end user.
# The certificate authority generates the keys. The end user receives the encryption key from the 
# certificate authority, encrypts its secret biometric fingerprint (e.g., using a Face ID API) 
# - essentially a binary hash, and send it to the identity verifier. At a later time, When the end user
# tries to authenticate, they scan their face again, and a new encrypted hash is sent to the identity 
# verifier, and the hamming distance between it and the user's original encrypted hash is computed.
# The result is sent to the certificate authority which decrypts it to determine whether the 
# authentication process was successful or not.

# We also consider an optional scenario where instead of sending the Hamming distance as-is to 
# the certificate authority, a threshold function is applied over it to get an encrypted indicator
# that signals whether the received hash is close to the original hash or not.
# The reason we might want to compute this threshold function on the identity verifier side 
# is that we want to keep the Hamming distances private (and not expose them to the certifying 
# authority). Another possible reason might be that we want the encrypted result to be used in 
# another computation (not on the certifying authority side).
# The downside, of course, is that this computation is heavy. In the standard use-case, the
# certificate authority can just compute this threshold function in plaintext (after decryption)
# and spare this heavy computation. 

# For this demo, the bit width of the used hashes are the following.
HASH_SIZE = 2048

# Whether to compute the threshold function over the encrypted Hamming distance or not
COMPUTE_THRESHOLD = True

# For this demo, we allow the two hashes of the same person to have the following maximal Hamming distance between them.
ALLOWED_THRESHOLD = 10

# Parameters for the homomorphic sign function (used when computing the threshold function)
GREP = 2
FREP = 1

# Number of iterations to run
ITERATIONS = 1

# Whether to run this demo using the GPU
RUN_WITH_GPU = False

try:
    import pyhelayers
except ImportError:
    import pyhelayerslite as pyhelayers
print(pyhelayers.__name__, pyhelayers.VERSION)

import numpy as np
import math
import time

# Helper functions
def print_results(actual_hamming_distance, decrypted_authentication_result):
    print("Actual Hamming distance is {}".format(actual_hamming_distance))
    if COMPUTE_THRESHOLD:
        he_hamming_distance_too_far = bool(round(decrypted_authentication_result[0]))
        print("HE \'Hamming distance is too far\' indicator is {}".format(he_hamming_distance_too_far))
        assert he_hamming_distance_too_far == (actual_hamming_distance > ALLOWED_THRESHOLD)
    else:
        he_hamming_distance = round(decrypted_authentication_result[0])
        print("HE Hamming distance is {}".format(he_hamming_distance))
        assert he_hamming_distance == actual_hamming_distance    

def homomorphic_threshold(authentication_result):
    # We add an extra 0.5 to the allowed threshold so that the result after the following
    # subtraction will be either strictly smaller than zero, or strictly bigger than zero.
    # The reason we want it this way, is that the homomorphic sign method returns 0 if
    # the encrypted value is smaller than zero, returns 1 if it's bigger than zero,
    # but returns 0.5 if it's equal to zero (this is because the sign function is 
    # approximated using a polynomial). Since we want binary results, we add this 0.5.

    authentication_result.add_scalar(-(ALLOWED_THRESHOLD + 0.5)) 

    fe = pyhelayers.FunctionEvaluator(identity_verifier_he_context)
    fe.sign_in_place(authentication_result, GREP, FREP, HASH_SIZE, True)

#
# Certificate authority side: Generate keys.
#

# We initialize the context.
# Since our computation is shallow, we can configure low depth
# And small number of slots.
print("Initializing context . . .")

# for this demo, we use SEAL backend (since 1.5.5)
he_context = pyhelayers.DefaultContext()

# Number of slots per ciphertext. We set it such that an entire
# hash could be placed inside a single ciphertext.
num_slots = HASH_SIZE * 2

# Allow 1 level of multiplications, as needed by this application.
multiplication_depth = 1

# Set the fractional precision to the lowest possible. Since we work
# over integral data in this application, and we only perform additions,
# rotations and a single multiplication, we don't need high fractional 
# precision.
fractional_part_precision = 40

# Set integer precision to 12, so that the encrypted domain could
# handle integers in range [0, HASH_SIZE] as needed by this application.
integer_part_precision = 1 + int(math.log2(HASH_SIZE))

# We set the security level to 128 bits.
security_level = 128

if COMPUTE_THRESHOLD:

    # If we want to compute the threshold function, we need deeper multiplication depth
    multiplication_depth = 3 * (GREP + FREP) + 2

    # We also need better precision
    fractional_part_precision = 48

    # We find the minimal number of slots we can use, given the other parameters, to optimize performance
    num_slots = he_context.get_min_feasible_num_slots(
        pyhelayers.HeConfigRequirement(-1, 
                                       multiplication_depth, 
                                       fractional_part_precision, 
                                       integer_part_precision, 
                                       security_level))

requirement = pyhelayers.HeConfigRequirement(num_slots, multiplication_depth, fractional_part_precision, integer_part_precision, security_level)

# This step inits the context and generates the keys.
# By default, multiple keys are generated allowing arbitrary computations.
# For our purposes we customize the generation for just the keys we want:
# We assume our binary vectors have 512 bits. Therefore to compute the Hamming distance 
# with minimal latency we need rotation keys that are powers of 2 up to (excluding) num_slots.
# Also, we exclude the conjugation key that is not needed for this application.
# By default, a relinearization key needed for multiplication is included.
public_functions = pyhelayers.PublicFunctions()
public_functions.rotate = pyhelayers.RotationSetType.CUSTOM_ROTATIONS
public_functions.set_rotation_steps([2**p for p in range(int(math.log2(requirement.num_slots)))])
public_functions.conjugate = pyhelayers.ConjugationSupport.FALSE
requirement.public_functions = public_functions
he_context.init(requirement)

if RUN_WITH_GPU:
    he_context.set_default_device(pyhelayers.DeviceType.DEVICE_GPU)

# Serialize the evaluation keys to be used on the identity verifier side.
identity_verifier_context_buffer = he_context.save_to_buffer()
print('Identity verifier context: {:.3f} MB'.format(len(identity_verifier_context_buffer)/1024/1024))

# For the end user side we just need the encryption key, so we clear all evaluation keys.
public_functions_no_eval_keys = pyhelayers.PublicFunctions()
public_functions_no_eval_keys.clear()
public_functions_no_eval_keys.encrypt = True
end_user_context_buffer = he_context.save_to_buffer(public_functions_no_eval_keys)
print('End user context: {:.3f} MB'.format(len(end_user_context_buffer)/1024/1024))

# Finally, for the certificate authority itself we just need the secret key, 
# so we clear all public keys as well. Furthermore, to reduce the size of the 
# saved data, we save only the seed of the secret key.
certificate_authority_context_buffer = he_context.save_to_buffer(public_functions_no_eval_keys)
secret_key_buffer = he_context.save_secret_key(seed_only = True)
print('Certificate authority context: {:.3f} MB'.format(len(certificate_authority_context_buffer + secret_key_buffer)/1024/1024))

for _ in range(ITERATIONS):
    #
    # End user side: Register to the system.
    #

    # We now start the registration process on the end user side. 
    # The user generates a secret hash, encrypts it and send it to the identity verifier.

    # Create our sample hash
    original_hash = np.random.randint(2, size = HASH_SIZE)

    # Encrypt the hash.
    # We load the context with the encryption key for this purpose.
    end_user_context = pyhelayers.load_he_context(end_user_context_buffer)
    end_user_encoder = pyhelayers.Encoder(end_user_context)
    encrypted_original_hash = end_user_encoder.encode_encrypt(original_hash)

    # Serialize the encrypted hash (this is sent to the identity verifier)
    encrypted_original_hash_buffer = encrypted_original_hash.save_to_buffer()


    #
    # End user side: Authenticate.
    #

    # At a later time, when the user wants to authenticate itself, they scan 
    # their face again and generate a new hash, which is expected to be close
    # in Hamming distance to the original hash. We simulate this by adding 
    # small random noise to the original hash
    max_noise_rate = ALLOWED_THRESHOLD / HASH_SIZE
    max_noise_rate = max_noise_rate * 0.9 # add redundant distance, so that the new hash will certainly be "valid"
    new_hash = original_hash ^ np.random.choice([0, 1], size = HASH_SIZE, p=[1-max_noise_rate, max_noise_rate])
    encrypted_new_hash = end_user_encoder.encode_encrypt(new_hash)
    encrypted_new_hash_buffer = encrypted_new_hash.save_to_buffer()


    #
    # Identity verifier side: Compute Hamming distance
    #

    # Load the context with the keys required for evaluation.
    identity_verifier_he_context = pyhelayers.load_he_context(identity_verifier_context_buffer)

    # Load the encrypted hashes
    loaded_original_hash = pyhelayers.load_ctile(identity_verifier_he_context, encrypted_original_hash_buffer)
    loaded_new_hash = pyhelayers.load_ctile(identity_verifier_he_context, encrypted_new_hash_buffer)
    authentication_result = loaded_new_hash

    # Compute the Hamming distance
    start = time.time()

    # This performs bitwise XOR between the original hash and the new hash
    authentication_result.sub(loaded_original_hash)
    authentication_result.square()

    # This sums the elements of the ciphertext
    authentication_result.inner_sum()

    if COMPUTE_THRESHOLD:
        # Apply the threshold function
        homomorphic_threshold(authentication_result)

    end = time.time()
    print("Duration of homomorphic Hamming distance evaluation: {:.6f} seconds".format(end - start))

    # Serialize the result
    authentication_result_buffer = authentication_result.save_to_buffer()


    #
    # Certificate authority side: Check result.
    #

    # Back on the certificate authority side, which we assume is the secure side.
    # We now load the context we prepared And also load the secret key for decryption.
    certificate_authority_he_context = pyhelayers.load_he_context(certificate_authority_context_buffer)
    certificate_authority_he_context.load_secret_key(secret_key_buffer, seed_only = True)

    # Load the encrypted authentication result and decrypt it.
    loaded_authentication_result = pyhelayers.load_ctile(certificate_authority_he_context, authentication_result_buffer)
    certificate_authority_encoder = pyhelayers.Encoder(certificate_authority_he_context)
    decrypted_authentication_result = certificate_authority_encoder.decrypt_decode_double(loaded_authentication_result)

    actual_hamming_distance = np.count_nonzero(original_hash ^ new_hash)
    print_results(actual_hamming_distance, decrypted_authentication_result)


# For completeness, we demonstrate the scenario of a malicious end user who
# tries to authenticate as the original user.


#
# Malicious user side:
#

# We simulate the malicious user hash by sampling a random hash, independently 
# from the original user hash.
malicious_hash = np.random.randint(2, size = HASH_SIZE)
encrypted_malicious_hash = end_user_encoder.encode_encrypt(malicious_hash)
encrypted_malicious_hash_buffer = encrypted_malicious_hash.save_to_buffer()


#
# Identity verifier side: Compute Hamming distance
#

loaded_malicious_hash = pyhelayers.load_ctile(identity_verifier_he_context, encrypted_malicious_hash_buffer)
authentication_result2 = loaded_malicious_hash
authentication_result2.sub(loaded_original_hash)
authentication_result2.square()
authentication_result2.inner_sum()
if COMPUTE_THRESHOLD:
        homomorphic_threshold(authentication_result2)
authentication_result2_buffer = authentication_result2.save_to_buffer()

#
# Certificate authority side: Check result.
#

loaded_authentication_result2 = pyhelayers.load_ctile(certificate_authority_he_context, authentication_result2_buffer)
decrypted_authentication_result2 = certificate_authority_encoder.decrypt_decode_double(loaded_authentication_result2)

actual_hamming_distance = np.count_nonzero(original_hash ^ malicious_hash)
print_results(actual_hamming_distance, decrypted_authentication_result2)
