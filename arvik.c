//Stephen Bangs
//CS 333 - Jesse Chaney
//4/29/25
//Arvik Reads, Writes, and Archives files in babbage using a given arvik.h file supplied by jesse chaney
//I use five main functions for the main options, with verbose and help as a static tag

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
#include <pwd.h>
#include <grp.h>

//#define OPTIONS "cxtvVf:h"
#define BUF 4096

static int help = 0;
static int verbose = 0;
//holds the archive name for use in different functions without passing. could be moved to an argument
static char *archive_filename = NULL;
//holds the effective output for getopt
static var_action_t curr_action = ACTION_NONE;

//prototypes
//displays formatting for help
void help_display(void);
//creates an archive of type .arvik
void create_archive(int argc, char * argv[]);
//extracts files from an arvik archive
void extract_archive(void);
//displays list of files in a given archive_filename. More with verbose [-v] enabled
void toc_archive(void);
//validates that files in an archive are correctly formatted and their crc is valid
void validate_archive(void);

int main(int argc, char *argv[]) {

	//getopt option handling for ARVIK_OPTIONS
	int opt;
	while((opt = getopt(argc, argv, ARVIK_OPTIONS)) != -1) {

		switch(opt) {
			//exraction
			case 'x':
				if(curr_action != ACTION_NONE && curr_action != ACTION_EXTRACT) {
					fprintf(stderr, "Please enter a valid command line.\n");
					//exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "x option\n");
				curr_action = ACTION_EXTRACT;
				break;
			//creation
			case 'c':
				if(curr_action != ACTION_NONE /*&& curr_action != ACTION_CREATE*/) {
					fprintf(stderr, "Please enter a valid command line.\n");
					//exit(INVALID_CMD_OPTION);
				}

				fprintf(stderr, "c option\n");
				curr_action = ACTION_CREATE;
				break;
			//table of contents
			case 't':
				if(curr_action != ACTION_NONE && curr_action != ACTION_TOC) {
					fprintf(stderr, "Please enter a valid command line.\n");
					//exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "t option\n");
				curr_action = ACTION_TOC;
				break;
			//filename required argument
			case 'f':
				if (optarg == NULL || strlen(optarg) == 0) {
					fprintf(stderr, "-f requires a filename argument\n");
					//exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "f option\n");
				if(optarg != NULL){
					archive_filename = optarg;
				}
				break;
			//displays help flag
			case 'h':
				fprintf(stderr, "h option\n");
				help = 1;
				break;
			//verbose version - TOC matters for formatting
			case 'v':
				fprintf(stderr, "v option\n");
				verbose = 1;
				break;
			//validate files inside archive for header, crc, read fails
			case 'V':
				if(curr_action != ACTION_NONE && curr_action != ACTION_VALIDATE) {
					fprintf(stderr, "Please enter a valid command line.\n");
					//exit(INVALID_CMD_OPTION);
				}
				fprintf(stderr, "V option\n");
				curr_action = ACTION_VALIDATE;
				break;
			default:
				fprintf(stderr, "Please enter a valid command line option.\n");
				exit(INVALID_CMD_OPTION);
				break;
		}
	}
	
	//credit: stack overflow and jesse's slides once I remembered where they were
	//checking for bugs with handling weird arguments
	for (int i = optind; i < argc; i++) {
	    fprintf(stderr, "Non-option arg: %s\n", argv[i]);
	}

	//display help if -h has appeared
	if(help == 1) {
		help_display();
	}
	
	//take action based on user options.
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


//Display -h help formatting, just like jesse's
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

//creates an archive of type .arvik
void create_archive(int argc, char *argv[]) {
	
	int archive_fd;
	//flag for checking stdin or named archive
	int use_stdout = 0;
	int i;
	fprintf(stderr, "Placeholder create_archive\n");

	//open archive file
	if(archive_filename != NULL) {
		archive_fd = open(archive_filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		if(archive_fd < 0) {
			perror("open");
			exit(READ_FAIL);
		}

		//permission setting for schmodding
		fchmod(archive_fd, 0664);
	} else {
		archive_fd = STDOUT_FILENO;
		//for closing file or not at the end of function
		use_stdout = 1;
	}

	//ARVIK tag
	if (write(archive_fd, ARVIK_TAG, strlen(ARVIK_TAG)) != (ssize_t)strlen(ARVIK_TAG)) {
		perror("write tag");
		exit(BAD_TAG);
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
		
		//hold current file nom
		char *filename = argv[i];
		//file descriptor for reading file
		int input_fd;
		//meta data struct
		struct stat sb;
		//from arvik.h
		arvik_header_t header;
		arvik_footer_t footer;
		//misc buffer of size 4096
		unsigned char buf[BUF];
		//for reading and writing
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
			exit(READ_FAIL);
		}

		//get metadata
		if (fstat(input_fd, &sb) < 0) {
			perror("fstat");
			close(input_fd);
			exit(READ_FAIL);
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
				exit(READ_FAIL);
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
	
		//verbose option not super needed
		if(verbose) {
			fprintf(stderr, "Added: %s\n", filename);
		}

	}//end for loop that processes cmd line files

	if (!use_stdout) {
		close(archive_fd);
	}
	return;
}

//extracts files from an arvik archive
void extract_archive(void){

	//outrageous variable list, because I am not good at being elegant
	//file descriptors
	int archive_fd;
	int out_fd;

	//given header and footer structs
	arvik_header_t header;
	arvik_footer_t footer;
	char filename[sizeof(header.arvik_name)];
	char buf[BUF];
	//helps with crc32 characters
	char temp_buf[11];
	//for helping with bugs concerning arvik tag
	char tag_buf[sizeof(ARVIK_TAG)];

	//for reading files in archive
	ssize_t bytes_read, bytes_to_read;
	ssize_t remaining, chunk;
	//checking padding
	char pad;
	//for removing the trailing slash in file names
	char *slash;
	
	//handling metadata
	long size = 0;
	int mode = 0;
	time_t mtime = 0;
	struct timespec times[2];
	uLong crc;

	fprintf(stderr, "Placeholder extract_archive\n");

	//open given archive
	if(archive_filename) {
		archive_fd = open(archive_filename, O_RDONLY);
		if (archive_fd < 0) {
			perror("open archive file");
			exit(EXTRACT_FAIL);
		}
	}//otherwise just use stdin
	else {
		archive_fd = STDIN_FILENO;
	}
	
	//validate archive tag - I didn't know this was only supposed to happen in Validate 
	//until I already finished
	bytes_read = read(archive_fd, tag_buf, strlen(ARVIK_TAG));
	if(bytes_read != (ssize_t)strlen(ARVIK_TAG) || strncmp(tag_buf, ARVIK_TAG, strlen(ARVIK_TAG)) != 0) {
		fprintf(stderr, "Invalid arvik tag\n");
		exit(BAD_TAG);
	}

	//read members til EOF
	while ((bytes_read = read(archive_fd, &header, sizeof(header))) == sizeof(header)) {
		//extract filename, remove trailing slash
		memset(filename, 0, sizeof(filename));
		memcpy(filename, header.arvik_name, sizeof(header.arvik_name));
		slash = strchr(filename, ARVIK_NAME_TERM);
		if (slash) {
			*slash = '\0';
		}
		
		//parse metadata
		size = strtol(header.arvik_size, NULL, 10);	
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
	
		//read loop for data in file
		while (remaining > 0) {
			bytes_to_read = (remaining > BUF) ? BUF : remaining;
			chunk = read(archive_fd, buf, bytes_to_read);
			//number of bytes actually read from read() is important
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
			
			//calculate crc32 each write, decrement remaining
			crc = crc32(crc, (unsigned char *)buf, chunk);
			remaining -= chunk;
		}
		
		//calculate if padding is acceptable, or error in extraction
		if(size % 2 == 1) {
			if (read(archive_fd, &pad, 1) != 1) {
				perror("read padding byte");
				close(out_fd);
				exit(EXTRACT_FAIL);
			}
		}
	
		//read footer
		if(read(archive_fd, &footer, sizeof(footer)) != sizeof(footer)) {
				fprintf(stderr, "Could not read footer\n");
				close(out_fd);
				exit(EXTRACT_FAIL);
		}

		//print into temp buf, then copy the crc over
		sprintf(temp_buf, "0x%08lx", crc);
		memcpy(footer.arvik_data_crc, temp_buf, strlen(temp_buf));
		if(strncmp(temp_buf, footer.arvik_data_crc, sizeof(footer.arvik_data_crc)) != 0) {
			fprintf(stderr, "Warning: CRC mismatch on %s\n", filename);
			//TODO POSSIBLY WITH VALIDATION - yep, this was all supposed to happen 
			//just with -V. oh well.
			exit(CRC_DATA_ERROR);
		}
		
		//set file permissions and time w/ futimens
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

	//error checking
	if(bytes_read != 0) {
		fprintf(stderr, "Corrupt archive - truncated header\n");
		exit(READ_FAIL);
	}

	if(archive_fd != STDIN_FILENO) {
		close(archive_fd);
	}

	return;
}

//displays list of files in a given archive_filename. More with verbose [-v] enabled
void toc_archive(void){
	//outrageous variable list partie deux
	int archive_fd;
	arvik_header_t header;
	arvik_footer_t footer;
	char tag_buf[sizeof(ARVIK_TAG)];
	char filename[sizeof(header.arvik_name)];
	//verbose metadata
	long size;
	int mode, uid, gid;
	time_t mtime;
	//temp chars for various things. Could be trimmed
	char mode_str[11];
	char temp[16];
	char buf[BUF];
	char time_buf[64];
	//verbose info structs
	struct tm *timeinfo;
	struct passwd *pw;
	struct group *gr;
	//reading info from file
	ssize_t bytes_read, skip_bytes, chunk;
	//checking padding
	char pad;
	//removing slash from file name
	char *slash = NULL;
	uLong crc;


	fprintf(stderr, "Placeholder toc_archive\n");

	//open the archive
	if(archive_filename != NULL) {
		archive_fd = open(archive_filename, O_RDONLY);
		if (archive_fd < 0) {
			perror("open archive");
			exit(READ_FAIL);
		}
	}
	else {
		archive_fd = STDIN_FILENO;
	}

	//read the arvik tag, error check
	bytes_read = read(archive_fd, tag_buf, strlen(ARVIK_TAG));
	if (bytes_read != (ssize_t)strlen(ARVIK_TAG) || strncmp(tag_buf, ARVIK_TAG, strlen(ARVIK_TAG)) != 0) {
		perror("Invalid Arvik tag");
		exit(BAD_TAG);
	}

	//grab header loop
	while ((bytes_read = read(archive_fd, &header, sizeof(header))) == sizeof(header)) {
	
		//grab filename
		memset(filename, 0, sizeof(filename));
		memcpy(filename, header.arvik_name, sizeof(header.arvik_name));
		filename[sizeof(header.arvik_name) - 1] = '\0';
		slash = strchr(filename, ARVIK_NAME_TERM);
		if (slash) {
			*slash = '\0';
		}
		
		//concerned about not resetting temp buffer properly, so I do
		//it every time
		memset(temp, 0, sizeof(temp));
		//metadata, copy into each data section's variable
		memcpy(temp, header.arvik_size, sizeof(header.arvik_size));
		temp[sizeof(header.arvik_size)] = '\0';
		size = strtol(temp, NULL, 10);
		//uid
		memset(temp, 0, sizeof(temp));
		memcpy(temp, header.arvik_uid, sizeof(header.arvik_uid));
		temp[sizeof(header.arvik_uid)] = '\0';
		uid = strtol(temp, NULL, 10);
		//gid	
		memset(temp, 0, sizeof(temp));
		memcpy(temp, header.arvik_gid, sizeof(header.arvik_gid));
		temp[sizeof(header.arvik_gid)] = '\0';
		gid = strtol(temp, NULL, 10);
		//permissions
		memset(temp, 0, sizeof(temp));
		memcpy(temp, header.arvik_mode, sizeof(header.arvik_mode));
		temp[sizeof(header.arvik_mode)] = '\0';
		mode = strtol(temp, NULL, 8);
		//time
		memset(temp, 0, sizeof(temp));
		memcpy(temp, header.arvik_date, sizeof(header.arvik_date));
		temp[sizeof(header.arvik_date)] = '\0';
		mtime = strtol(temp, NULL, 10);

		//mode to string
		//credit: stack overflow
		//does not work with directory prefix
		//mode_str[0] = (S_ISDIR(mode)) ? 'd' : '-';
		mode_str[0] = (mode & 0400) ? 'r' : '-';
		mode_str[1] = (mode & 0200) ? 'w' : '-';
		mode_str[2] = (mode & 0100) ? 'x' : '-';
		mode_str[3] = (mode & 0040) ? 'r' : '-';
		mode_str[4] = (mode & 0020) ? 'w' : '-';
		mode_str[5] = (mode & 0010) ? 'x' : '-';
		mode_str[6] = (mode & 0004) ? 'r' : '-';
		mode_str[7] = (mode & 0002) ? 'w' : '-';
		mode_str[8] = (mode & 0001) ? 'x' : '-';
		mode_str[9] = '\0';

		if(!verbose) {
			fprintf(stdout, "%s\n", filename);
		}
		else {
			//in depth formatting for TOC -v verbose mode
			pw = getpwuid(uid);
			gr = getgrgid(gid);

			//format for time
			timeinfo = localtime(&mtime);
			strftime(time_buf, sizeof(time_buf), "%b %e %R %Y", timeinfo);

			//full verbose output
			fprintf(stdout, "file name: %s\n", filename);

			fprintf(stdout, "%-16s", "    mode:");
			fprintf(stdout, "%9s\n", mode_str);

			fprintf(stdout, "%-16s", "    uid:");
			fprintf(stdout, "%9d  %s\n", uid, pw ? pw->pw_name : "unknown");

			fprintf(stdout, "%-16s", "    gid:");
			fprintf(stdout, "%9d  %s\n", gid, gr ? gr->gr_name : "unknown");
			
			fprintf(stdout, "%-16s", "    size:");
			fprintf(stdout, "%9ld bytes\n", size);

			fprintf(stdout, "%-16s", "    mtime:");
			fprintf(stdout, "%s\n", time_buf);
		}
		
		//compute and display crc
		crc = crc32(0L, Z_NULL, 0);
		skip_bytes = size;
		//parse thru file data until we get to end, computing crc whole time. this was unneeded.
		while (skip_bytes > 0) {
		    chunk = (skip_bytes > BUF) ? BUF : skip_bytes;
		    if (read(archive_fd, buf, chunk) != chunk) {
			fprintf(stderr, "Failed to read file content for %s\n", filename);
			exit(TOC_FAIL);
		    }
		    crc = crc32(crc, (unsigned char *)buf, chunk);
		    skip_bytes -= chunk;
		}

		//read padding - error checking if padding is off.
		if (size % 2 == 1) {
			if (read(archive_fd, &pad, 1) != 1) {
				perror("reading padding");
				exit(TOC_FAIL);
			}
		}
		
		//read footer
		if (read(archive_fd, &footer, sizeof(footer)) != sizeof(footer)) {
			fprintf(stderr, "Failed to read footer\n");
			exit(TOC_FAIL);
		}

		if(verbose) {
			fprintf(stdout, "    data csc32: 0x%08lx\n", crc);
		}

	}//end toc file while
	
	//TODO check stderrs
	if(bytes_read != 0) {
		fprintf(stderr, "Corrupt archive (bad header)\n");
		exit(BAD_TAG);
	}

	if (archive_fd != STDIN_FILENO) {
		close(archive_fd);
	}

	return;
}

//validates that files in an archive are correctly formatted and their crc is valid
void validate_archive(void){
	//outrageous variable list partie trois
	int archive_fd;
	arvik_header_t header;
	arvik_footer_t footer;
	char tag_buf[sizeof(ARVIK_TAG)];
	char filename[sizeof(header.arvik_name)];
	char buf[BUF];
	char temp[16];
	long size;
	ssize_t bytes_read, chunk, remaining;
	char pad;
	char *slash = NULL;
	//for calcing CRC's
	char expected_crc[sizeof(footer.arvik_data_crc) +1 ];
	char actual_crc[sizeof(footer.arvik_data_crc) + 1];
	uLong crc;

	fprintf(stderr, "Placeholder validate_archive\n");

	//open archive
	if (archive_filename != NULL) {

		archive_fd = open(archive_filename, O_RDONLY);
		if (archive_fd < 0) {
			perror("open archive file");
			exit(READ_FAIL);
		}
	}
	else {
		archive_fd = STDIN_FILENO;
	}

	//read and verify arvik of archive itself
	bytes_read = read(archive_fd, tag_buf, strlen(ARVIK_TAG));
	fprintf(stderr, "\nbytes_read is: %ld\ntag_buf is: %s\n", bytes_read, tag_buf);
	if (bytes_read != (ssize_t)strlen(ARVIK_TAG) || strncmp(tag_buf, ARVIK_TAG, strlen(ARVIK_TAG)) != 0) {
		exit(BAD_TAG);
	}
	
	//process archive member files
	while ((bytes_read = read(archive_fd, &header, sizeof(header))) == sizeof(header)) {
	
		// Get filename for error messages
		memset(filename, 0, sizeof(filename));
		memcpy(filename, header.arvik_name, sizeof(header.arvik_name));
		slash = strchr(filename, ARVIK_NAME_TERM);
		if (slash) { 
			*slash = '\0';
		}
		filename[sizeof(filename) - 1] = '\0';	

		//parse file size
		memcpy(temp, header.arvik_size, sizeof(header.arvik_size));
		temp[sizeof(header.arvik_size)] = '\0';
		size = strtol(temp, NULL, 10);

		//CRC compute
		crc = crc32(0L, Z_NULL, 0);
		remaining = size;

		//read file and calculate crc32 from each loop
		while(remaining > 0) {
			chunk = (remaining > BUF) ? BUF : remaining;
			if (read(archive_fd, buf, chunk) != chunk) {
				fprintf(stderr, "Error reading file data for %s\n", filename);
				exit(READ_FAIL);
			}
			crc = crc32(crc, (unsigned char *)buf, chunk);
			remaining -= chunk;
		}

		//skip padding if needed
		if (size % 2 == 1) {
			if (read(archive_fd, &pad, 1) != 1) {
				exit(READ_FAIL);
			}
		}

		//read footer
		if (read(archive_fd, &footer, sizeof(footer)) != sizeof(footer)) {
			fprintf(stderr, "Could not read footer for %s\n", filename);
			exit(READ_FAIL);
		}

		//compare CRCs
		snprintf(expected_crc, sizeof(expected_crc), "0x%08lx", crc);
		memcpy(actual_crc, footer.arvik_data_crc, sizeof(footer.arvik_data_crc));
		actual_crc[sizeof(footer.arvik_data_crc)] = '\0';

		if (strncmp(expected_crc, actual_crc, sizeof(footer.arvik_data_crc)) != 0) {
			fprintf(stderr, "CRC mismatch for %s\n", filename);
			exit(CRC_DATA_ERROR);
		}

		if (verbose) {
		    fprintf(stderr, "Validated %s (CRC OK)\n", filename);
		}

	}//end file while

	if (bytes_read != 0) {
		fprintf(stderr, "Corrupt archive with incomplete header\n");
		exit(READ_FAIL);
	}

	if (archive_fd != STDIN_FILENO) {
		close(archive_fd);
	}
	return;
}
