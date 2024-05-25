// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#define BEGIN 0
#define END 1
#define MIDDLE 2
#define MAX_BUFF 256
#define NBCHAR 10
#define WRI "TEST WRITE"
#define SIZEWRI 11
#define SIZEOW 11
#define OFFSET1 4096
#define OFFSET2 1024
#define READTEXT \
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec finibus neque eu sem maximus euismod. Class aptent taciti sociosqu ad litora torquent per conubia nostra"
#define CLOCKS_PER_MQ (CLOCKS_PER_SEC / 1000000)
/*
 * affiche un buffer complet : ainsi que tous les caractères non imprimable
 */
void print_comp(const char *src, size_t size)
{
	char buuf[size * 2 + 1];

	memset(buuf, 0, size * 2 + 1);
	int count = 0;

	for (int i = 0; i < size; i++) {
		if (isprint(src[i])) {
			buuf[count] = src[i];
			count++;
		} else {
			switch (src[i]) {
			case '\n':
				buuf[count] = '/';
				buuf[count + 1] = 'n';
				count += 2;
				break;
			case '\0':
				buuf[count] = '/';
				buuf[count + 1] = '0';
				count += 2;
				break;
			default:
				buuf[count] = '?';
				count++;
				break;
			}
		}
	}
	printf("%s\n", buuf);
}

/*
 * Duplique le fichier src_path vers dest_path
 */
void duplication_test(const char *src_path, const char *dest_path, int testing)
{
	int src_fd = open(src_path, O_RDWR);
	int dest_fd = open(dest_path, O_CREAT | O_RDWR | O_TRUNC, 0600);

	if (dest_fd < 0 || src_fd < 0) {
		fprintf(stderr, "%s: error during file creation\n", __func__);
		return;
	}

	printf("===== TEST Duplication\n");
	int totalread = 0;
	int totalwritten = 0;
	char buf2[MAX_BUFF];

	memset(buf2, 0, MAX_BUFF);
	int k;
	clock_t begin = clock();

	while ((k = read(src_fd, buf2, MAX_BUFF)) > 0) {
		//printf("loop buf: %s\n",buf2);
		print_comp(buf2, k);
		totalread += k;
		totalwritten += write(dest_fd, buf2, k);
		memset(buf2, 0, MAX_BUFF);
	}
	printf("\n");
	clock_t end = clock();

	close(dest_fd);
	if (testing)
		printf("duplication test: temps: %ld micro_sec | lecture: %d octets | Ecriture: %d octets\n",
		       (end - begin) / CLOCKS_PER_MQ, totalread, totalwritten);
}

/*
 * lit nb_char caractère depuis le début du fichier
 */
int read_start(int src_fd, int nb_char, clock_t *time)
{
	lseek(src_fd, 0, SEEK_SET);
	char buff[nb_char + 1];

	memset(buff, 0, nb_char + 1);

	clock_t begin = clock();
	int ret = read(src_fd, buff, nb_char);
	clock_t end = clock();

	*time = (end - begin);

	char *tmp = buff;

	printf("===== TEST READ START\n");
	print_comp(buff, ret);
	printf("\n");

	return ret;
}

/*
 * Lit entièrement le fichier
 */
int read_all(int src_fd, clock_t *time)
{
	lseek(src_fd, 0, SEEK_SET);
	int total = 0;
	char buff[MAX_BUFF];

	memset(buff, 0, MAX_BUFF);
	int r = 0;

	printf("===== TEST READ ENTIER\n");

	clock_t begin = clock();

	while ((r = read(src_fd, buff, MAX_BUFF - 1)) > 0) {
		total += r;
		print_comp(buff, r);
		memset(buff, 0, MAX_BUFF);
	}
	printf("\n");
	clock_t end = clock();
	*time = (end - begin);
	return total;
}

