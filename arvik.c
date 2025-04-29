//Stephen Bangs
//CS 333 - Jesse Chaney
//4/29/25
//Arvik Reads, Writes, and Archives files in babbage using a given arvik.h file supplied by jesse chaney

#include "arvik.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <errno.h>
#include <time.h>
#include <zlib.h>

#define OPTIONS "cxtvVf:h"
#define BUF 2056

typedef enum opt_enum {
	ACTION_DEFAULT = 0,
	ACTION_EXTRACT,
	ACTION_CREATE,
	ACTION_TOC,
	ACTION_VALIDATE
} opt_actions;

//prototypes
void help();
void create_archive(int argc, char * argv[]);
void extract_archive();
void toc_archive();
void validate _archive();
void bad_tag_exit();

int main(int argc, char *argv[]) {

	
	//getopt stuff
	int help = 0;
	int verbose = 0;
	//int validate_crc = 0;
	int f_used = 0;
	//char archive_filename[BUF];
	char *archive_filename = NULL;
	opt_actions curr_action = ACTION_DEFAULT;
	int opt;

	while((opt = getopt(argv, argv, OPTIONS)) != -1) {

		switch(opt) {
			case 'x':
				if(curr_action != ACTION_DEFAULT && curr_action != ACTION_EXTRACT) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}

				action = ACTION_EXTRACT;
				break;
			case 'c':
				if(curr_action != ACTION_DEFAULT && curr_action != ACTION_CREATE) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}
				action = ACTION_CREATE;
				break;
			case 't':
				if(curr_action != ACTION_DEFAULT && curr_action != ACTION_TOC) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}
				action = ACTION_TOC;
				break;
			case 'f':
				f_used = 1;
				archive_filename = optarg
				break;
			case 'h':
				help = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'V':
				if(curr_action != ACTION_DEFAULT && curr_action != ACTION_VALIDATE) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}
				action = ACTION_VALIDATE;
				//validate_crc = 1;
				break;
			case default:
				fprintf(stderr, "Please enter a valid command line option.\n");
				exit(INVALID_CMD_OPTION);
				break;
		}
	}
	
	if(h == 1) {
		help();
	}

	switch (curr_action) {
		case ACTION_DEFAULT:
			fprintf(stderr, "No action specified\n");
			exit(NO_ACTION_GIVEN);
			break;
		case ACTION_EXTRACT:
			extract_archive();
			break;
		case ACTION_CREATE:
			create_archive(argc, argv);
			break;
		case ACTION_TOC:
			toc_archive();
			break;
		case ACTION_VALIDATE:
			validate_archive();
			break;
		default:
			fprintf(stderr, "Unknown command.\n");
			exit(INVALID_CMD_OPTION);
			break;
	}


	return EXIT_SUCCESS;
}



void help() {
	fprintf(stderr, "Placeholder Help\n");
	return;
}

void create_archive(int argc, char *argv[]) {
	fprintf(stderr, "Placeholder create_archive\n");
	return;
}
void extract_archive(){

	fprintf(stderr, "Placeholder extract_archive\n");
	return;

}
void toc_archive(){

	fprintf(stderr, "Placeholder toc_archive\n");
	return;

}
void validate _archive(){

	fprintf(stderr, "Placeholder validate_archive\n");
	return;

}
void bad_tag_exit() {

	fprintf(stderr, "Placeholder bad_tag_exit\n");
	return;

}
