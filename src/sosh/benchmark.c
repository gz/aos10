#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/* Your OS header file */
#include <sos.h>

#include "benchmark.h"

static int buffer_size;
static char* buffer;

typedef int(*benchmark_function_ptr)(fildes_t, char*, size_t);

static void warmup(benchmark_function_ptr io_function) {

	PRINT_VERBOSE("Warmup phase\n");

	fildes_t fd = open(BENCHMARK_FILENAME, O_RDWR);
	int num_written = 0;
	for (char* bufptr = buffer; bufptr < buffer+buffer_size; bufptr += BENCHMARK_MAXREQSIZE) {
		num_written += io_function(fd, bufptr, BENCHMARK_MAXREQSIZE);
		PRINT_VERBOSE(". ");
	}
	close(fd);

	PRINT_VERBOSE("\n");
}


static void print_result(int num_written, unsigned int req_size, uint64_t time_us) {

	// calculate
	unsigned int time_ms = (unsigned int)(time_us / 1000);
	unsigned int time_s = (unsigned int)(time_us / 1000000);
	unsigned int time_ms_part = time_ms - (time_s * 1000);
	unsigned int speed = (unsigned int)((uint64_t)buffer_size * 1000000 / time_us);
	unsigned int speed_part = (unsigned int)((uint64_t)buffer_size * 1000000000 / time_us) - (speed * 1000);

	// output
	PRINT_VERBOSE("Written %d of %d bytes, in chunks of size %d bytes, in %d.%d s.\n", num_written, buffer_size, req_size, time_s, time_ms_part);
	time_ms_part--; // hack because of unused warning
	PRINT_VERBOSE("Exact time in microseconds: %llu us\n", time_us);
	PRINT_VERBOSE("Writing speed: %d.%d bytes/s\n", speed, speed_part);
	printf("%d %d.%d\n", req_size, speed, speed_part);

}

static void measure(benchmark_function_ptr io_function) {
	// write the file several times while using different request sizes
	for (unsigned int req_size = BENCHMARK_MINREQSIZE; req_size <= BENCHMARK_MAXREQSIZE; req_size <<= 1) {

		// open file to write, write the whole buffer, close the file
		fildes_t fd = open(BENCHMARK_FILENAME, FM_WRITE);
		int num_written = 0;
		uint64_t time_us = 0ULL;
		char* bufptr = buffer;

		uint64_t start = time_stamp();
		for (int i=0; i < BENCHMARK_CALLS; i++) {
			num_written += io_function(fd, bufptr, req_size);
			bufptr += req_size;
		}
		uint64_t end = time_stamp();
		time_us += end-start;

		close(fd);

		print_result(num_written, req_size, time_us);
	}
}

int benchmark(int argc, char **argv) {

	for(int z=0; z<BENCHMARK_REPETITIONS; z++) {

		// Allocate a buffer to write files from and read files into
		buffer_size = BENCHMARK_MAXREQSIZE*BENCHMARK_CALLS;
		buffer = malloc(buffer_size);
		memset(buffer,'x',buffer_size);
		PRINT_VERBOSE("Buffer of size %d bytes created.\n", buffer_size);

		printf("Benchmarking Write\n");
		warmup((benchmark_function_ptr) &write);
		measure((benchmark_function_ptr) &write);


		printf("Benchmarking Read\n");
		warmup(&read);
		measure(&read);

		// free buffer
		free(buffer);
	}

	return 0;
}
