/*
 * sos_serial.h
 * Functions for communicating over the network using the serial protocol.
 */

#ifndef SOS_SERIAL_H_
#define SOS_SERIAL_H_

#include <l4/message.h>

struct serial* ser;

//void sos_serial_init(void);
//int sos_serial_send(int, char*);
void sos_serial_receive (struct serial *serial, char c);
void sos_serial_read(L4_Msg_t* msg_p);

void sos_serial_test(L4_Msg_t*, L4_Word_t*);


#endif /* SOS_SERIAL_H_ */
