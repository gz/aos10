/* Simple shell to run on SOS */
/* 
 * Orignally written by Gernot Heiser 
 * - updated by Ben Leslie 2003  
 * - updated by Charles Gray 2006
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/* Your OS header file */
#include <sos.h>

#define BUF_SIZ   128
#define MAX_ARGS   32

static fildes_t in;
static stat_t sbuf;

static void prstat(const char *name) {
	/* print out stat buf */

	printf("%c%c%c%c 0x%06x 0x%lx 0x%06lx %s\n",
			sbuf.st_type == ST_SPECIAL ? 's' : '-',
			sbuf.st_fmode & FM_READ ? 'r' : '-', sbuf.st_fmode & FM_WRITE ? 'w'
					: '-', sbuf.st_fmode & FM_EXEC ? 'x' : '-', sbuf.st_size,
			sbuf.st_ctime, sbuf.st_atime, name);
}

static int cat(int argc, char **argv) {
	fildes_t fd;
	char buf[BUF_SIZ];
	int num_read, num_written = 0;

	if (argc != 2) {
		printf("Usage: cat filename\n");
		return 1;
	}

	printf("<%s>\n", argv[1]);

	fd = open(argv[1], FM_READ);

	assert(fd >= 0);

	while ((num_read = read(fd, buf, BUF_SIZ)) > 0)
		num_written = write(stdout_fd, buf, num_read);

	if (num_read == -1 || num_written == -1) {
		printf("error on write\n");
		return 1;
	}

	return 0;
}

static int cp(int argc, char **argv) {
	fildes_t fd, fd_out;
	char *file1, *file2;
	char buf[BUF_SIZ];
	int num_read, num_written = 0;

	if (argc != 3) {
		printf("Usage: cp from to %d\n", argc);
		return 1;
	}

	file1 = argv[1];
	file2 = argv[2];

	fd = open(file1, FM_READ);
	fd_out = open(file2, FM_WRITE);

	assert(fd >= 0);

	while ((num_read = read(fd, buf, BUF_SIZ)) > 0)
		num_written = write(fd_out, buf, num_read);

	if (num_read == -1 || num_written == -1) {
		printf("error on cp\n");
		return 1;
	}

	close(fd);
	close(fd_out);
	return 0;
}

#define MAX_PROCESSES 10

static int ps(int argc, char **argv) {

	process_t *process;
	int i, processes;

	process = malloc(MAX_PROCESSES * sizeof(*process));

	if (process == NULL) {
		printf("%s: out of memory\n", argv[0]);
		return 1;
	}

	processes = process_status(process, MAX_PROCESSES);

	printf("TID SIZE   STIME   CTIME COMMAND\n");

	for (i = 0; i < processes; i++) {
		printf("%3x %4x %7d %7d %s\n", process[i].pid, process[i].size,
				process[i].stime, process[i].ctime, process[i].command);
	}

	free(process);

	return 0;
}

static int exec(int argc, char **argv) {
	pid_t pid;
	int r;
	int bg = 0;

	if (argc < 2 || (argc > 2 && argv[2][0] != '&')) {
		printf("Usage: exec filename [&]\n");
		return 1;
	}

	if ((argc > 2) && (argv[2][0] == '&')) {
		bg = 1;
	}

	if (bg == 0) {
		r = close(in);
		assert(r == 0);
	}

	pid = process_create(argv[1]);
	if (pid >= 0) {
		printf("Child pid=%d\n", pid);
		if (bg == 0) {
			process_wait(pid);
		}
	} else {
		printf("Failed!\n");
	}
	if (bg == 0) {
		in = open("console", FM_READ);
		assert(in>=0);
	}
	return 0;
}


static int dir(int argc, char **argv) {
	int i = 0, r;
	char buf[BUF_SIZ];

	if (argc > 2) {
		printf("usage: %s [file]\n", argv[0]);
		return 1;
	}

	if (argc == 2) {
		r = stat(argv[1], &sbuf);
		if (r < 0) {
			printf("stat(%s) failed: %d\n", buf, r);
			return 0;
		}
		prstat(argv[1]);
		return 0;
	}

	while (1) {
		r = getdirent(i, buf, BUF_SIZ);
		if (r < 0) {
			printf("dirent(%d) failed: %d\n", i, r);
			break;
		} else if (!r) {
			break;
		}
#if 0
		printf("dirent(%d): \"%s\"\n", i, buf);
#endif
		r = stat(buf, &sbuf);
		if (r < 0) {
			printf("stat(%s) failed: %d\n", buf, r);
			break;
		}
		prstat(buf);
		i++;
	}
	return 0;
}

static int uptime(int argc, char **argv) {

	if (argc != 1) {
		printf("usage: %s\n", argv[0]);
		return 1;
	}


	uint64_t current = time_stamp();
	int in_seconds = current / 1000000;

	int seconds   = in_seconds % 60;
	int in_minutes = in_seconds / 60;
	int minutes   = in_minutes % 60;
	int in_hours   = in_minutes / 60;
	int hours     = in_hours % 24;
	int days      = in_hours / 24;

	printf("System running since: %d Days, %d Hours, %d Minutes, %d Seconds\n", days, hours, minutes, seconds);
	return 0;
}

#define BENCHMARK_FILESIZE		(1 << 13)
#define BENCHMARK_MAXREQSIZE	(1 << 10)
#define BENCHMARK_MINREQSIZE	(1 << 10)
#define BENCHMARK_FILENAME		"benchmark"

