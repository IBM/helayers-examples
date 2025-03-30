#!/usr/bin/env python3
import pyhelayers
import numpy as np
import onnx
from onnx import helper, numpy_helper
import time
import sys
import torch
from torchvision import datasets, transforms
import matplotlib.pyplot as plt

torch.manual_seed(42)
num_iter = 5

def plot(images, figname):
    num_images = min(10, images.shape[0])  # Ensure we don't exceed available images
    plt.figure(figsize=(10, 2))

    for i in range(num_images):
        plt.subplot(1, 10, i + 1)
        plt.imshow(images[i], cmap='gray')
        plt.axis('off')

    plt.savefig(figname)
    print(f"Saved to: {figname}")

def mse_loss(y_predicted, y_true):
    squared_difference = np.square(y_true - y_predicted)
    mse = np.mean(squared_difference)
    return mse

def extract_matmul_layers(input_onnx):
    model = onnx.load(input_onnx)
    assert input_onnx.endswith(".onnx")
    graph = model.graph
    matmul_nodes = [node for node in graph.node if node.op_type == "MatMul"]
    if len(matmul_nodes) < 2:
        raise ValueError("Less than two MatMul layers found")

    extracted_weights = []
    for node in matmul_nodes:
        for input_name in node.input:
            constant_node = next((n for n in graph.node if n.op_type == "Constant" and n.output[0] == input_name), None)
            if constant_node:
                tensor = next(attr.t for attr in constant_node.attribute if attr.name == "value")
                extracted_weights.append(numpy_helper.to_array(tensor))

    assert len(extracted_weights) == 2, f"{extracted_weights}"
    inputPermMat, outputPermMat = extracted_weights[0], extracted_weights[1]
    return inputPermMat, outputPermMat

# Model file created by he_pruning_demo.py
if not len(sys.argv) == 5:
    print("Usage ./he_pruning_helayers.py <unpruned_model_file_path> <pruned_model_file_path> <pruned_model_noPermMats_file_path> <output_figures_path>")
    exit(1)

model_file_unpruned = sys.argv[1]
model_file_pruned = sys.argv[2]
model_file_pruned_noPermMats = sys.argv[3]
fig_root = sys.argv[4]

# Load MNIST dataset
mnist = datasets.MNIST(root="./data", train=False, download=True, transform=transforms.ToTensor())
# We run 512 samples in a single batch
batch_size = 512
# Select `batch_size` random images and flatten them
indices = torch.randperm(len(mnist))[:batch_size]
plain_samples = torch.stack([mnist[i][0].view(-1) for i in indices]).numpy()
plot(plain_samples.reshape(-1, 28, 28), f"{fig_root}/input.pdf")

# Trying first in using plain (unencrypted) model
nnp = pyhelayers.NeuralNetPlain()
nnp.init_from_files(pyhelayers.PlainModelHyperParams(), [model_file_unpruned])
print('Running predict on plain model . . . ')
plain_res=nnp.predict([plain_samples])
plain_res=plain_res[0]
plot(plain_res.reshape(-1, 28, 28), f"{fig_root}/output_plain.pdf")
del nnp

# Configure HE run:
# This demo uses a SEAL context 
# Specify batch size to optimize for
# Specify model weights are not encrypted
he_run_req = pyhelayers.HeRunRequirements()
he_run_req.set_he_context_options([pyhelayers.HeContext.create([ "SEAL_CKKS"])])
he_run_req.set_model_encrypted(False)
# We also specify a specific tile shape.
# Usually we let the optimizer choose it automatically,
# But here we already ran the optimizer, and relied on this choice in the pruning step.
# So here we set it to the pre-chosen shape just to be sure.
he_run_req.set_fixed_tile_layout(pyhelayers.TTShape([4, 8, batch_size]))
# Uncomment this line if you remove the set_fixed_tile_layout instruction above
#he_run_req.optimize_for_batch_size(batch_size)

# ------ unpruned HE model -------
print("----------------------------")
print("Processing UNPRUNED HE model")
print("----------------------------")
print('Initializing model and keys . . .')
nn = pyhelayers.NeuralNet()
nn.encode([model_file_unpruned], he_run_req)
he_context = nn.get_created_he_context()

print('Encrypting samples for test . . . ')
model_io_encoder = pyhelayers.ModelIoEncoder(nn)
encrypted_samples = pyhelayers.EncryptedData(he_context)
model_io_encoder.encode_encrypt(encrypted_samples, [plain_samples])

print('Starting inference under encryption . . .')
start_time = time.time()
for i in range(num_iter):
    enc_predictions = pyhelayers.EncryptedData(he_context)
    nn.predict(enc_predictions, encrypted_samples)
end_time = time.time()
print('Elapsed time:', (end_time - start_time) / num_iter, 'seconds')
decrypted_res = model_io_encoder.decrypt_decode_output(enc_predictions)

print(f'MSE loss compared with plain samples= {mse_loss(decrypted_res, plain_samples)}')
print(f'MSE loss compared with plain NN results= {mse_loss(decrypted_res, plain_res)}')


plot(decrypted_res.reshape(-1, 28, 28), f"{fig_root}/output_decrypted.pdf")
del model_io_encoder
del nn
# ------ END unpruned HE model -------

# ------ pruned HE model -------
print("--------------------------")
print("Processing PRUNED HE model")
print("--------------------------")
inputPermMat, outputPermMat = extract_matmul_layers(model_file_pruned)
print('Initializing model and keys . . .')
nn = pyhelayers.NeuralNet()
nn.encode([model_file_pruned_noPermMats], he_run_req)
he_context = nn.get_created_he_context()

print('Encrypting samples for test . . . ')
model_io_encoder = pyhelayers.ModelIoEncoder(nn)
encrypted_samples = pyhelayers.EncryptedData(he_context)
model_io_encoder.encode_encrypt(encrypted_samples, [plain_samples @ inputPermMat])

pyhelayers.helayers_internal_flags().set_bool("pruneZeroPTiles", True)
print('Turned on pruning:')
print(nn)
print('Starting inference under encryption . . .')
start_time = time.time()
for i in range(num_iter):
    enc_predictions = pyhelayers.EncryptedData(he_context)
    nn.predict(enc_predictions, encrypted_samples)
end_time = time.time()
print('Elapsed time:', (end_time - start_time) / num_iter, 'seconds')
decrypted_res = model_io_encoder.decrypt_decode_output(enc_predictions) @ outputPermMat
print(f'MSE loss compared with plain samples= {mse_loss(decrypted_res, plain_samples)}')
print(f'MSE loss compared with plain NN results= {mse_loss(decrypted_res, plain_res)}')

plot(decrypted_res.reshape(-1, 28, 28), f"{fig_root}/output_decrypted_pruned.pdf")
del model_io_encoder
del nn
# ------ END pruned HE model -------
