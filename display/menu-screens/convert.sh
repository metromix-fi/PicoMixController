#!/bin/bash

for file in *.bmp; do
    # Check if the file is a regular file
    if [[ -f "$file" ]]; then
        # Convert the file to monochrome and overwrite it
        convert "$file" -monochrome "$file"
    fi
done