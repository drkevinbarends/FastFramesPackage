#!/bin/bash

runs=(
    "tWZOFSR"
    "tWZSFSR"
    "tWZLooseSR"
    "ttZCR"
    "ZZbCR"
)

for param in "${runs[@]}"; do
    export region="$param"

    echo "Running with region: $region"

    # Create a temporary config file
    temp_config="FastFrames/temp_config.yml"
    envsubst < FastFrames/tWZ4Lep_config.yml > "$temp_config"

    # Run Python with the temporary config file
    python3 FastFrames/python/FastFrames.py -c "$temp_config" --step n

    echo "Completed run with PARAMETER: $region"
done