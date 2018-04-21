#!/bin/bash

mode="racex"

if [ "$HOSTNAME" = dungani ]; then
	source /various/common_tools/intel-icc-and-tools/bin/compilervars.sh intel64
else	
	source /opt/intel/bin/compilervars.sh intel64
fi

export MIC_ENV_PREFIX=MIC
export MIC_USE_2MB_BUFFERS=32K
export MIC_KMP_AFFINITY=granularity=fine,balanced
export MIC_BUFFERSIZE=128M
export MIC_MKL_DYNAMIC=false

echo "Running Reduction 56";
export MIC_OMP_NUM_THREADS=56
~/shoc-mic-final/bin/Reduction -s 1 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_56_s1.log 
~/shoc-mic-final/bin/Reduction -s 2 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_56_s2.log
~/shoc-mic-final/bin/Reduction -s 3 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_56_s3.log
~/shoc-mic-final/bin/Reduction -s 4 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_56_s4.log
echo "Running Reduction 112";
export MIC_OMP_NUM_THREADS=112
~/shoc-mic-final/bin/Reduction -s 1 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_112_s1.log 
~/shoc-mic-final/bin/Reduction -s 2 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_112_s2.log
~/shoc-mic-final/bin/Reduction -s 3 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_112_s3.log
~/shoc-mic-final/bin/Reduction -s 4 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_112_s4.log
echo "Running Reduction 224";
export MIC_OMP_NUM_THREADS=224
~/shoc-mic-final/bin/Reduction -s 1 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_224_s1.log 
~/shoc-mic-final/bin/Reduction -s 2 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_224_s2.log
~/shoc-mic-final/bin/Reduction -s 3 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_224_s3.log
~/shoc-mic-final/bin/Reduction -s 4 &> ~/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_${mode}_224_s4.log
