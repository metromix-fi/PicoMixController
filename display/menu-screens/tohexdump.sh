#!/bin/bash

for file in *bmp; do
    echo "Converting $file to hexdump..."
    python hexdump_to_c_array.py "$file"
done
```