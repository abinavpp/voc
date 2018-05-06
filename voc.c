/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2017 Abinav Puthan Purayil
 * */

#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <getopt.h>

#define MAX_FILES 64
#define MAX_WORDLENGTH 128
#define MAX_WORDS 256
#define BUFSIZE 4096

void usage(){

	fprintf(stderr, ""
"Usage : voc -<option> <optarg> <voc_db>\n"
"where <voc_db> is a dir containing all voc files\n"
"\n"
"interactive:\n"
"   voc -(c|q) <file> <voc_db>\n"
"   where -c for choice, -q for quiz from voc file <file>\n"
"   which must be in relative path to voc_db (not absolute!)\n"
"\n"
"non-interactive:\n"
"   voc -r <n> <voc_db>\n"
"   prints n number of randoms definitions\n"
"   from any voc file in voc_db\n"
	);

}

void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	_exit(EXIT_FAILURE);
}

/* return (if path is a dir) ? 1 : 0 */
int is_dir(const char *path)
{
	struct stat sbuf;
	if ((stat(path, &sbuf)) == -1)
		die("stat(\"%s\") : %s\n", path, strerror(errno));

	return S_ISDIR(sbuf.st_mode);
}

/* return (if path is a reg_file) ? 1 : 0 */
int is_regfile(const char *path)
{
	struct stat sbuf;
	if ((stat(path, &sbuf)) == -1)
		die("stat(\"%s\") : %s\n", path, strerror(errno));

	return S_ISREG(sbuf.st_mode);
}

/*
 * Sets words and its corresponding meanings for the file pointed by
 * fd and sets n to number of words found.
 */
void fill_dict(int fd, char words[][MAX_WORDLENGTH],
		char meanings[][MAX_WORDLENGTH], int *n)
{
	int red, iwords, ibuf;
	char buf[BUFSIZE] = {0};

	for (iwords = 0; (read(fd, buf, BUFSIZE));) {
		for (ibuf = 0; buf[ibuf]; iwords++) {
			for (int i = 0; buf[ibuf] && buf[ibuf] != ':'; ibuf++)
				words[iwords][i++] = buf[ibuf];
			ibuf++;
			for (int i = 0; buf[ibuf] && buf[ibuf] != '\n'; ibuf++)
				meanings[iwords][i++] = buf[ibuf];
			ibuf++;
		}
	}
	*n = iwords;
}

/*
 * sets file to a random reg file in dir path.
 * returns 1 if successful, else -1
 */
int rand_file(char *path, char *file)
{
	DIR *dirp;
	struct dirent *entry;
	char files[MAX_FILES][MAX_WORDLENGTH] = {0};
	int i = 0;

	if (!(dirp = opendir(path)))
		goto err;
	for (; (entry = readdir(dirp));) {
		if (entry->d_type == DT_REG)
			strcpy(files[i++], entry->d_name);
	}

	if (!i) { /* no reg files in path */
		goto err;
	}

	srand(time(NULL));
	strcpy(file, files[rand() % i]);
	return 1;

err:
	return -1;
}

void start_quiz(char words[][MAX_WORDLENGTH],
	   char meanings[][MAX_WORDLENGTH], int n)
{
	for (int i=0; i<n; i++) {
		printf("\n\nDefine %s ? ", words[i]);
		getchar();
		printf("\n%s ", meanings[i]);
		getchar();
		printf("\033c");
	}
}

void start_interactive_choice(char words[][MAX_WORDLENGTH],
		char meanings[][MAX_WORDLENGTH], int n)
{
	int choice = 0;

	for (int i=0; i<n; i++)
		printf("%d %s | ", i, words[i]);

	while (1) {
		
		printf("\n ? ");
		scanf("%d", &choice); getchar();
		
		if (choice >=n)
			continue;
		if (choice < 0)
			break;
		printf("%s -> %s\n", words[choice], meanings[choice]);
	}
}

int main(int argc, char *argv[])
{
	char voc_db[PATH_MAX], fname[MAX_WORDLENGTH];
	char words[MAX_WORDS][MAX_WORDLENGTH] = {0};
	char meanings[MAX_WORDS][MAX_WORDLENGTH] = {0};
	int fd, n, opt, irand, nrand, run = 0;

	if (argc != 4) {
		goto invalid_format;
	}

	strcpy(voc_db, argv[argc - 1]);

	if (!(is_dir(voc_db)))
		die("Invalid db\n");

	/* if arg for voc_db doesn't have a trailing / */
	strcat(voc_db, "/");

	while ((opt = getopt(argc, argv, "c:q:r:")) != -1) {
		switch (opt) {
		case 'c' :
			strcat(voc_db, optarg);
			run = 'c';
			break;
		case 'q' :
			strcat(voc_db, optarg);
			run = 'q';
			break;
		case 'r' :
			nrand = atoi(optarg);
			run = 'r';
			if (rand_file(voc_db, fname) == -1) {
				die("invalid db\n");
			}
			strcat(voc_db, fname);
			break;

		case '?' :
			goto invalid_format;

		default :
			goto invalid_format;
			
		}
	}

	/* voc_db should refer to a file now */

	if (!(is_regfile(voc_db)))
		die("%s is not a regular file\n", voc_db);

	if ((fd = open(voc_db, O_RDONLY)) == -1)
		die("open(\"%s\") : %s\n", voc_db, strerror(errno));

	fill_dict(fd, words, meanings, &n);

	switch (run) {
	case 'c' :
		start_interactive_choice(words, meanings, n);
		break;

	case 'q' :
		start_quiz(words, meanings, n);
		break;

	case 'r' :
		for (int i = 0; i < nrand; i++) {
			irand = rand() % n;
			while (strlen(words[irand]) <= 1)
				irand++;
			printf("%s -> %s\n", words[irand], meanings[irand]);
		}
		break;
	}

	return EXIT_SUCCESS;

invalid_format:
	fprintf(stderr, "Invalid format\n");
	usage();
	return EXIT_FAILURE;
}
