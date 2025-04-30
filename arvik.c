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

//#define OPTIONS "cxtvVf:h"
#define BUF 4096

static int help = 0;
static int verbose = 0;
//int validate_crc = 0;
static int f_used = 0;
//char archive_filename[BUF];
static char *archive_filename = NULL;
static var_action_t curr_action = ACTION_NONE;

//prototypes
void help_display(void);
void create_archive(int argc, char * argv[]);
void extract_archive(void);
void toc_archive(void);
void validate_archive(void);
void bad_tag_exit(void);

int main(int argc, char *argv[]) {

	//getopt stuff
	int opt;

	while((opt = getopt(argc, argv, ARVIK_OPTIONS)) != -1) {

		switch(opt) {
			case 'x':
				if(curr_action != ACTION_NONE && curr_action != ACTION_EXTRACT) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "x option\n");
				curr_action = ACTION_EXTRACT;
				break;
			case 'c':
				if(curr_action != ACTION_NONE && curr_action != ACTION_CREATE) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}

				fprintf(stderr, "c option\n");
				curr_action = ACTION_CREATE;
				break;
			case 't':
				if(curr_action != ACTION_NONE && curr_action != ACTION_TOC) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "t option\n");
				curr_action = ACTION_TOC;
				break;
			case 'f':
				fprintf(stderr, "f option\n");
				f_used = 1;
				archive_filename = optarg;
				break;
			case 'h':
				fprintf(stderr, "h option\n");
				help = 1;
				break;
			case 'v':
				fprintf(stderr, "v option\n");
				verbose = 1;
				break;
			case 'V':
				if(curr_action != ACTION_NONE && curr_action != ACTION_VALIDATE) {
					fprintf(stderr, "Please enter a valid command line.\n");
					exit(INVALID_CMD_OPTION);
				}

				fprintf(stderr, "V option\n");
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
	//TODO remove
	f_used++;
	verbose++;
	archive_filename++;

	return EXIT_SUCCESS;
}


//Display -h help
void help_display(void) {
	fprintf(stderr, "Placeholder Help\n");
	fprintf(stdout, "Usage: arvik -[cxtvVf:h] archive-file file...\n");
	fprintf(stdout, "	-c           create a new archive file\n");
	fprintf(stdout, "	-x           extract members from an existing archive file\n");
	fprintf(stdout, "	-t           show the table of contents of archive file\n");
	fprintf(stdout, "	-f filename  name of archive file to use\n");
	fprintf(stdout, "	-V           Validate the crc value for the data\n");
	fprintf(stdout, "	-v           verbose output\n");
	fprintf(stdout, "	-h           show help text\n");

	return;
}

