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
					//exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "x option\n");
				curr_action = ACTION_EXTRACT;
				break;
			case 'c':
				if(curr_action != ACTION_NONE /*&& curr_action != ACTION_CREATE*/) {
					fprintf(stderr, "Please enter a valid command line.\n");
					//exit(INVALID_CMD_OPTION);
				}

				fprintf(stderr, "c option\n");
				curr_action = ACTION_CREATE;
				break;
			case 't':
				if(curr_action != ACTION_NONE && curr_action != ACTION_TOC) {
					fprintf(stderr, "Please enter a valid command line.\n");
					//exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "t option\n");
				curr_action = ACTION_TOC;
				break;
			case 'f':
				if (optarg == NULL || strlen(optarg) == 0) {
					fprintf(stderr, "-f requires a filename argument\n");
					//exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "f option\n");
				if(optarg != NULL){
					f_used = 1;
					archive_filename = optarg;
				}
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
					//exit(INVALID_CMD_OPTION);
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
	
	//credit: stack overflow
	for (int i = optind; i < argc; i++) {
	    fprintf(stderr, "Non-option arg: %s\n", argv[i]);
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
		size_t copy_len = 0;	
		//temp storage to work around snprintf truncate warnings
		char temp_buf[32];
		char crc_buf[11];
		uLong crc = crc32(0L, Z_NULL, 0);

		if(filename == NULL) {
			fprintf(stderr, "NULL filename");
		}

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


		//set header
		//set mem to space
		memset(&header, 32, sizeof(header));

		//setting filename	
		copy_len = MIN(strlen(filename), sizeof(header.arvik_name) - 2);
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
		
		//CRC
		sprintf(crc_buf, "0x%08lx", crc);
		memcpy(footer.arvik_data_crc, crc_buf, sizeof(footer.arvik_data_crc));
		//terminator	
		memcpy(footer.arvik_term, ARVIK_TERM, sizeof(footer.arvik_term));

		//error for footer
		if (write(archive_fd, &footer, sizeof(footer)) != sizeof(footer)) {
			perror("writing footer");
			close(input_fd);
			exit(CREATE_FAIL);
		}

		close(input_fd);
		
		//TODO
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

	int archive_fd;
	arvik_header_t header;
	arvik_footer_t footer;
	char buf[BUF];
	char temp_buf[11];
	char filename[sizeof(header.arvik_name)];
	ssize_t bytes_read, bytes_to_read;
	uLong crc;
//	char expected_crc[sizeof(footer.arvik_data_crc)];
	char tag_buf[sizeof(ARVIK_TAG)];
	int out_fd;
	ssize_t remaining, chunk;
	char pad;
	char *slash;
	//stat info
	long size = 0;
	//int uid = 0;
	//int gid = 0;
	int mode = 0;
	time_t mtime = 0;
	struct timespec times[2];

	fprintf(stderr, "Placeholder extract_archive\n");
	
	if(archive_filename) {
		archive_fd = open(archive_filename, O_RDONLY);
		if (archive_fd < 0) {
			perror("open archive file");
			exit(EXTRACT_FAIL);
		}
	}
	else {
		archive_fd = STDIN_FILENO;
	}
	
	//validate archive tag
	bytes_read = read(archive_fd, tag_buf, strlen(ARVIK_TAG));
	if(bytes_read != (ssize_t)strlen(ARVIK_TAG) || strncmp(tag_buf, ARVIK_TAG, strlen(ARVIK_TAG)) != 0) {
		fprintf(stderr, "Invalid arvik tag\n");
		exit(BAD_TAG);
	}

	//read members til EOF
	while ((bytes_read = read(archive_fd, &header, sizeof(header))) == sizeof(header)) {
		//extract filename to /
		memset(filename, 0, sizeof(filename));
		memcpy(filename, header.arvik_name, sizeof(header.arvik_name));
		slash = strchr(filename, ARVIK_NAME_TERM);
		if (slash) {
			*slash = '\0';
		}
		
		//parse metadata
		size = strtol(header.arvik_size, NULL, 10);	
	//	uid = strtol(header.arvik_uid, NULL, 10);	
	//	gid = strtol(header.arvik_gid, NULL, 10);	
		mode = strtol(header.arvik_mode, NULL, 8);	
		mtime = strtol(header.arvik_date, NULL, 10);	

		//create output file
		out_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(out_fd < 0) {
			perror("Perror create output file");
			exit(EXTRACT_FAIL);
		}

		//r/w file data
		remaining = size;
		fprintf(stderr, "remaining: %ld\n", remaining);
		crc = crc32(0L, Z_NULL, 0);
	
		//read loop
		while (remaining > 0) {
			bytes_to_read = (remaining > BUF) ? BUF : remaining;
			chunk = read(archive_fd, buf, bytes_to_read);
			if(chunk <= 0) {
				perror("reading file content");
				close(out_fd);
				exit(EXTRACT_FAIL);
			}

			//write to file
			if (write(out_fd, buf, chunk) != chunk) {
				perror("writing to file");
				close(out_fd);
				exit(EXTRACT_FAIL);
			}


			crc = crc32(crc, (unsigned char *)buf, chunk);
			remaining -= chunk;
		}

		if(size % 2 == 1) {
			if (read(archive_fd, &pad, 1) != 1) {
				perror("read padding byte");
				close(out_fd);
				exit(EXTRACT_FAIL);
			}
		}
		
		if(read(archive_fd, &footer, sizeof(footer)) != sizeof(footer)) {
				fprintf(stderr, "Could not read footer\n");
				close(out_fd);
				exit(EXTRACT_FAIL);
		}

		sprintf(temp_buf, "0x%08lx", crc);
		memcpy(footer.arvik_data_crc, temp_buf, strlen(temp_buf));
		if(strncmp(temp_buf, footer.arvik_data_crc, sizeof(footer.arvik_data_crc)) != 0) {
			fprintf(stderr, "Warning: CRC mismatch on %s\n", filename);
			//TODO POSSIBLY WITH VALIDATION
			exit(CRC_DATA_ERROR);
		}

		fchmod(out_fd, mode);
		times[0].tv_sec = mtime;
		times[0].tv_nsec = 0;
		times[1].tv_sec = mtime;
		times[1].tv_nsec = 0;
		futimens(out_fd, times);

		close(out_fd);


		if(verbose) {
			fprintf(stderr, "Extracted %s\n", filename);
		}
	}//end member read while

	if(bytes_read != 0) {
		fprintf(stderr, "Corrupt archive - truncated header\n");
		exit(READ_FAIL);
	}

	if(archive_fd != STDIN_FILENO) {
		close(archive_fd);
	}

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
