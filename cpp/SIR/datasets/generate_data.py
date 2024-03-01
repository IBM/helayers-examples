import pandas as pd
import numpy as np
import sys

def generate_dataframe(dim, size, seed=None):
    np.random.seed(seed)
    data = np.random.uniform(-1, 1, (size, dim+1))
    data = np.round(data, 3)
    df = pd.DataFrame(data, columns=[f"F{i}" for i in range(1, dim+1)] + ['y'])
    return df

def main(dim, size, filename, seed=None):
    df = generate_dataframe(dim, size, seed)
    df.to_csv(filename, index=False)
    print(f"DataFrame saved to {filename}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python script.py <dim> <size> <filename> [seed]")
    else:
        try:
            dim = int(sys.argv[1])
            size = int(sys.argv[2])
            filename = sys.argv[3]
            seed = int(sys.argv[4]) if len(sys.argv) == 5 else 777
            main(dim, size, filename, seed)
        except ValueError:
            print("Error: Dimension, size, and seed (if provided) must be integers.")

