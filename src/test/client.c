#include <stdio.h>
#include <unistd.h>
#include "c1218_stack.h"

int main(int argc, char *argv[])
{
	int opt;
	int is_client = 0;	// default C12.18 Device
	c1218_stack_t stack;
	char devname[20] = { 0 };

	while ((opt = getopt(argc, argv, "t:d:")) != -1) {
		switch (opt) {
		case 'd':
			printf("Device: %s\n", optarg);
			strcpy(devname, optarg);
			break;
		case 't':
			if (!strcmp(optarg, "client") ||
			    !strcmp(optarg, "Client") ||
			    !strcmp(optarg, "CLIENT"))
				is_client = 1;
			else if (!strcmp(optarg, "device") ||
				 !strcmp(optarg, "Device") ||
				 !strcmp(optarg, "DEVICE"))
				is_client = 0;
			break;
		default:
			break;
		}
	}

	if (!c1218_stack_init(&stack, devname, is_client))
		c1218_stack_run(&stack);

	return 0;
}