int read_mid(int src_fd, int nb_char, int pos, clock_t *time)
{
	lseek(src_fd, pos, SEEK_SET);

	char buff[nb_char + 1];

	memset(buff, 0, nb_char + 1);

	clock_t begin = clock();
	int ret = read(src_fd, buff, nb_char);
	clock_t end = clock();

	printf("===== TEST READ MID-FILE\n");
	print_comp(buff, ret);
	printf("\n");

	*time = end - begin;

	return ret;
}

int read_empty(const char *src, clock_t *time)
{
	int src_fd = open(src, O_TRUNC | O_CREAT | O_RDWR, 0666);

	if (src_fd < 0)
		return -1;

	char buff[MAX_BUFF];

	memset(buff, 0, MAX_BUFF);
	clock_t begin = clock();
	int ret = 0;

	ret = read(src_fd, buff, MAX_BUFF);
	clock_t end = clock();

	printf("===== TEST READ EMPTY\n");
	print_comp(buff, ret);
	printf("\n");

	*time = end - begin;
	close(src_fd);
	return ret;
}

void read_test(const char *src, const char *rep)
{
	int src_fd = open(src, O_RDONLY);

	if (src_fd == -1) {
		fprintf(stdout, "Fichier non_existant\n");
		return;
	}
	struct stat st;

	fstat(src_fd, &st);

	clock_t bec = 0;
	clock_t mic = 0;
	clock_t empc = 0;
	clock_t allc = 0;
	int be = read_start(src_fd, NBCHAR, &bec);
	int mid = read_mid(src_fd, NBCHAR, st.st_size / 2, &mic);
	size_t sz = strlen(rep);
	char path[MAX_BUFF];

	memcpy(path, rep, sz);
	path[sz] = '/';
	memcpy(path + sz, "readempty.txt", 14);
	int emp = read_empty(path, &empc);
	int all = read_all(src_fd, &allc);

	printf("\nRead au Debut: lecture: %d octets | attendu: %d octets | time %ld micro_sec\n",
	       be, (int)((st.st_size < NBCHAR) ? st.st_size : NBCHAR),
	       bec / CLOCKS_PER_MQ);
	printf("Read au Milieu: lecture: %d octets | attendu: %d octets | time %ld micro_sec\n",
	       mid, (int)((st.st_size / 2 < NBCHAR) ? st.st_size / 2 : NBCHAR),
	       mic / CLOCKS_PER_MQ);
	if (emp >= 0) {
		printf("Read vide: lecture: %d octets | attendu: %d octets | time %ld micro_sec\n",
		       emp, 0, empc / CLOCKS_PER_MQ);
	}
	printf("Read entier: lecture: %d octets | attendu: %d octets | time %ld micro_sec\n",
	       all, (int)st.st_size, allc / CLOCKS_PER_MQ);

	close(src_fd);
}

int write_append(const char *src, int test, int rdt)
{
	int written = 0;
	int src_fd = open(src, O_CREAT | O_WRONLY | O_APPEND, 0666);

	if (src_fd < 0) {
		fprintf(stdout, "Fichier non_existant\n");
		return -1;
	}
	struct stat st;

	fstat(src_fd, &st);
	int taille_avant = st.st_size;

	clock_t begin = clock();

	if (rdt)
		written = write(src_fd, READTEXT, sizeof(READTEXT));
	else
		written = write(src_fd, WRI, SIZEWRI);
	clock_t end = clock();

	fstat(src_fd, &st);

	close(src_fd);
	if (test) {
		printf("Write append:  ecriture: %d octets | attendu: %d octets | time: %ld micro_sec\n",
		       written, SIZEWRI, (end - begin) / CLOCKS_PER_MQ);
		printf("Write append:  Taille fichier: %d octets | attendu: %d octets\n",
		       (int)st.st_size, taille_avant + SIZEWRI);
	}

	return written;
}

