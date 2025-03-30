# Pruning Support in HElayers
This demo runs a set of MNIST images through an Autoencoder to generate a reconstructed image. This would be a part of, e.g., an image processing pipeline. Simply run the demo as `./he_pruning_demo.sh`.

The prints are logged into `output/run.log`. Once this finishes, `cd` into `output/results/lr_0.0025/Autoenc-Reconst/MNIST/128_32_128/P4E-Gl-0.0625/Lc_L1_Wei/0.3/`:
* Summarized statistics are available in `results_4x4.csv`
* Visualization of weight matrices, input/output images and zero tile histograms are in `weight_mats/`, `images/`, and `zero_tile_histograms/`, respectively.
* The exported ONNX model is: `onnx_exports/4x4/pr2rt2pe1ex1.onnx`.