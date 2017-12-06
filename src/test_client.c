#include <stdio.h>
#include <string.h>
#include <scif.h>
#include "common.h"

int main() {
	scif_epd_t endPoint;
	
	printf("Going to test scif_open\n");
	endPoint = scif_open();
	printf("endpoint is %d\nexiting..\n", endPoint);
	scif_close(endPoint);

	return 0;
}
