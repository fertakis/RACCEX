#include <stdio.h>
#include <string.h>
#include "common.h"
#include "libscifapiwrapper.c"

int main() {
	scif_epd_t endPoint;
	printf("Going to test scif_open\n");
	endPoint = scif_open();
	printf("endpoint is %d\nexiting..\n", endPoint);
	if((scif_close(endPoint) == 0))
		printf("test completed succesfully... \nCongrats!!\n");

	return 0;
}
