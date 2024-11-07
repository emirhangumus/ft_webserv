#!/bin/bash

for i in {1..2}
do
    # send `i` in body of the request
    # POST request
    curl -X GET -H "Content-Type: application/json" -d "$i" http://localhost:8082
    # GET request
    # curl -X GET http://localhost:8082
done