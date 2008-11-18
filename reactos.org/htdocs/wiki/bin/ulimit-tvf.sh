#!/bin/bash

ulimit -t $1 -v $2 -f $3
shift 3
"$@"