void create_archive(int argc, char *argv[]) {
	
	int archive_fd;
	int use_stdout = 0;
	int i;
	//mode_t old_umask;
	fprintf(stderr, "Placeholder create_archive\n");

	//open archive file
	if(archive_filename != NULL) {
		archive_fd = open(archive_filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		if(archive_fd < 0) {
			perror("open");
			exit(CREATE_FAIL);
		}

		//permission setting
		fchmod(archive_fd, 0664);
	} else {
		archive_fd = STDOUT_FILENO;
		use_stdout = 1;
	}

	//ARVIK tag
	if (write(archive_fd, ARVIK_TAG, strlen(ARVIK_TAG)) != (ssize_t)strlen(ARVIK_TAG)) {
		perror("write tag");
		exit(CREATE_FAIL);
	}

	//if no member files given, exit after writing tag
	if(optind >= argc) {
		if(!use_stdout) {
			close(archive_fd);
		}
		return;
	}

	//process each file on cmd line
	
	for(i = optind; i < argc; i++) {
		char *filename = argv[i];
		int input_fd;
		struct stat sb;
		arvik_header_t header;
		arvik_footer_t footer;
		unsigned char buf[BUF];
		ssize_t bytes_read, bytes_written;
		ssize_t total_read = 0;
		//size_t name_len = 0;
		size_t copy_len = 0;	
		//temp storage to work around snprintf truncate warnings
		char temp_buf[32];
		char crc_buf[11];
		uLong crc = crc32(0L, Z_NULL, 0);

		//opening input fie
		input_fd = open(filename, O_RDONLY);
		if (input_fd < 0) {
			perror("open input file");
			exit(CREATE_FAIL);
		}

		//get metadata
		if (fstat(input_fd, &sb) < 0) {
			perror("fstat");
			close(input_fd);
			exit(CREATE_FAIL);
		}


		//get header

		//not working - warning from truncating null term
		//strncpy(header.arvik_name, filename, sizeof(header.arvik_name) - 1);
		//name_len = strlen(header.arvik_name);
	//	if (header.arvik_name[name_len - 1] != ARVIK_NAME_TERM) {
	//		if (name_len < sizeof(header.arvik_name) - 1) {
	//			header.arvik_name[name_len] = ARVIK_NAME_TERM;
	//			header.arvik_name[name_len + 1] = '\0';
	//		}
	//	}

		//set mem to space
		memset(&header, 32, sizeof(header));

		//setting filename	
		copy_len = MIN(strlen(filename), sizeof(header.arvik_name) - 2);
			   //sizeof(header.arvik_name) - 1);
		memcpy(header.arvik_name, filename, copy_len);

		//could want protective if statement
		header.arvik_name[copy_len] = ARVIK_NAME_TERM;

		//date
		sprintf(temp_buf, "%ld", sb.st_mtime);
		memcpy(header.arvik_date, temp_buf, strlen(temp_buf));	
		//uid
		sprintf(temp_buf, "%d", sb.st_uid);
		memcpy(header.arvik_uid, temp_buf, strlen(temp_buf));	
		//gid
		sprintf(temp_buf, "%d", sb.st_gid);
		memcpy(header.arvik_gid, temp_buf, strlen(temp_buf));	
		//mode
		sprintf(temp_buf, "%o", sb.st_mode);
		memcpy(header.arvik_mode, temp_buf, strlen(temp_buf));	
		//size
		sprintf(temp_buf, "%ld", sb.st_size);
		memcpy(header.arvik_size, temp_buf, strlen(temp_buf));	
		//term
		memcpy(header.arvik_term, ARVIK_TERM, sizeof(header.arvik_term));
		
		//copy in header
		//memcpy(header.arvik_name, filename, strlen(filename));
		//snprintf(header.arvik_date, sizeof(header.arvik_date), "-11%ld", sb.st_mtime);
		//snprintf(header.arvik_uid, sizeof(header.arvik_uid), "-5%d", sb.st_uid);
		//snprintf(header.arvik_gid, sizeof(header.arvik_gid), "%-5d", sb.st_gid);
		//snprintf(header.arvik_mode, sizeof(header.arvik_mode), "%-7o", sb.st_mode & 07777);
		//snprintf(header.arvik_size, sizeof(header.arvik_size), "%-9ld", sb.st_size);
		////strncpy(header.arvik_term, ARVIK_TERM, sizeof(header.arvik_term));
		//memcpy(header.arvik_term, ARVIK_TERM, sizeof(header.arvik_term));

		//write header
		if (write(archive_fd, &header, sizeof(header)) != sizeof(header)) {
			perror("write header");
			close(input_fd);
			exit(CREATE_FAIL);
		}

		//copy file contents
		while ((bytes_read = read(input_fd, buf, sizeof(buf))) > 0) {
			bytes_written = write(archive_fd, buf, bytes_read);
			if(bytes_written != bytes_read) {
				perror("Inequal bytes written and read");
				close(input_fd);
				exit(CREATE_FAIL);
			}
			crc = crc32(crc, buf, bytes_read);
			total_read += bytes_read;
		}
		
		//if read fails
		if(bytes_read < 0) {
			perror("read input file");
			close(input_fd);
			exit(CREATE_FAIL);
		}

		//if total_read is odd, add newline
		if (total_read % 2 == 1 ) {
			if(write(archive_fd, "\n", 1) != 1) {
				perror("writing offset newline if odd bytes");
				close(input_fd);
				exit(CREATE_FAIL);
			}
		}

		//write footer
		memset(&footer, 32, sizeof(footer));
		
		//sprintf(footer.arvik_data_crc, "0x%08lx", crc);
		sprintf(crc_buf, "0x%08lx", crc);
		memcpy(footer.arvik_data_crc, crc_buf, sizeof(footer.arvik_data_crc));
		
		//snprintf(footer.arvik_data_crc, sizeof(footer.arvik_data_crc), "0x%08lx", crc); //warning from truncating null
		//strncpy(footer.arvik_term, ARVIK_TERM, sizeof(footer.arvik_term)); //warning for null term
		
		memcpy(footer.arvik_term, ARVIK_TERM, sizeof(footer.arvik_term));

		if (write(archive_fd, &footer, sizeof(footer)) != sizeof(footer)) {
			perror("writing footer");
			close(input_fd);
			exit(CREATE_FAIL);
		}

		close(input_fd);

		if(verbose) {
			fprintf(stderr, "Added: %s\n", filename);
		}

	}//end for

	if (!use_stdout) {
		close(archive_fd);
	}
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
	exit(BAD_TAG);
	return;

}
