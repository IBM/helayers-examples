#!/bin/bash

#
# The open-source repository https://github.com/IBM/helayers-examples doesn't
# contain the data directory; instead, the data files are attach as a ZIP file
# to the github releases.
#
# This script downloads the ZIP file and extracts it so the examples can use
# the dataset files.
#

set -eux

examples_dir="$(dirname $0)"
cd $examples_dir
curl -f -L -O https://github.com/IBM/helayers-examples/releases/download/v0.0.1/helayers-examples-data.zip
unzip helayers-examples-data.zip
rm helayers-examples-data.zip
echo "HElayers examples data directory ready."
