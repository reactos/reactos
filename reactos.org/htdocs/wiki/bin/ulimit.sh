#!/bin/bash

ulimit -t $1
ulimit -v $2
shift 2
"$@"