static int benchmark(int argc, char **argv) {
	// allocate a buffer to write files from and read files into
	int buf_size = BENCHMARK_FILESIZE*sizeof(char);
	char* buf = malloc(buf_size);
	memset(buf,'x',buf_size);
	printf("buffer of size %d bytes created.\n", buf_size);
	// write the file several times while using different request sizes
	for (unsigned int req_size = BENCHMARK_MINREQSIZE;
			req_size <= BENCHMARK_MAXREQSIZE; req_size <<= 1) {
		// get current time stamp before writing file
		uint64_t start = time_stamp();
		// open file to write, write the whole buffer, close the file
		fildes_t fd = open(BENCHMARK_FILENAME, FM_WRITE);
		int num_written = 0;
		for (char* bufptr = buf; bufptr < buf+buf_size; bufptr += req_size) {
			num_written += write(fd, bufptr, req_size);
		}
		close(fd);
		// get current time stamp after writing file
		uint64_t end = time_stamp();
		// print result
		double time = ((double)(end-start)) / 1000000.0;
		double speed = buf_size / time;
		printf("%d of %d bytes, in chunks of size %d bytes, have been written in %.3f s.\nWriting speed: %.3f bytes/s\n", num_written, buf_size, req_size, time, speed);
	}
	// read the previously written file several times while using different request sizes
	for (unsigned int req_size = BENCHMARK_MINREQSIZE;
			req_size <= BENCHMARK_MAXREQSIZE; req_size <<= 1) {
		// get current time stamp before writing file
		uint64_t start = time_stamp();
		// open file to write, write the whole buffer, close the file
		fildes_t fd = open(BENCHMARK_FILENAME, FM_READ);
		int num_read = 0;
		for (char* bufptr = buf; bufptr < buf+buf_size; bufptr += req_size) {
			num_read += read(fd, bufptr, req_size);
		}
		close(fd);
		// get current time stamp after writing file
		uint64_t end = time_stamp();
		// print result
		double time = ((double)(end-start)) / 1000000.0;
		double speed = buf_size / time;
		printf("%d of %d bytes, in chunks of size %d bytes, have been read in %.3f s.\nReading speed: %.3f bytes/s\n", num_read, buf_size, req_size, time, speed);
	}
	// free buffer
	free(buf);
	return 0;
}

static int wait(int argc, char **argv) {

	if (argc != 2) {
		printf("usage: %s <seconds>\n", argv[0]);
		return 1;
	}

	sleep(atoi(argv[1])*1000);
	return 0;
}



struct command {
	char *name;
	int (*command)(int argc, char **argv);
};

struct command commands[] = {
		{ "dir", dir },
		{ "ls", dir },
		{ "cat", cat },
		{ "cp", cp },
		{ "ps", ps },
		{ "exec", exec },
		{ "uptime", uptime },
		{ "wait", wait },
		{ "benchmark", benchmark }
};

int main(void) {
	char buf[BUF_SIZ];
	char *argv[MAX_ARGS];
	int i, r, done, found, new, argc;
	char *bp, *p;

	in = open("console", FM_READ);
	assert (in >= 0);

	bp = buf;
	done = 0;
	new = 1;

	printf("\n[SOS Starting]\n");

	while (!done) {
		if (new) {
			printf("$ ");
		}
		new = 0;
		found = 0;

		while (!found && !done) {
			r = read(in, bp, BUF_SIZ - 1 + buf - bp);
			if (r < 0) {
				printf("Console read failed!\n");
				done = 1;
				break;
			}
			bp[r] = 0; /* terminate */
			for (p = bp; p < bp + r; p++) {
				if (*p == '\03') { /* ^C */
					printf("^C\n");
					p = buf;
					new = 1;
					break;
				} else if (*p == '\04') { /* ^D */
					p++;
					found = 1;
				} else if (*p == '\010' || *p == 127) {
					/* ^H and BS and DEL */
					if (p > buf) {
						printf("\010 \010");
						p--;
						r--;
					}
					p--;
					r--;
				} else if (*p == '\n') { /* ^J */
					printf("%c", *p);
					*p = 0;
					found = p > buf;
					p = buf;
					new = 1;
					break;
				} else {
					printf("%c", *p);
				}
			}
			bp = p;
			if (bp == buf) {
				break;
			}
		}

		if (!found) {
			continue;
		}

		argc = 0;
		p = buf;

		while (*p != '\0') {
			/* Remove any leading spaces */
			while (*p == ' ')
				p++;
			if (*p == '\0')
				break;
			argv[argc++] = p; /* Start of the arg */
			while (*p != ' ' && *p != '\0') {
				p++;
			}

			if (*p == '\0')
				break;

			/* Null out first space */
			*p = '\0';
			p++;
		}

		if (argc == 0) {
			continue;
		}

		found = 0;

		for (i = 0; i < sizeof(commands) / sizeof(struct command); i++) {
			if (strcmp(argv[0], commands[i].name) == 0) {
				commands[i].command(argc, argv);
				found = 1;
				break;
			}
		}

		/* Didn't find a command */
		if (found == 0) {
			/* They might try to exec a program */
			if (stat(argv[0], &sbuf) != 0) {
				printf("Command \"%s\" not found\n", argv[0]);
			} else if (!(sbuf.st_fmode & FM_EXEC)) {
				printf("File \"%s\" not executable\n", argv[0]);
			} else {
				/* Execute the program */
				argc = 2;
				argv[1] = argv[0];
				argv[0] = "exec";
				exec(argc, argv);
			}
		}
	}
	printf("[SOS Exiting]\n");
}
