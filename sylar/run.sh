#!/bin/bash
cd build/ && make && make && cd ../bin/ && ./test_config && cd ..
