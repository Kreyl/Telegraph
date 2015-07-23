/*
 * evt_mask.h
 *
 *  Created on: Apr 12, 2013
 *      Author: g.kruglov
 */

#ifndef EVT_MASK_H_
#define EVT_MASK_H_

// Event masks
#define EVTMSK_NO_MASK          0

#define EVTMSK_UART_NEW_CMD     EVENT_MASK(1)
// Indication
// Sensors
// USB
#define EVTMSK_USB_CONNECTED    EVENT_MASK(21)
#define EVTMSK_USB_DISCONNECTED EVENT_MASK(22)
#define EVTMSK_USB_READY        EVENT_MASK(23)
#define EVTMSK_USB_DATA_OUT     EVENT_MASK(24)

#endif /* EVT_MASK_H_ */
