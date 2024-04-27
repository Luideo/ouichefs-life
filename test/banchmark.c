#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

int main(int  argc , char ** argv) {
	
	if(	argc< 2 ) exit(-1);
	
	char *path = "/mnt/ouiche/test.txt";
	int fd = open(path, O_CREAT);
	
	int fd2 = open(argv[1],O_RDONLY); 
	if(fd2 < 0 || fd < 0 ) exit(-1)
	
	//char buf[7] = "bonjou";
	//time_t *ptime_debut = (time_t *)malloc(sizeof(time_t));
	//time_t *ptime_fin = (time_t *)malloc(sizeof(time_t));
	clock_t begin = clock();
	
	//time(ptime_debut);
	//write(fd, buf, sizeof(buf));
	//lseek(fd, 0, SEEK_SET);
	
	char buf2[100];
	memset(buf2,0,sizeof(buf2));
	//read(fd, buf2, sizeof(buf2));
	int k ; 
	while( (k = read(fd2,buf2,sizeof(buf2))) > 0){
		write(fd , buf2 , k);
		memset(buf2,0,sizeof(buf2));
	}
	
	clock_t end = clock();
	//time(ptime_fin);
	
	printf("test %ld\n", (end-begin));
	//printf("test %f\n", difftime(*ptime_fin, *ptime_debut));
	
	return 0;
}
