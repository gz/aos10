#include <sos.h>
#include <elf/elf.h>

#define HEAP_START 0x40000000

//static char elf_buffer[4096*35];

int main(void) {
	char filename[N_NAME];
	process_get_name(filename);

	int fd = open(filename, O_RDONLY);
	int bytes_read = 0;

	// copy the file into the heap memory (directly, without malloc)
	// this is legitimate since process_start initializes the heap again
	// and there the buffer is not used anymore
	// by doing it this way we don't need any file size buffer size checks
	// because the max data size we can load is limited by the heap size
	data_ptr elf_buffer = (data_ptr)HEAP_START;
	data_ptr copy_to = elf_buffer;
	while( (bytes_read = read(fd, copy_to, 512)) ) {
		copy_to += bytes_read;
	}
	int ret = elf_loadFile(elf_buffer, 0);

	process_start(ret);
	return 0;
}

