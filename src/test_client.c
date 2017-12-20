#include <stdio.h>
#include <string.h>
#include <scif.h>
#include "common.h"

int scif_get_driver_version(void);

int main() {
	scif_epd_t endPoint;
	printf("get_versioon=%d\n",scif_get_driver_version());	
	printf("Going to test scif_open\n");
	endPoint = scif_open();
	printf("endpoint is %d\nexiting..\n", endPoint);
	if((scif_close(endPoint) == 0))
		printf("test completed succesfully... \nCongrats!!\n");

	return 0;
}
