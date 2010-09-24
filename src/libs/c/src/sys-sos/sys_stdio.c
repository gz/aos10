#include <stdio.h>

// Must be provided by the userland libraary somewhere
extern size_t
sos_write(const void *data, long int position, size_t count, void *handle);
extern size_t
sos_read(void *data, long int position, size_t count, void *handle);

struct __file __stdin = {
	NULL,
	sos_read,
	NULL,
	NULL,
	NULL,
	_IONBF,
	NULL,
	0,
	0
};


struct __file __stdout = {
	NULL,
	NULL,
	sos_write,
	NULL,
	NULL,
	_IONBF,
	NULL,
	0,
	0
};


struct __file __stderr = {
	NULL,
	NULL,
	sos_write,
	NULL,
	NULL,
	_IONBF,
	NULL,
	0,
	0
};

FILE *stdin = &__stdin;
FILE *stdout = &__stdout;
FILE *stderr = &__stderr;
