#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define BEGIN 	0
#define END	1
#define MIDDLE	2
#define MAX_BUFF 256

/*
 * return : int : fd du fichier nouvellement créé (duplicata)
 * return : -1 si erreur
 */
void duplication_test(int src_fd, const char *dest_path, int testing) {
	lseek(src_fd, 0, SEEK_SET);
	int dest_fd = open(dest_path, O_CREAT | O_RDWR | O_TRUNC ,0600);

	if(dest_fd < 0){
		fprintf(stderr, "duplication_test: error during file creation\n");
		return ;
	}
	
	clock_t begin = clock();
	
	char buf2[250];
	memset(buf2,0,sizeof(buf2));
	int k ; 
	while( (k = read(src_fd,buf2,sizeof(buf2))) > 0){
		write(dest_fd, buf2, k);
		memset(buf2,0,sizeof(buf2));
	}
	
	clock_t end = clock();
	close(dest_fd);
	if(testing)
		printf("duplication test: %ld\n", (end-begin));
}

/*
 * INSERTION DANS LE FICHIER (sur la duplication)
 */
void insert_test(const char *dest_path, const char *text_test, int location ){
	char *test_type = malloc(7 * sizeof(char));
	clock_t begin, end;
	int dest_fd = open(dest_path, O_WRONLY);
	int src_fd = open(dest_path, O_RDONLY);
	struct stat st;
	fstat(dest_fd, &st);

	switch(location) {
		case BEGIN:
			strncpy(test_type, "begin", strlen(test_type));
			lseek(dest_fd, 0, SEEK_SET);
			lseek(src_fd, 0 , SEEK_SET);
			break;
		case END:
			strncpy(test_type, "end", strlen(test_type));
			lseek(dest_fd, 0, SEEK_END);
			lseek(src_fd, 0 , SEEK_END);
			break;
		case MIDDLE:
			strncpy(test_type, "middle", strlen(test_type));
			lseek(dest_fd, st.st_size/2, SEEK_SET);
			lseek(src_fd, st.st_size/2, SEEK_SET);
			break;
		default:
			fprintf(stderr, "unsupported location type\n");
			return;
	}

	char buf_src[MAX_BUFF];
	char buf_dest[MAX_BUFF];
	memset(buf_src,0,MAX_BUFF);
	memset(buf_dest,0,MAX_BUFF);

	int r;
	int i = strlen(text_test);

	memmove(buf_dest , text_test , i);

	begin = clock();

	while((r = read(src_fd , buf_src, MAX_BUFF) ) > 0){

		write(dest_fd,buf_dest,i);
		memset(buf_dest,0,MAX_BUFF);

		i = r ;

		memmove(buf_dest,buf_src,r);
		memset(buf_src,0,MAX_BUFF);

	}

	end = clock();


	printf("%s insert test %ld _\n", test_type, (end-begin) ) ;
	
	free(test_type);
	close(dest_fd);
	close(src_fd);
}

/*
 * SUPPRESSION DANS LE FICHIER (sur la duplication)
 */
void remove_test(const char *dest_path, int nb_char, int location){
	char *test_type = malloc(7 * sizeof(char));
	clock_t begin, end;
	int dest_fd = open(dest_path, O_WRONLY);
	int src_fd;
	struct stat st;
	if(location != END)
		src_fd = open(dest_path, O_RDONLY);

	switch(location) {
		case BEGIN:
			strncpy(test_type, "begin", strlen(test_type));
			lseek(dest_fd, 0, SEEK_SET);
			lseek(src_fd, nb_char, SEEK_SET);
			break;
		case END:
			strncpy(test_type, "end", strlen(test_type));
			fstat(dest_fd, &st);
			begin = clock();
			ftruncate(dest_fd, st.st_size-nb_char);
			end = clock();
			goto finish;
			break;
		case MIDDLE:
			strncpy(test_type, "middle", strlen(test_type));
			fstat(dest_fd, &st);
			lseek(dest_fd, st.st_size/2, SEEK_SET);
			lseek(src_fd, (st.st_size/2)+nb_char, SEEK_SET);
			break;
		default:
			fprintf(stderr, "unsupported location type\n");
			return;
	}

	char  buf_lect[MAX_BUFF];
	char  buf_write[MAX_BUFF];
	memset(buf_lect,0,MAX_BUFF);
	memset(buf_write,0,MAX_BUFF);
	int r;
	begin = clock();
	while((r = read(src_fd , buf_lect, MAX_BUFF)) > 0){
		memmove(buf_write,buf_lect,MAX_BUFF);
		//printf("%s \n=====\n %s \n", buf_lect,buf_write);
		write(dest_fd,buf_write,r);
		memset(buf_lect,0,MAX_BUFF);
		memset(buf_write,0,MAX_BUFF);
		
	}
	end = clock();

finish:
	if(location != END)
		close(src_fd);

	close(dest_fd);
	printf("%s remove test %ld\n", test_type, (end-begin));
	free(test_type);
} 


int main(int  argc , char ** argv) {
	
	if(argc != 2) {
		fprintf(stderr, "Usage: benchmark <path_to_source_file>\n");
		EXIT_FAILURE;
	}

	int src_fd = open(argv[1], O_RDONLY); 

	const char *text_test = "#_TEST STRING 123123_#";
	
	duplication_test(src_fd, "/mnt/ouiche/test1.txt", 1);
	//duplication_test(src_fd, "test1.txt", 1);
	insert_test( "/mnt/ouiche/test1.txt", text_test, BEGIN);
	
	duplication_test(src_fd, "/mnt/ouiche/test2.txt", 0);
	insert_test("/mnt/ouiche/test2.txt", text_test, END);

	duplication_test(src_fd, "/mnt/ouiche/test3.txt", 0);
	insert_test("/mnt/ouiche/test3.txt", text_test, MIDDLE);

	duplication_test(src_fd, "/mnt/ouiche/test4.txt", 0);
	remove_test("/mnt/ouiche/test4.txt", 10, BEGIN);

	close(src_fd);

	return 0;
}
