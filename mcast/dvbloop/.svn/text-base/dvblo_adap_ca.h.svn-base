/* dvbloop - A DVB Loopback Device
 * Copyright (C) 2006 Christian Praehauser, Deti Flieg
 -----------------------------------------
 * File: dvblo_adap.c
 * Desc: Support for virtual DVB adapters - Frontend implementation
 * Date: October 2006
 * Author: Christian Praehauser <cpreahaus@cosy.sbg.ac.at>, Deti Fliegl <deti@fliegl.de>
 *
 * This file is released under the GPLv2.
 */

#ifndef _DVBLO_ADAP_CA_H_
#define _DVBLO_ADAP_CA_H_

#include "dvb-core/dvbdev.h"
#include "dvb-core/dvb_demux.h"
#include "dvb-core/dmxdev.h"
#include "dvb-core/dvb_net.h"
#include "dvb-core/dvb_frontend.h"
#include "dvb-core/dvb_ringbuffer.h"
#include "linux/dvb/ca.h"

void ci_get_data(struct dvb_ringbuffer *cibuf, u8 *data, int len);

/**
 * Register new ca device
 */
int dvblo_ca_register(struct dvblo *dvblo);
/**
 * Unregister ca device
 */
void dvblo_ca_unregister(struct dvblo *dvblo);
/**
 * Initialize ca device 
 */
int dvblo_ca_init(struct dvblo* dvblo);
/**
 * Uninitialize ca device 
 */
void dvblo_ca_exit(struct dvblo* dvblo);


#endif /* _DVBLO_ADAP_FE_H_ */
