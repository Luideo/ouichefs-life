
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
#define NBCHAR 10
#define WRI "TEST WRITE"
#define SIZEWRI 11

/*
* Duplique le fichier src_path vers dest_path
*/

void duplication_test(const char * src_path, const char *dest_path, int testing) {

	int src_fd = open(src_path, O_RDWR );
	int dest_fd = open(dest_path, O_CREAT | O_RDWR | O_TRUNC ,0600);

	if(dest_fd < 0 ||  src_fd < 0){
		fprintf(stderr, "duplication_test: error during file creation\n");
		return ;
	}
	
	clock_t begin = clock();
	
    int totalread = 0;
    int totalwritten = 0;
	char buf2[MAX_BUFF];
	memset(buf2,0,MAX_BUFF);
	int k ; 
	while( (k = read(src_fd,buf2,MAX_BUFF)) > 0){
        totalread+=k;
		totalwritten += write(dest_fd, buf2, k);
		memset(buf2,0,MAX_BUFF);
	}
	
	clock_t end = clock();
	close(dest_fd);
	if(testing)
		printf("duplication test: temps:%ld | lecture: %d | Ecriture: %d\n", (end-begin), totalread, totalwritten);
}

/*
lit nb_char caractère depuis le début du fichier
*/

int read_start(int src_fd , int nb_char){
    lseek(src_fd, 0, SEEK_SET);

    char buff[nb_char+1];
    memset(buff,0,nb_char+1);

    int ret = read(src_fd,buff,nb_char);
    char * tmp = buff;
    printf("%s\n",tmp);

    return ret;
}

/*
Lit entièrement le fichier 
*/
int read_all(int src_fd){
    lseek(src_fd,0,SEEK_SET);

    int total= 0;
    char buff[MAX_BUFF];
    memset(buff,0,MAX_BUFF);
    int r = 0;

    while((r = read(src_fd,buff,MAX_BUFF-1)) > 0  ){
        total+=r;
        printf("%s",buff);
        memset(buff,0,MAX_BUFF);
    }

    return total;
}

int read_mid(int src_fd , int nb_char , int pos){
    lseek(src_fd,pos,SEEK_SET);

    char buff[nb_char+1];
    memset(buff,0,nb_char+1);

    int ret = read(src_fd,buff,nb_char);
    printf("%s\n",buff);

    return ret;
}

int read_empty(){
    int src_fd = open("/mnt/ouiche/readempty.txt" , O_TRUNC | O_CREAT |O_RDWR, 0666);
    if(src_fd < 0){
        return -1 ;
    }
    
    char buff[MAX_BUFF];
    memset(buff , 0 , MAX_BUFF);

    int ret = 0;
    ret = read(src_fd,buff,MAX_BUFF);
    printf("%s\n",buff);

    close(src_fd);
    return ret ;
}

void read_test(const char * src){
    int src_fd = open(src,O_RDONLY);
    if(src_fd == -1 ){
        fprintf(stdout, "Fichier non_existant\n");
		EXIT_FAILURE;
    }
    struct stat st;
	fstat(src_fd, &st);

    int be = read_start(src_fd , NBCHAR);
    int mid = read_mid(src_fd, NBCHAR , st.st_size/2);
    int emp = read_empty();
    int all = read_all(src_fd);

    printf("\nRead au Debut: lecture: %d | attendu: %d \n",be,(int)( (NBCHAR>st.st_size)? st.st_size:NBCHAR ) );
    printf("Read au Milieu: lecture: %d | attendu: %d \n",mid,(int)( (NBCHAR>st.st_size/2)? st.st_size/2:NBCHAR ) );
    printf("Read vide: lecture: %d | attendu: %d \n",emp, 0 );
    printf("Read entier: lecture: %d | attendu: %d \n",all, (int)st.st_size );

    close(src_fd);
}

int write_append(const char * src){
    int written = 0;
    int src_fd = open(src, O_CREAT | O_WRONLY | O_APPEND, 0666);

    if(src_fd < 0){
        fprintf(stdout, "Fichier non_existant\n");
        return -1;
    }
    struct stat st;
    fstat(src_fd, &st);
    int taille_avant= st.st_size; 

    written = write(src_fd,WRI, SIZEWRI);
    
	fstat(src_fd, &st);

    close(src_fd);

    printf("Write append:  ecriture: %d | attendu: %d \n" , written , SIZEWRI);
    printf("Write append:  Taille fichier: %d | attendu: %d \n" , (int)st.st_size , taille_avant + SIZEWRI);
    return written;
}

