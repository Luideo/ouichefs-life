#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include "ouicheioctl.h"

int main(int argc, char **argv) {
	if(argc != 3 || argv[1][0] != '-' || (argv[1][1] != 'u' && argv[1][1] != 'p' && argv[1][1] != 'i' && argv[1][1] != 'l' && argv[1][1] != 'd')) {
		printf("Usage: %s <option> <file>\n", argv[0]);
		printf("Options:\n");
		printf("\t-u : used blocks\n");
		printf("\t-p : partially filled blocks\n");
		printf("\t-i : internal fragmentation\n");
		printf("\t-l : list of used blocks\n");
		printf("\t-d : defragment the file\n");
		return 1;
	}

	int fd = open(argv[2], O_RDWR);

	if(fd < 0) {
		perror("open");
		return 1;
	}

	// int32_t val;
	char val[64];

	switch(argv[1][1]) {
		case 'u':
			if(ioctl(fd, USED_BLKS, val) == 0)
				printf("Used blocks: %s\n", val);
			else
				perror("ioctl");
			break;
		case 'p':
			if(ioctl(fd, PART_FILLED_BLKS, val) == 0)
				printf("Partially filled blocks: %s\n", val);
			else
				perror("ioctl");
			break;
		case 'i':
			if(ioctl(fd, INTERN_FRAG_WASTE, val) == 0 )
				printf("Internal fragmentation: %s\n", val);
			else
				perror("ioctl");
			break;
		case 'l':
			if (ioctl(fd, USED_BLKS_INFO, NULL) != 0)
				perror("ioctl");
			break;
		case 'd':
			if(ioctl(fd, DEFRAG) != 0)
				perror("ioctl");
			break;
		default:
			printf("Invalid option\n");
			break;
	}

	close(fd);

	return 0;
}