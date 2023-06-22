#!/usr/bin/env python3

from contextlib import contextmanager
from psutil import virtual_memory, Process
from timeit import default_timer
import os
import h5py
import numpy as np
from sklearn import metrics
import time
import shutil
import sys

class Color:
    RED   = "\033[1;31m"
    RESET = "\033[0;0m"

def assess_results(true_labels, predicted_labels):
    # make sure true labels are in the shape of (samples,classes),
    # reshape to (samples,1) if 1D
    assert(len(true_labels.shape) <= 2)
    if len(true_labels.shape) == 1:
        true_labels = true_labels.reshape(true_labels.shape[0], 1)

    # make sure predicted labels has the exact same shape
    assert(predicted_labels.shape == true_labels.shape)

    print()
    if true_labels.shape[1] == 1:
        # a single score per sample, handle as binary
        true_labels = np.where(true_labels > 0.5, 1, 0).reshape(-1)

        f, t, thresholds = metrics.roc_curve(true_labels, predicted_labels)
        print(f"AUC Score: {metrics.auc(f,t):.3f}")

        predicted_labels = np.where(predicted_labels > 0.5, 1, 0).reshape(-1)
        print("Classification report:")
        print(metrics.classification_report(true_labels, predicted_labels))

    else:
        true_labels = true_labels.argmax(axis=1)
        predicted_labels = predicted_labels.argmax(axis=1)

    confusion_matrix = metrics.confusion_matrix(true_labels, predicted_labels)
    print("Confusion Matrix:")
    print(confusion_matrix)

    accuracy = float(np.trace(confusion_matrix)) / np.sum(confusion_matrix)
    return accuracy


def extract_batch(x_test, y_test, batch_size, batch_num):
    num_samples = x_test.shape[0]
    num_lebels = y_test.shape[0]

     # assert same size
    assert(num_samples == num_lebels)

    # calc start and end index
    start_index = batch_num * batch_size
    if start_index >= num_samples:
        raise RuntimeError('Not enough samples for batch number ' +
                           str(batch_num) + ' when batch size is ' + str(batch_size))
    end_index = min(start_index + batch_size, num_samples)

    batch_x = x_test.take(indices=range(start_index, end_index), axis=0)
    batch_y = y_test.take(indices=range(start_index, end_index), axis=0)

    return (batch_x, batch_y)

def extract_batch_from_files(x_filename, y_filename, batch_size, batch_num):
    """Extract batches from the files containing x and y samples."""
    with h5py.File(x_filename) as f:
        x_test = np.array(f['x_test'])
    with h5py.File(y_filename) as f:
        y_test = np.array(f['y_test'])

    plain_samples, labels = extract_batch(x_test, y_test, batch_size, batch_num)
    return plain_samples, labels

def get_data_sets_dir(path_to_utils=None):
    path_from_utils = '../../data'
    relative_path = path_from_utils if path_to_utils is None else os.path.join(path_to_utils, path_from_utils)
    return os.getenv('HELAYERS_DATA_SETS_DIR', relative_path)

def get_temp_output_data_dir():
    return './data'

def create_clean_dir(path):
    if os.path.exists(path):
        shutil.rmtree(path)
    os.mkdir(path)

start_time = None

def start_timer():
    global start_time
    start_time = time.perf_counter()

def report_duration(op_name,duration):
    print("Duration of " + op_name + ":", "{:.3f}".format(duration), "(s)")

def end_timer(op_name, silent=False):
    global start_time
    if start_time is None:
        raise RuntimeError('Timer was not started')

    duration = time.perf_counter() - start_time
    start_time = None

    if not silent:
        report_duration(op_name,duration)

    return duration

@contextmanager
def elapsed_timer(op_name, batch_size):
    """Context manager for timing the execution of an operation."""
    start_time = default_timer()

    class _Timer:
        start = start_time
        end = default_timer()
        duration = end - start

    yield _Timer

    end_time = default_timer()
    _Timer.end = end_time
    _Timer.duration = end_time - start_time
    print('Duration of {}: {} (s)'.format(op_name, _Timer.duration))

    if batch_size > 1:
        print('Duration of {} per sample: {:.3f} (s)'.format(op_name, _Timer.duration/batch_size))


def save_data_set(x, y, data_type, path, s=''):
    if not os.path.exists(path):
        os.makedirs(path)
    fname=os.path.join(path, f'x_{data_type}{s}.h5')
    print("Saving x_{} of shape {} in {}".format(data_type, x.shape, fname))
    xf = h5py.File(fname, 'w')
    xf.create_dataset('x_{}'.format(data_type), data=x)
    xf.close()

    print("Saving y_{} of shape {} in {}".format(data_type, y.shape, fname))
    yf = h5py.File(os.path.join(path, f'y_{data_type}{s}.h5'), 'w')
    yf.create_dataset(f'y_{data_type}', data=y)
    yf.close()


def serialize_model(model, path, s=''):
    model_json = model.to_json()
    with open(os.path.join(path, 'model.json'), "w") as json_file:
        json_file.write(model_json)
    # serialize weights to HDF5
    model.save_weights(os.path.join(path, 'model.h5'))
    print("Saved model to disk")

def verify_memory(min_memory_size=8, tolerance=0.5):
    mem = virtual_memory()
    total_mem = mem.total/(1024.**3)
    if total_mem <= (min_memory_size-tolerance):
        sys.stdout.write(Color.RED)
        print("Warning: total available memory is",total_mem,"GB", "the minimum required memory is", min_memory_size, "GB")
        sys.stdout.write(Color.RESET)

def get_memory():
    mem = virtual_memory()
    mem_MB = mem.total/(1024.**2)
    return mem_MB

def get_used_ram():
    mem_MB = Process().memory_info().rss / (1024 * 1024)
    return mem_MB
