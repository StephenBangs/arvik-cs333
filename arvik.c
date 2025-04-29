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

//typedef enum opt_enum {
//	ACTION_NONE = 0,
//	ACTION_EXTRACT,
//	ACTION_CREATE,
//	ACTION_TOC,
//	ACTION_VALIDATE
//} opt_actions;

//prototypes
void help_display(void);
void create_archive(int argc, char * argv[]);
void extract_archive(void);
void toc_archive(void);
void validate_archive(void);
void bad_tag_exit(void);

int main(int argc, char *argv[]) {

	
	//getopt stuff
	int help = 0;
	int verbose = 0;
	//int validate_crc = 0;
	int f_used = 0;
	//char archive_filename[BUF];
	char *archive_filename = NULL;
	var_action_t curr_action = ACTION_NONE;
	int opt;

	while((opt = getopt(argc, argv, OPTIONS)) != -1) {

		switch(opt) {
			case 'x':
				if(curr_action != ACTION_NONE && curr_action != ACTION_EXTRACT) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}

				curr_action = ACTION_EXTRACT;
				break;
			case 'c':
				if(curr_action != ACTION_NONE && curr_action != ACTION_CREATE) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}
				curr_action = ACTION_CREATE;
				break;
			case 't':
				if(curr_action != ACTION_NONE && curr_action != ACTION_TOC) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}
				curr_action = ACTION_TOC;
				break;
			case 'f':
				f_used = 1;
				archive_filename = optarg;
				break;
			case 'h':
				help = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'V':
				if(curr_action != ACTION_NONE && curr_action != ACTION_VALIDATE) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}
				curr_action = ACTION_VALIDATE;
				//validate_crc = 1;
				break;
			default:
				fprintf(stderr, "Please enter a valid command line option.\n");
				exit(INVALID_CMD_OPTION);
				break;
		}
	}
	
	if(help == 1) {
		help_display();
	}

	switch (curr_action) {
		case ACTION_NONE:
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
	//TODO
	f_used++;
	verbose++;
	archive_filename++;

	return EXIT_SUCCESS;
}



void help_display(void) {
	fprintf(stderr, "Placeholder Help\n");
	return;
}

void create_archive(int argc, char *argv[]) {
	fprintf(stderr, "Placeholder create_archive\n");
	argc++;
	argv++;
	return;
}
void extract_archive(void){

	fprintf(stderr, "Placeholder extract_archive\n");
	return;

}
void toc_archive(void){

	fprintf(stderr, "Placeholder toc_archive\n");
	return;

}
void validate_archive(void){

	fprintf(stderr, "Placeholder validate_archive\n");
	return;

}
void bad_tag_exit(void) {

	fprintf(stderr, "Placeholder bad_tag_exit\n");
	return;

}
