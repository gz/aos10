/**
 * Benchmark program
 * =================
 *
 * Console program that executes benchmarking of nfs read and write functions.
 *
 */

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

/**
 * Processes the whole buffer once with the provided io function.
 * Uses highest request size for speed.
 *
 * @param io_function function pointer to desired io function (read/write)
 *
 */
static void warmup(benchmark_function_ptr io_function) {

	PRINT_VERBOSE("Warmup phase ");

	fildes_t fd = open(BENCHMARK_FILENAME, O_RDWR);
	int num_processed = 0;
	int dot_count = 0;
	for (char* bufptr = buffer; bufptr < buffer+buffer_size; bufptr += BENCHMARK_MAXREQSIZE) {
		num_processed += io_function(fd, bufptr, BENCHMARK_MAXREQSIZE);
		dot_count++;
		if (dot_count % 16 == 0) PRINT_VERBOSE(". ");
	}
	close(fd);

	PRINT_VERBOSE("\n");
}


/**
 * Prints results of a bandwidth measurement.
 * If BENCHMARK_VERBOSE is defined in benchmark.h, nice readable output is generated,
 * else only request size and bandwidth are printed (ready for plotting).
 *
 * @param num_bytes number of bytes processed
 * @param buf_size number of bytes that were supposed to be processed in the current test
 * @param req_size request or chunk size in bytes, how many data is sent with each io function call
 * @param time_us duration time of the current test in microseconds
 *
 */
static void print_result(unsigned int num_bytes, unsigned int buf_size, unsigned int req_size, uint64_t time_us) {

	// calculate
	unsigned int time_ms = (unsigned int)(time_us / 1000);
	unsigned int time_s = (unsigned int)(time_us / 1000000);
	unsigned int time_ms_part = time_ms - (time_s * 1000);
	unsigned int speed = (unsigned int)((uint64_t)num_bytes * 1000000 / time_us);
	unsigned int speed_part = (unsigned int)((uint64_t)num_bytes * 1000000000 / time_us) - (speed * 1000);

	// output
	PRINT_VERBOSE("\nProcessed %d of %d bytes, in chunks of size %d bytes, in %d.%d s.\n", num_bytes, buf_size, req_size, time_s, time_ms_part);
	time_ms_part--; // hack because of unused warning
	PRINT_VERBOSE("Exact time in microseconds: %llu us\n", time_us);
	PRINT_VERBOSE("Bandwidth: %d.%03d bytes/s\n", speed, speed_part);
	printf("%d %d.%03d\n", req_size, speed, speed_part);

}

/**
 * Performs one measurement for each power of two request size between
 * BENCHMARK_MINREQSIZE and BENCHMARK_MAXREQSIZE.
 * For each request size, the io function gets called BENCHMARK_CALLS times
 * and the time is measured over all the BENCHMARK_CALLS calls.
 * Results are printed after each measurement.
 *
 * @param io_function function pointer to desired io function (read/write)
 *
 */
static void measure(benchmark_function_ptr io_function) {
	// write the file several times while using different request sizes
	for (unsigned int req_size = BENCHMARK_MINREQSIZE; req_size <= BENCHMARK_MAXREQSIZE; req_size <<= 1) {

		// open file read-write mode
		fildes_t fd = open(BENCHMARK_FILENAME, O_RDWR);
		unsigned int num_processed = 0;
		char* bufptr = buffer;

		// do the measuring
		uint64_t start = time_stamp();
		for (int i=0; i < BENCHMARK_CALLS; i++) {
			num_processed += io_function(fd, bufptr, req_size);
			bufptr += req_size;
		}
		uint64_t end = time_stamp();
		uint64_t time_us = end-start;

		// close file and print result
		close(fd);
		print_result(num_processed, BENCHMARK_CALLS*req_size, req_size, time_us);
	}
}

/**
 * Entry point of benchmark program.
 * Allocates/frees buffer and calls the warmups and measurement functions
 * BENCHMARK_REPETITIONS times.
 *
 * @param argc number of command line arguments (ignored)
 * @param argv array of command line arguments (ignored)
 *
 */
int benchmark(int argc, char **argv) {
	// allocate a buffer to write files from and read files into
	buffer_size = BENCHMARK_MAXREQSIZE*BENCHMARK_CALLS;
	buffer = malloc(buffer_size);
	memset(buffer,'x',buffer_size);
	PRINT_VERBOSE("Buffer of size %d bytes created.\n", buffer_size);
	// repeat benchmark test several times
	for(int z=0; z<BENCHMARK_REPETITIONS; z++) {
		printf("\n-- Benchmarking WRITE --\n\n");
		warmup((benchmark_function_ptr)&write);
		measure((benchmark_function_ptr)&write);
		printf("\n-- Benchmarking READ --\n\n");
		warmup(&read);
		measure(&read);
	}
	// free buffer
	free(buffer);
	return 0;
}
