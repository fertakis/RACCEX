#include <scif.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	scif_epd_t endp;
	struct scif_portID *remote;
	int port;

	if((endp = scif_open()) < 0)
	{
		printf("scif_open failed\n");
		perror("scif_open");
		exit(1);
	}
	else 
		printf("scif_open() succeded with endp=%d\n", endp);

	remote = (struct scif_portID * )malloc(sizeof(struct scif_portID));
	remote->node = 0;
	remote->port = 1099;

	if((port = scif_connect(endp, remote)) < 0)
	{
		perror("scif_connect");
		exit(1);
	}
	else 
		printf("scif_connect() succeded with port=%d\n", port);
	exit(0);
}
