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

#ifndef _DVBLO_ADAP_FE_H_
#define _DVBLO_ADAP_FE_H_

#include "dvb-core/dvbdev.h"
#include "dvb-core/dvb_demux.h"
#include "dvb-core/dmxdev.h"
#include "dvb-core/dvb_net.h"
#include "dvb-core/dvb_frontend.h"
extern struct dvb_frontend_ops dvblo_adap_fe_ops;
int dvblo_fe_get_info (struct dvblo *dvblo, struct dvb_frontend_info *info);
int dvblo_fe_set_info (struct dvblo *dvblo, struct dvb_frontend_info *info);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
int dvblo_fe_get_tunerinfo (struct dvblo *dvblo, struct dvb_tuner_info *tunerinfo);
int dvblo_fe_set_tunerinfo (struct dvblo *dvblo, struct dvb_tuner_info *tunerinfo);

#endif /*  */

#endif /* _DVBLO_ADAP_FE_H_ */
