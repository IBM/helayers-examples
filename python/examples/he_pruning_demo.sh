#!/bin/bash
set -e

mkdir -p output

##########
# SET ME #
##########
network_cfg="256 64 256"
prune_fracs="0.9"

# Use this setting for a more thorough demo with various pruning fractions:
# prune_fracs="0.9 0.95 0.99"
##########

# Prepare the ONNX file
network_cfg_underscored=$(echo "$network_cfg" | sed 's/ /_/g')
./he_pruning_demo.py \
    --application Autoenc-Reconst \
    --dataset MNIST \
    --prune-fractions ${prune_fracs} \
    --lr 0.0025 \
    --max-epochs 10 \
    --num-neurons ${network_cfg} \
    --tile-shape 4x8 \
    --show-viz 2>&1 | tee output/run.log

# Demonstrate the effect of pruning in pyhelayers
for prune_frac in ${prune_fracs}; do
    echo "Running HeLayers for prune fraction = ${prune_frac}..."
    out_path=output/figures_${prune_frac}
    mkdir -p ${out_path}

    model_file_root="output/results/lr_0.0025/Autoenc-Reconst/MNIST/${network_cfg_underscored}/P4E-Gl-0.0625/Lc_L1_Wei/${prune_frac}/onnx_exports/4x8"
    model_file_unpruned="${model_file_root}/pr0rt0pe0ex0.onnx"
    model_file_pruned="${model_file_root}/pr2rt2pe1ex1.onnx"
    model_file_pruned_noPermMats="${model_file_root}/pr2rt2pe1ex1_noPermMats.onnx"

    ./he_pruning_helayers_mnist.py $model_file_unpruned $model_file_pruned $model_file_pruned_noPermMats ${out_path}
done
