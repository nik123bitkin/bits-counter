#!/bin/bash

truncate -s 0 log1
truncate -s 0 log

xxd -b $1 > log

grep -Eo "[[:digit:]]{8} " log > log1

grep -o 1 log1 | wc -l
