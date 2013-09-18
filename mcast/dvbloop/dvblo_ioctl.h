/* dvbloop - A DVB Loopback Device
 * Copyright (C) 2006 Christian Praehauser, Deti Fliegl
 -----------------------------------------
 * File: dvblo_char.h
 * Desc: Char device support for dvblo
 * Date: October 2006
 * Author: Christian Praehauser <cpreahaus@cosy.sbg.ac.at>, Deti Fliegl <deti@fliegl.de>
 *
 * This file is released under the GPLv2.
 */

#ifndef _DVBLO_IOCTL_H_
#define _DVBLO_IOCTL_H_

#ifndef WIN32
#include <linux/ioctl.h>
#endif
/**
 * Maximum number of devices
 */
#define DVBLO_IOC_MAGIC	'd'
#define PRIV_DATA_SIZE	4096
typedef struct
{
  u_int16_t pid[256];
  int num;
} dvblo_pids_t;
typedef struct dvblo_sec
{
	struct dvb_diseqc_master_cmd diseqc_cmd;
	fe_sec_mini_cmd_t mini_cmd;
	fe_sec_tone_mode_t tone_mode;
	fe_sec_voltage_t voltage;
} dvblo_sec_t;
typedef struct dvblo_festatus
{
	fe_status_t st;
	u_int32_t ber;
	u_int16_t strength;
	u_int16_t snr;
	u_int32_t ucblocks;
} dvblo_festatus_t;
typedef unsigned char dvblo_private_t[PRIV_DATA_SIZE];

#define CA_MAX_SLOTS 16
typedef struct {
  ca_caps_t cap;
  ca_slot_info_t info[CA_MAX_SLOTS];
} dvblo_cacaps_t;

#define CA_TPDU_MAX 2048
typedef struct {
  u_int16_t len;
  u_int8_t data[CA_TPDU_MAX]; 
} dvblo_tpdu_t;

#define EV_MASK_FE	0x0000000f
#define EV_MASK_PID	0x000000f0
#define EV_MASK_SEC	0x00000f00
#define EV_MASK_PRIV	0x0000f000
#define EV_MASK_CA	0x000f0000

#define EV_FRONTEND	0x00000001
#define EV_TUNER	0x00000002
#define EV_FREQUENCY	0x00000004
#define EV_BANDWIDTH	0x00000008

#define EV_PIDFILTER	0x00000010

#define EV_TONE		0x00000100
#define EV_VOLTAGE	0x00000200
#define EV_DISEC_MSG	0x00000400
#define EV_DISEC_BURST	0x00000800

#define EV_PRIV_READ	0x00001000
#define EV_PRIV_WRITE	0x00002000

#define EV_CA_RESET	0x00010000
#define EV_CA_WRITE	0x00020000
#define EV_CA_PID	0x00040000
#define EV_CA_DESCR	0x00080000

struct dvblo_ioc_dev
{

	/// The MAC address of the virtual DVB adapter
	u_int8_t mac[6];

	/**
	 * This is set to the number of the new device when ioctl(DVBLO_IOCADDDEV)
	 * was successful.
	 * @note This corresponds to the minor device number.
	 */
	int num;
};

/**
 * @brief Add a new DVBLoop adapter device
 */
#define DVBLO_IOCADDDEV		_IO(DVBLO_IOC_MAGIC, 1)
/**
 * @brief Remove the DVBLoop adapter device with the specified number
 */
#define DVBLO_IOCDELDEV		_IO(DVBLO_IOC_MAGIC, 2)
/**
 * @brief Check if DVBLoop adapter has a corresponding dvb device
 */
#define DVBLO_IOCCHECKDEV	_IO(DVBLO_IOC_MAGIC, 30)
/**
 * @brief Get event mask
 */
#define DVBLO_GET_EVENT_MASK	_IOR(DVBLO_IOC_MAGIC, 3, unsigned int)
/**
 * @brief Get FE parameters
 */
#define DVBLO_GET_FRONTEND_PARAMETERS	_IOR(DVBLO_IOC_MAGIC, 4, struct dvb_frontend_parameters)
/**
 * @brief Set FE parameters
 */
