#ifndef SERIAL_H_
#define SERIAL_H_

struct serial {
	//void* file;
    void (*fHandler) (struct serial *serial, char c);
    struct udp_pcb *fUpcb;
};

extern struct serial *serial_init(void);
extern int serial_send(struct serial *serial, char *data, int len);
extern int serial_register_handler(struct serial *serial, void (*handler) (struct serial *serial, char c));


#endif /* SERIAL_H_ */
