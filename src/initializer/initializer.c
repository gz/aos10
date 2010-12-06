#include <assert.h>
#include <sos.h>
#include <elf/elf.h>

static char elf_buffer[4096*27];

int main(void) {
	printf("haiu Im teh initializer\n");


	int fd = open("sosh", O_RDONLY);
	int bytes_read = 0;

	data_ptr copy_to = elf_buffer;

	printf("copy some filez\n");
	while( (bytes_read = read(fd, copy_to, 512)) ) {
		copy_to += bytes_read;
	}

	printf("starting elf_loadFile\n");

	elf_loadFile(elf_buffer, 0);
	printf("end elf_loadFile enter at:0x%llX\n", elf_getEntryPoint(elf_buffer));

	process_start();
	//L4_Start_SpIp(L4_Myself(), 0xC0000000, 0x1000);
}
