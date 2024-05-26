// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include "ouicheioctl.h"

int main(int argc, char **argv)
{
	if (argc != 3 ||
	    (argv[1][1] != 'i' || argv[1][1] != 'd' || argv[1][1] != 's' ||
	     argv[1][1] != 'w') &&
		    argv[1][0] != '-') {
		printf("Usage: %s <option> <file>\n", argv[0]);
		printf("Options:\n");
		printf("\t-i : display informations about the file\n");
		printf("\t-d : defragment the file\n");
		printf("\t-s : switch the read/write mode for the file system of the file\n");
		printf("\t-w : display current read/write mode for the file system of the file\n");
		return 1;
	}

	int fd = open(argv[2], O_RDWR);

	if (fd < 0) {
		perror("open");
		return 1;
	}

	// int32_t val;
	char val[64];

	switch (argv[1][1]) {
	case 'i':
		if (ioctl(fd, INFO) != 0)
			perror("ioctl");
		else
			printf("Informations displayed in dmesg.\n");
		break;
	case 'd':
		if (ioctl(fd, DEFRAG) != 0)
			perror("ioctl");
		break;
	case 's':
		if (ioctl(fd, SWITCH_MODE) != 0)
			perror("ioctl");
		break;
	case 'w':
		if (ioctl(fd, DISPLAY_MODE, val) == 0)
			printf("Current read/write mode: %s\n", val);
		else
			perror("ioctl");
		break;
	default:
		printf("Invalid option\n");
		break;
	}

	close(fd);

	return 0;
}