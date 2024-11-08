#!/bin/bash

siege -t30S -b http://localhost:8082/cgi-bin/test.php

# for i in {1..2}
# do
    # # send `i` in body of the request
    # curl -X POST -H "Content-Type: application/json" -d "$i" http://localhost:8081
# done