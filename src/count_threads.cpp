using namespace std;

#pragma offload_attribute(push,target(mic)) //{

#include <iostream>
#include <omp.h>

	void
testThreadCount()
{
	int thread_count;

	#pragma omp parallel
	{
		#pragma omp single
		thread_count = omp_get_num_threads();
	}
	cout << "Thread count: " << thread_count << endl;

#ifdef __MIC__
	cout << "running on target" << endl;
#else 
	cout << "running on host" << endl;
#endif

}

#pragma offload_attribute(pop) //}

	int
main (int argc, char **argv)
{
	#pragma offload target(mic)
	testThreadCount();
}

