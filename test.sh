#!/bin/bash

for i in {1..2}
do
    # send `i` in body of the request
    curl -X POST -H "Content-Type: application/json" -d "$i" http://localhost:8081
done