int write_new(const char *src)
{
	int written = 0;
	int src_fd = open(src, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	if (src_fd < 0) {
		fprintf(stdout, "Fichier non_existant\n");
		return -1;
	}
	struct stat st;

	fstat(src_fd, &st);
	int taille_avant = st.st_size;

	clock_t begin = clock();

	written = write(src_fd, WRI, SIZEWRI);
	clock_t end = clock();

	fstat(src_fd, &st);

	close(src_fd);
	printf("Write new:  ecriture: %d octets | attendu: %d octets | time: %ld micro_sec\n",
	       written, SIZEWRI, (end - begin) / CLOCKS_PER_MQ);
	printf("Write new:  Taille fichier: %d octets | attendu: %d octets\n",
	       (int)st.st_size, taille_avant + SIZEWRI);
	return written;
}

int write_start(const char *src)
{
	int written = 0;
	int src_fd = open(src, O_CREAT | O_WRONLY, 0666);

	if (src_fd < 0) {
		fprintf(stdout, "Fichier non_existant\n");
		return -1;
	}

	struct stat st;

	fstat(src_fd, &st);
	int taille_avant = st.st_size;

	clock_t begin = clock();

	written = write(src_fd, WRI, SIZEWRI);
	clock_t end = clock();

	fstat(src_fd, &st);

	close(src_fd);
	printf("Write start:  ecriture: %d octets | attendu: %d octets | time: %ld micro_sec\n",
	       written, SIZEWRI, (end - begin) / CLOCKS_PER_MQ);
	printf("Write start:  Taille fichier: %d octets\n", (int)st.st_size);
	return written;
}

int write_mid(const char *src)
{
	int written = 0;
	int src_fd = open(src, O_CREAT | O_WRONLY, 0666);

	if (src_fd < 0) {
		fprintf(stdout, "Fichier non_existant\n");
		return -1;
	}

	struct stat st;

	fstat(src_fd, &st);
	int taille_avant = st.st_size;

	lseek(src_fd, st.st_size / 2, SEEK_SET);

	clock_t begin = clock();

	written = write(src_fd, WRI, SIZEWRI);
	clock_t end = clock();

	fstat(src_fd, &st);

	close(src_fd);
	printf("Write mid:  ecriture: %d octets | attendu: %d octets | time: %ld micro_sec\n",
	       written, SIZEWRI, (end - begin) / CLOCKS_PER_MQ);
	printf("Write mid:  Taille fichier: %d octets\n", (int)st.st_size);
	return written;
}

int write_offset_fromTheEnd(const char *src, size_t offset)
{
	int written = 0;
	int src_fd = open(src, O_CREAT | O_WRONLY, 0666);

	if (src_fd < 0) {
		fprintf(stdout, "Fichier non_existant\n");
		return -1;
	}
	struct stat st;

	fstat(src_fd, &st);
	int taille_avant = st.st_size;

	lseek(src_fd, offset, SEEK_END);

	clock_t begin = clock();

	written = write(src_fd, WRI, SIZEWRI);
	clock_t end = clock();

	fstat(src_fd, &st);

	close(src_fd);
	printf("Write off:  ecriture: %d octets | attendu: %d octets | time: %ld micro_sec\n",
	       written, (int)(SIZEWRI), (end - begin) / CLOCKS_PER_MQ);
	printf("Write off:  Taille fichier: %d octets | attendu: %d octets\n",
	       (int)st.st_size, (int)(taille_avant + SIZEWRI + offset));
	return written;
}

/*
 * insert dans un fichier à l'offset donnée
 * fait pour être utilisé avec el comportement classique du write et du read
 */
int insertion(const char *src, size_t offset, clock_t *time, int *sizeFicPrec,
	      int test)
{
	int src_fd = open(src, O_CREAT | O_RDONLY, 0666);
	int dest_fd = open(src, O_CREAT | O_WRONLY, 0666);
	int taille_apres = 0;

	if (src_fd < 0 || dest_fd < 0) {
		fprintf(stdout, "Fichier non_existant %s\n", src);
		return -1;
	}
	struct stat st;

	fstat(src_fd, &st);

	*sizeFicPrec = st.st_size;

	lseek(src_fd, offset, SEEK_SET);
	lseek(dest_fd, offset, SEEK_SET);

	char buffwr[MAX_BUFF];
	char buffrd[MAX_BUFF];

	memset(buffwr, 0, MAX_BUFF);
	memset(buffrd, 0, MAX_BUFF);
	memcpy(buffwr, WRI, SIZEWRI);

	clock_t begin = clock();
	size_t total;
	size_t rd = 0;
	size_t wr = 0;

	while ((rd = read(src_fd, buffrd, MAX_BUFF)) > 0) {
		write(dest_fd, buffwr, wr);
		memset(buffwr, 0, MAX_BUFF);
		wr = rd;
		memcpy(buffwr, buffrd, rd);
		memset(buffrd, 0, MAX_BUFF);
	}

	clock_t end = clock();
	*time = end - begin;

	fstat(src_fd, &st);
	taille_apres = st.st_size;

	close(src_fd);
	close(dest_fd);
	if (test) {
		printf("%s: time: %ld micro_sec\n", __func__,
		       (end - begin) / CLOCKS_PER_MQ);
		printf("%s:  Taille fichier: %d octets | attendu: %d octets\n",
		       __func__, (int)taille_apres,
		       (int)(*sizeFicPrec + SIZEWRI + offset));
	}
	return taille_apres;
}

void write_test(void)
{
	printf("======== test append dans /mnt/ouiche/appendwritetest.txt\n");
	write_append("/mnt/ouiche/appendwritetest.txt", 1, 0);
	printf("======== test start dans /mnt/ouiche/startwritetest.txt\n");
	write_start("/mnt/ouiche/startwritetest.txt");
	printf("======== test mid dans /mnt/ouiche/midwritetest.txt\n");
	write_mid("/mnt/ouiche/midwritetest.txt");
	printf("======== test new dans /mnt/ouiche/newwritetest.txt\n");
	write_new("/mnt/ouiche/newwritetest.txt");
	printf("======== test offset dans /mnt/ouiche/offsetwritetest.txt\n");
	write_offset_fromTheEnd("/mnt/ouiche/offsetwritetest.txt", OFFSET2);
}

int test_all(const char *src_rep)
{
	struct stat help;

	stat(src_rep, &help);
	if (!S_ISDIR(help.st_mode)) {
		printf("Dossier non existant %s\n", src_rep);
		return -1;
	}
	printf("src_rep : %s\n", src_rep);

	size_t sz = strlen(src_rep);

	char *read = "/loremtest.txt";
	char *app = "/appendwritetest.txt";
	char *start = "/startwritetest.txt";
	char *mid = "/midwritetest.txt";
	char *new = "/newwritetest.txt";
	char *off = "/offsetwritetest.txt";
	char *cop = "/copy.txt";

	char path[MAX_BUFF];

	memcpy(path, src_rep, sz);
	path[strlen(src_rep)] = '/';
	memcpy(path + sz, read, 15);

	char tmp[MAX_BUFF];

	memcpy(tmp, path, 15 + sz);

	write_append(path, 0, 1);
	read_test(path, src_rep);

	memcpy(path + (sz), app, 21);
	printf("\n======== test append dans %s\n", path);
	write_append(path, 1, 0);

	memcpy(path + (sz), start, 20);
	printf("\n======== test start dans %s\n", path);
	write_start(path);
	write_start(path);

	memcpy(path + (sz), mid, 18);
	printf("\n======== test mid dans %s\n", path);
	write_mid(path);
	write_mid(path);

	memcpy(path + (sz), new, 18);
	printf("\n======== test new dans %s\n", path);
	write_new(path);

	memcpy(path + (sz), off, 21);
	printf("\n======== test offset dans %s\n", path);
	write_offset_fromTheEnd(path, OFFSET2);
	write_offset_fromTheEnd(path, OFFSET1);

	printf("\n");
	memcpy(path + (sz), cop, 10);
	duplication_test(tmp, path, 1);
}

int toParam(const char *param)
{
	if (strcmp("-d", param) == 0)
		return 1;

	if (strcmp("-r", param) == 0)
		return 2;

	if (strcmp("-w", param) == 0)
		return 3;

	if (strcmp("-ws", param) == 0)
		return 4;

	if (strcmp("-wm", param) == 0)
		return 5;

	if (strcmp("-wa", param) == 0)
		return 6;

	if (strcmp("-wo", param) == 0)
		return 7;

	if (strcmp("-wall", param) == 0)
		return 8;

	if (strcmp("-i", param) == 0)
		return 9;

	if (strcmp("-A", param) == 0)
		return 10;

	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stdout, "Usage: benchmark <option> <path>\n");
		fprintf(stdout,
			"-d : duplication\n-r :read\n-w :write append\n-ws :write at the start\n-wm :write in the middle\n-wa :write in a new\n");
		fprintf(stdout, "-wo :write with an offset\n");
		fprintf(stdout, "-A <path_to_directory> :all the test\n");
		fprintf(stdout,
			"attention pour les writes les fichiers risquent d'être détruits\n");
		return EXIT_FAILURE;
	}

	switch (toParam(argv[1])) {
	case 1:
		/*
		 * Test de duplication : Read+Write
		 */
		if (argc < 3) {
			fprintf(stdout,
				"Usage: benchmark -d <path_to_source_file>\n");
			return EXIT_FAILURE;
		}
		if (argc == 3)
			duplication_test(argv[2], argv[3], 1);
		else
			duplication_test(argv[2],
					 "/mnt/ouiche/duplicationtest.txt", 1);
		break;
	case 2:
		/*
		 * Test de Read
		 */
		if (argc < 3) {
			fprintf(stdout,
				"Usage: benchmark -r <path_to_source_file>\n");
			return EXIT_FAILURE;
		}
		read_test(argv[2], "/mnt/ouiche");
		break;

	case 3:
		/*
		 * test write : append un fichier
		 */
		if (argc == 3)
			write_append(argv[2], 1, 0);
		else
			write_append("/mnt/ouiche/appendwritetest.txt", 1, 0);
		break;

	case 4:
		/*
		 * test write : debut d'un fichier
		 */
		if (argc == 3)
			write_start(argv[2]);
		else
			write_start("/mnt/ouiche/startwritetest.txt");
		break;

	case 5:
		/*
		 * test write : milieu d'un fichier
		 */
		if (argc == 3)
			write_mid(argv[2]);
		else
			write_mid("/mnt/ouiche/midwritetest.txt");
		break;

	case 6:
		/*
		 * test write : ecriture dans un fichier vierge
		 */
		if (argc == 3)
			write_new(argv[2]);
		else
			write_new("/mnt/ouiche/newwritetest.txt");
		break;
	case 7:
		/*
		 * test write : ecriture dans un fichier à un offest de la fin de un bloc
		 */
		if (argc == 3) {
			write_offset_fromTheEnd(argv[2], OFFSET1);
		} else
			write_offset_fromTheEnd(
				"/mnt/ouiche/offsetwritetest.txt", OFFSET1);
		break;
	case 8:
		/*
		 * Tous les tests d'ecriture sur des fichiers
		 */
		write_test();
		break;
	case 9:
		/*
		 * insertion dans un fichier
		 */
		clock_t tmp;
		int sizebefore;

		insertion(argv[2], NBCHAR, &tmp, &sizebefore, 1);
		break;
	case 10:
		if (argc == 3) {
			test_all(argv[2]);
			break;
		}
	default:
		fprintf(stdout, "Usage: benchmark <option> <path_>\n");
		fprintf(stdout,
			"-d : duplication\n-r :read\n-w :write append\n-ws :write at the start\n-wm :write in the middle\n-wa :write in a new\n");
		fprintf(stdout, "-wo :write with an offset\n");
		fprintf(stdout, "-A <path_to_directory> :all the test\n");
		fprintf(stdout,
			"attention pour les writes les fichiers risquent d'être détruits\n");
		break;
	}

	return 0;
}
