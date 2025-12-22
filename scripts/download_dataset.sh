#!/bin/bash

# Directory setup
kaggle datasets download ericanacletoribeiro/cicids2017-cleaned-and-preprocessed
DATA_DIR="../data/datasets/cic_ids2017"
mkdir -p "$DATA_DIR"

echo "Directory created at $DATA_DIR"
echo "Downloading CIC-IDS2017 (CSV Version)..."

# Note: This uses the Kaggle API. 
# If you don't have Kaggle API set up, you can manually download 
# "Machine Learning Logic Analysis - IDS 2017" from Kaggle and place it in data/datasets/

# Alternative: Direct download from a public S3 bucket or academic mirror if available.
# For this script, we will assume manual download or use the 'kaggle' CLI tool.

if command -v kaggle &> /dev/null
then
    kaggle datasets download -d cicdataset/cicids2017 -p "$DATA_DIR" --unzip
    echo "Download complete and unzipped!"
else
    echo "Kaggle CLI not found."
    echo "Please install it (pip install kaggle) or download manually from:"
    echo "https://www.kaggle.com/datasets/cicdataset/cicids2017"
fi