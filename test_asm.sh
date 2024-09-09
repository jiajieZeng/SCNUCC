#!/bin/bash

# Check if the input filename is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <input_filename>"
  exit 1
fi

# Define the directory prefix and variables
input_file="$1"
output_asm="test.S"
output_exec="test"
output_log="./${input_file%.sy}.out"

# Generate the assembly code
./build/backend/gen_code "$input_file" > "$output_asm" 2>> "$output_log"

# Compile the assembly code to an executable and link with libsylib.a
riscv64-unknown-elf-gcc -g -o "$output_exec" "$output_asm" -L. -lsylib -no-pie 2>> "$output_log"

# Run the executable using QEMU and redirect output to .out file
qemu-riscv64 "$output_exec" >> "$output_log" 2>&1

# Print a newline and then the exit status
echo -e "\n$?" >> "$output_log"
