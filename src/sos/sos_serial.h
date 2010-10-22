/*
 * sos_serial.h
 * Functions for communicating over the network using the serial protocol.
 */

#ifndef SOS_SERIAL_H_
#define SOS_SERIAL_H_

#include <l4/message.h>

void sos_serial_send(L4_Msg_t* msg_p);
//void sos_serial_recieve(L4_Msg_t* msg_p, int* send_p);


#endif /* SOS_SERIAL_H_ */
