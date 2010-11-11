#ifndef BENCHMARK_H_
#define BENCHMARK_H_

/* Benchmark Settings */
#define BENCHMARK_REPETITIONS	5
#define BENCHMARK_CALLS   		512
#define BENCHMARK_MAXREQSIZE	(1 << 9)
#define BENCHMARK_MINREQSIZE	(1 << 4)
#define BENCHMARK_FILENAME		"benchmark"

/* Benchmark debug print */
//#define BENCHMARK_VERBOSE

#ifdef BENCHMARK_VERBOSE
	#define PRINT_VERBOSE(x...) printf( x )
#else
	#define PRINT_VERBOSE(x...) /* do nothing */
#endif

// the program
int benchmark(int, char**);

#endif /* BENCHMARK_H_ */
