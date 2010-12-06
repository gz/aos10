#include <assert.h>
#include <sos.h>

static char elf_buffer[4096*10];

int main(void) {
	int fd = open("sosh", O_RDONLY);
	int bytes_read = 0;

	data_ptr copy_to = &elf_buffer;

	while( (written = read(fd, copy_to, 127)) ) {
		copy_to += written;
	}
	elf_loadFile(&elf_buffer, 0);

	L4_Start_SpIp(L4_Myself(), 0xC0000000, 0x1000);
}