#define DVBLO_SET_FRONTEND_PARAMETERS	_IOW(DVBLO_IOC_MAGIC, 4, struct dvb_frontend_parameters)
/**
 * @brief Get tuner parameters
 */
#define DVBLO_GET_TUNER_PARAMETERS	_IOR(DVBLO_IOC_MAGIC, 5, struct dvb_frontend_parameters)
/**
 * @brief Set tuner parameters
 */
#define DVBLO_SET_TUNER_PARAMETERS	_IOW(DVBLO_IOC_MAGIC, 5, struct dvb_frontend_parameters)
/**
 * @brief Get SEC parameters
 */
#define DVBLO_GET_SEC_PARAMETERS	_IOR(DVBLO_IOC_MAGIC, 6, struct dvblo_sec)
/**
 * @brief Get SEC parameters
 */
#define DVBLO_SET_SEC_PARAMETERS	_IOW(DVBLO_IOC_MAGIC, 6, struct dvblo_sec)
/**
 * @brief Set FE-Status parameters
 */
#define DVBLO_GET_FRONTEND_STATUS	_IOR(DVBLO_IOC_MAGIC, 7, struct dvblo_festatus)
/**
 * @brief Set Tuner-Status parameters
 */
#define DVBLO_SET_FRONTEND_STATUS	_IOW(DVBLO_IOC_MAGIC, 7, struct dvblo_festatus)
/**
 * @brief Get Tuner-Status parameters
 */
#define DVBLO_GET_TUNER_STATUS		_IOR(DVBLO_IOC_MAGIC, 8, u_int32_t)
/**
 * @brief Set Tuner-Status parameters
 */
#define DVBLO_SET_TUNER_STATUS		_IOW(DVBLO_IOC_MAGIC, 8, u_int32_t)
/**
 * @brief Set FE-Info 
 */
#define DVBLO_GET_FRONTEND_INFO		_IOR(DVBLO_IOC_MAGIC, 9, struct dvb_frontend_info)
/**
 * @brief Set FE-Info 
 */
#define DVBLO_SET_FRONTEND_INFO		_IOW(DVBLO_IOC_MAGIC, 9, struct dvb_frontend_info)

#ifndef WIN32 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
/**
 * @brief Set Tuner-Info 
 */
#define DVBLO_GET_TUNER_INFO		_IOR(DVBLO_IOC_MAGIC, 10, struct dvb_tuner_info)
/**
 * @brief Set Tuner-Info 
 */
#define DVBLO_SET_TUNER_INFO		_IOW(DVBLO_IOC_MAGIC, 10, struct dvb_tuner_info)
#endif /*  */
/**
 * @brief Get list of PIDS
 */
#define DVBLO_GET_PIDLIST		_IOR(DVBLO_IOC_MAGIC, 20, dvblo_pids_t)
/**
 * @brief Pass through of private data
 */
#define DVBLO_GET_PRIVATE	_IOR(DVBLO_IOC_MAGIC, 40, dvblo_private_t)
/**
 * @brief Pass through of private data
 */
#define DVBLO_SET_PRIVATE	_IOW(DVBLO_IOC_MAGIC, 40, dvblo_private_t)
/**
 * @brief Get CA_CAPS including slot_info
 */
#define DVBLO_GET_CA_CAPS	_IOR(DVBLO_IOC_MAGIC, 80, dvblo_cacaps_t)
/**
 * @brief Set CA_CAPS including slot_info
 */
#define DVBLO_SET_CA_CAPS	_IOW(DVBLO_IOC_MAGIC, 80, dvblo_cacaps_t)
/**
 * @brief Get TPDU
 */
#define DVBLO_GET_TPDU		_IOR(DVBLO_IOC_MAGIC, 81, dvblo_tpdu_t)
/**
 * @brief Send TPDU
 */
#define DVBLO_SET_TPDU		_IOW(DVBLO_IOC_MAGIC, 81, dvblo_tpdu_t)

#endif /* _DVBLO_IOCTL_H_ */
#endif
