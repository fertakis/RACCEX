#!/bin/bash

source /various/common_tools/intel-icc-and-tools/bin/compilervars.sh intel64

make

export SINK_LD_LIBRARY_PATH=/various/common_tools/intel-icc-and-tools/composer_xe_2015.0.090/compiler/lib/mic/:/various/common_tools/intel-icc-and-tools/composer_xe_2015.0.090/mkl/lib/mic/

micnativeloadex bin/dgemm_with_timing
