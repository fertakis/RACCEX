#include <stdio.h>
#include <stdlib.h>

int main()
{
	#pragma offload target (mic:0)	
	{
		printf("Hello world\n");
	}
	return 0;
}
