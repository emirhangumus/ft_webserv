#!/bin/bash

# Path to the script you want to run
script_path="./test.sh"

# Spawn 10 processes
for i in {1..20}
do
    bash "$script_path" &
done

wait