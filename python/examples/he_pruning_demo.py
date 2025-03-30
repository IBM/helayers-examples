#!/usr/bin/env python3
import os
import sys
import argparse
import pyhelayers.mltoolbox.pruner.hepex as hepex

layer_pruning_policies = ["Gl/L1/Wei", "Lc/L1/Wei", "Lc/T-Min/-", "Lc/T-Max/-", "Lc/T-Avg/-", "Gl/T-Min/-", "Gl/T-Max/-", "Gl/T-Avg/-"]

# function to create directories if they don't exist
def create_directories(paths):
    for path in paths:
        os.makedirs(path, exist_ok=True)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--application', type=str, choices=("Autoenc-Denoise", "Autoenc-Reconst"), required=True,
                        help='Application to run.')
    parser.add_argument('--dataset', type=str, choices=("CIFAR10" "MNIST" "SVHN"), required=True,
                        help='Dataset to use with the application.')
    parser.add_argument('--num-neurons', type=int, nargs='+', required=True,
                        help='Number of neurons in each of the hidden layers of the network.')
    parser.add_argument('--mode', type=str, default="train-from-scratch", choices=("train-from-scratch", "use-unpruned-trained-models"),
                        help='Whether to train the model from scratch or use a pre-trained model.')
    parser.add_argument('--prune-fractions', type=float, nargs='+', required=True,
                        help='List of pruning fractions (0.0-1.0) to apply.')
    parser.add_argument('--policy', type=str, default="P4E-Gl-0.0625",
                        help="Pruning policy to use.")
    parser.add_argument('--sub-policy', type=str, default="Lc/L1/Wei", choices=layer_pruning_policies,
                        help="Pruning sub-policy to use.")
    parser.add_argument('--batch-size', type=int, default=128,
                        help="Batch size for training.")
    parser.add_argument('--lr', type=float, default=0.01,
                        help="Learning rate.")
    parser.add_argument('--max-epochs', type=int, default=10,
                        help="Maximum number of epochs.")
    parser.add_argument('--tile-shape', type=str, required=True,
                        help="Tile shape used for pruning and re-training.")
    parser.add_argument('--max-kmeans-iters', type=int, default=20,
                        help="Maximum number of k-Means iterations per permute iteration.")
    parser.add_argument('--max-permute-iters', type=int, default=5,
                        help="Maximum number of permutations iterations.")
    parser.add_argument('--models-path', type=str, default="./output/models",
                        help="Path to store models. Default is './output/models'.")
    parser.add_argument('--results-path', type=str, default="./output/results",
                        help="Path to store results. Default is './output/results'.")
    parser.add_argument('--save-models', action='store_true',
                        help="Save each pruned model.")
    parser.add_argument('--ncores', type=int, default=4,
                        help='Number of cores to parallelize k-Means with.')
    parser.add_argument('--show-viz', action='store_true', default=False,
                        help='visualize the output image of the autoencoder')
    parser.add_argument('--combine-cnn-permute-lay-with-first-fc-lay', action='store_true',
                        default=False, help='combine the permutation matmul in a CNN with the first'\
                                           ' FC layer so that we do not have to process the'\
                                           ' additional layer')
    # parser.add_argument('--skip-existing', action='store_true', default=False,
    #                     help='skip runs for which results file has already been generated')

    args = parser.parse_args()

    # Checks
    for prune_fraction in args.prune_fractions:
        assert 0.0 <= prune_fraction < 1.0, "Prune fractions must be in range [0.0, 1.0)"

    return args

if __name__ == "__main__":
    # parse and fetch cmd-line arg values
    args = parse_args()

    # create directories if they don't exist
    for path in [args.models_path, args.results_path]:
        os.makedirs(path, exist_ok=True)

    args.models_path = os.path.abspath(args.models_path)
    args.results_path = os.path.abspath(args.results_path)

    assert args.tile_shape.count('x') == 1, f"invalid argument for --tile-shape: {args.tile_shape}"
    tile_shape = tuple([int(x) for x in args.tile_shape.split('x')])

    # launch jobs for each policy, pruning, and fraction
    print(f"Policy:  {args.policy}")
    print(f"Pruning: {args.sub_policy}")
    layer_cfg = '_'.join([str(n) for n in args.num_neurons])
    sub_policy = args.sub_policy.replace('/', '_')

    results_path = f"{args.results_path}/lr_{args.lr}/{args.application}/{args.dataset}/{layer_cfg}/{args.policy}/{sub_policy}"

    # "P4E-Gl-0.0625"
    policy_split = args.policy.split('-')
    assert policy_split[0] == "P4E", "this script only supports the P4E scheme for now"
    assert len(policy_split) == 3, f"invalid argument for --policy: {args.policy}"
    policy_scope, policy_meta = policy_split[1], policy_split[2]
    steps = ["permute", f"prune_almost_zero_tiles-expand/{policy_scope}/{policy_meta}", "retrain"]
    if args.save_models:
        steps = ["prune", "retrain", "save_model"] + steps
    else:
        steps = ["prune", "retrain"] + steps
    if args.mode == "train-from-scratch":
        steps = ["train"] + steps

    print(f"Will execute the following steps: {steps}")

    for prune_fraction in args.prune_fractions:
        results_fname = f"{results_path}/{prune_fraction}/results_{args.tile_shape}.csv"
        # if the results file exists and it has at least 2 rows, skip the run
        # if args.skip_existing and
        if os.path.exists(results_fname) and os.stat(results_fname).st_size > 0:
            print(f"[WARN] Run already completed: {results_fname}, skipping...")
            continue

        act_models_path = f'{args.models_path}/lr_{args.lr}/{args.application}/{args.dataset}/{layer_cfg}'
        act_results_path = f'{results_path}/{prune_fraction}'

        create_directories([act_models_path, act_results_path])

        env = os.environ.copy()
        env["OMP_NUM_THREADS"] = str(args.ncores) # for k-Means parallelization

        pruner = hepex.Pruner(batch_size=args.batch_size,
                              val_perc=10.,
                              max_n_epochs=args.max_epochs,
                              max_kmeans_iters=args.max_kmeans_iters,
                              max_permute_iters=args.max_permute_iters,
                              lr=args.lr,
                              tile_shape=tile_shape,
                              prune_fracs=(prune_fraction, ),
                              layer_cfg=layer_cfg,
                              gamma=.7,
                              num_noisy_train_ex=5,
                              no_cuda=False,
                              log_interval=10,
                              conv_pruning="NA",
                              fc_pruning=args.sub_policy,
                              prune_steps=steps,
                              dataset=args.dataset,
                              app=args.application,
                              show_viz=args.show_viz,
                              results_path=act_results_path,
                              models_path=act_models_path,
                              dataset_root="./",
                              tile_shape_aware_train=False,
                              combine_cnn_permute_lay_with_first_fc_lay=\
                                args.combine_cnn_permute_lay_with_first_fc_lay)
        pruner.execute()
