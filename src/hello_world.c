#include <stdio.h>
#include <stdlib.h>

int main()
{
	#pragma offload target (mic)	
	{
		printf("Hello world\n");
	}
	return 0;
}