int write_new(const char * src){
    int written = 0;
    int src_fd = open(src, O_CREAT | O_WRONLY | O_TRUNC, 0666);

    if(src_fd < 0){
        fprintf(stdout, "Fichier non_existant\n");
        return -1;
    }
    struct stat st;
    fstat(src_fd, &st);
    int taille_avant= st.st_size; 

    written = write(src_fd,WRI, SIZEWRI);

    fstat(src_fd, &st);

    close(src_fd);
    printf("Write new:  ecriture: %d | attendu: %d \n" , written , SIZEWRI);
    printf("Write new:  Taille fichier: %d | attendu: %d \n" , (int)st.st_size , taille_avant + SIZEWRI);
    return written;
}

int write_start(const char * src){
    int written = 0;
    int src_fd = open(src, O_CREAT | O_WRONLY , 0666);

    if(src_fd < 0){
        fprintf(stdout, "Fichier non_existant\n");
        return -1;
    }

    struct stat st;
    fstat(src_fd, &st);
    int taille_avant= st.st_size; 

    written = write(src_fd,WRI, SIZEWRI);

    fstat(src_fd, &st);

    close(src_fd);
    printf("Write start:  ecriture: %d | attendu: %d \n" , written , SIZEWRI);
    printf("Write start:  Taille fichier: %d | attendu: %d \n" , (int)st.st_size , taille_avant);
    return written;
}

int write_mid(const char * src){
    int written = 0;    
    int src_fd = open(src, O_CREAT | O_WRONLY , 0666);

    if(src_fd < 0){
        fprintf(stdout, "Fichier non_existant\n");
        return -1;
    }

    struct stat st;
	fstat(src_fd, &st);
    int taille_avant= st.st_size;
    lseek(src_fd,st.st_size/2,SEEK_SET);

    written = write(src_fd,WRI, SIZEWRI);

    fstat(src_fd, &st);

    close(src_fd);
    printf("Write mid:  ecriture: %d | attendu: %d \n" , written , SIZEWRI);
    printf("Write mid:  Taille fichier: %d | attendu: %d \n" , (int)st.st_size , taille_avant);
    return written;
}

int toParam(const char * param){
    if(strcmp("-d" , param) == 0){
        return 1;
    }
    if(strcmp("-r" , param) == 0){
        return 2;
    }
    if(strcmp("-w" , param) == 0){
        return 3;
    }

    if(strcmp("-ws" , param) == 0){
        return 4;
    }

    if(strcmp("-wm" , param) == 0){
        return 5;
    }

    if(strcmp("-wa" , param) == 0){
        return 6;
    }

    return 0;
}

int main(int  argc , char ** argv) {
	
	if(argc < 2) {
		fprintf(stdout, "Usage: benchmark <option> <path_to_source_file>\n");
        fprintf(stdout, "-d : duplication\n-r :read\n-w :write append\n-ws :write at the start\n-wm :write in the middle\n-wa :write in a new\n");
        fprintf(stdout, "attention pour les writes les fichiers rsique d'être détruits\n");
		EXIT_FAILURE;
	}

    switch (toParam(argv[1]))
    {
    case 1 :
        /*
        * Test de duplication : Read+Write
        */
        if(argc < 3){
            fprintf(stdout, "Usage: benchmark -d <path_to_source_file>\n");
            EXIT_FAILURE;
            break;
        }
        duplication_test(argv[2] , "/mnt/ouiche/duplicationtest.txt",1);
        break;
    case 2 :
        /*
        * Test de Read
        */
        if(argc < 3 ){
            fprintf(stdout, "Usage: benchmark -r <path_to_source_file>\n");
            EXIT_FAILURE;
            break;
        }
        read_test(argv[2]);
        break;

    case 3 :
        /*
        * test write : append un fichier
        */
        if(argc == 3){
            write_append(argv[2]);
        }else 
            write_append("/mnt/ouiche/appendwritetest.txt");
        break;
    
    case 4 :
        /*
        * test write : debut d'un fichier
        */
        if(argc == 3){
            write_start(argv[2]);
        }else 
            write_start("/mnt/ouiche/startwritetest.txt");
        break;

    case 5 :
        /*
        * test write : milieu d'un fichier
        */
        if(argc == 3){
            write_mid(argv[2]);
        }else 
            write_mid("/mnt/ouiche/midwritetest.txt");
        break;
    
    case 6 :
        /*
        * test write : ecriture dans un fichier vierge
        */
        if(argc == 3){
            write_new(argv[2]);
        }else 
            write_new("/mnt/ouiche/newwritetest.txt");
        break;

    default:
        fprintf(stdout, "Usage: benchmark <option> <path_to_source_file>\n");
        fprintf(stdout, "-d : duplication\n-r :read\n-w :write append\n-ws :write at the start\n-wm :write in the middle\n-wa :write in a new\n");
        EXIT_FAILURE;
        break;
    } 


	return 0;
}
