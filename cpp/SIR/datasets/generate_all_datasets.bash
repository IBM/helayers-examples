#!/bin/bash

python generate_data.py 4 784 d4x1.csv
python generate_data.py 10 784 d10x1.csv
python generate_data.py 40 784 d40x1.csv

python generate_data.py 4 1568 d4x2.csv
python generate_data.py 10 1568 d10x2.csv
python generate_data.py 40 1568 d40x2.csv

python generate_data.py 4 6272 d4x8.csv
python generate_data.py 10 6272 d10x8.csv
python generate_data.py 40 6272 d40x8.csv

python generate_data.py 4 12544 d4x16.csv
python generate_data.py 10 12544 d10x16.csv
python generate_data.py 40 12544 d40x16.csv

python generate_data.py 4 50176 d4x64.csv
python generate_data.py 10 50176 d10x64.csv
python generate_data.py 40 50176 d40x64.csv

python generate_data.py 4 100352 d4x128.csv
python generate_data.py 10 100352 d10x128.csv
python generate_data.py 40 100352 d40x128.csv

python generate_data.py 4 200704 d4x256.csv
python generate_data.py 10 200704 d10x256.csv
python generate_data.py 40 200704 d40x256.csv

python generate_data.py 4 401408 d4x512.csv
python generate_data.py 10 401408 d10x512.csv
python generate_data.py 40 401408 d40x512.csv

python generate_data.py 4 802816 d4x1024.csv
python generate_data.py 10 802816 d10x1024.csv
python generate_data.py 40 802816 d40x1024.csv
