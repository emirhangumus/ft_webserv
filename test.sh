# #!/bin/bash

# siege -t30S -b http://localhost:8082/cgi-bin/test.php

# # for i in {1..2}
# # do
#     # # send `i` in body of the request
#     # curl -X POST -H "Content-Type: application/json" -d "$i" http://localhost:8081
# # done

curl -X POST http://localhost:8080/cgi-bin/example.cgi \
     -H "Transfer-Encoding: chunked" \
     -H "Content-Type: text/plain" \
     --data-binary @- <<EOF
7
Hello, 
4
this
6
 is a 
4
test
0

EOF
