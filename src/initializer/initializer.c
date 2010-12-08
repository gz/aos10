#include <sos.h>
#include <elf/elf.h>

static char elf_buffer[4096*35];

int main(void) {
	char filename[N_NAME];
	process_get_name(filename);

	printf("loading: %s\n", filename);

	int fd = open(filename, O_RDONLY);
	int bytes_read = 0;

	data_ptr copy_to = elf_buffer;

	printf("copy some file\n");
	while( (bytes_read = read(fd, copy_to, 512)) ) {
		copy_to += bytes_read;
	}

	printf("starting elf_loadFile\n");

	elf_loadFile(elf_buffer, 0);
	printf("end elf_loadFile enter at:0x%llX\n", elf_getEntryPoint(elf_buffer));

	process_start();

	return 0;
}

