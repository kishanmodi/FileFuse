#!/bin/bash

# Check if number of arguments is correct
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <folder_path> <max_byte_size>"
    exit 1
fi

if [ ! -d "$1" ]; then
    mkdir -p "$1"
fi

# Assign arguments to variables
folder_path="$1"
max_byte_size="$2"

# Check if folder exists
if [ ! -d "$folder_path" ]; then
    echo "Folder doesn't exist."
    exit 1
fi

# Check if max_byte_size is numeric
if ! [[ "$max_byte_size" =~ ^[0-9]+$ ]]; then
    echo "Max byte size must be a positive integer."
    exit 1
fi

# Loop through creating files of increasing size
for ((i = 1; i <= max_byte_size; i++)); do
    dd if=/dev/zero of="$folder_path/file_$i" bs=1 count="$i" &> /dev/null
done

echo "Files created successfully."
