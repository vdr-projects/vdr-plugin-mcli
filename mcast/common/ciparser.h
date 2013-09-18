/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#define ntohs16(data) ((u_int16_t)data[0]<<8 | (u_int16_t)data[1])
typedef struct {
	u_int16_t len;
	u_int8_t *data;
} ci_pdu_t;

typedef struct
{
	u_int8_t slot;
	u_int8_t tcid;
	u_int32_t length;
	u_int8_t *data;
} ci_ll_t;

typedef struct
{
	ci_ll_t *ll;
	u_int8_t c_tpdu_tag;
	u_int32_t length;
	u_int8_t tcid;
	u_int8_t *data;
} ci_tl_t;

typedef struct
{
	ci_tl_t *tl;
	u_int8_t tag;
	u_int32_t length;
	u_int32_t object_value;
	u_int8_t *data;
} ci_sl_t;

typedef struct
{
	ci_sl_t *sl;
	u_int32_t tag;
	u_int32_t length;
	u_int8_t *data;
} ci_al_t;

typedef struct
{
	u_int16_t ca_id;
	u_int16_t ca_pid;
} ca_desc_t;

typedef struct
{
	u_int8_t stream_type;
	u_int16_t pid;
	u_int16_t es_info_length;
	u_int8_t ca_pmt_cmd_id;
	ca_desc_t *cadescr;
} pidinfo_t;

typedef struct
{
	u_int8_t ca_pmt_list_management;
	u_int16_t program_number;
	u_int8_t version_number;
	u_int8_t current_next;
	u_int16_t program_info_length;
	u_int8_t ca_pmt_cmd_id;
	ca_desc_t *cadescr;
	pidinfo_t *pidinfo;
} ca_pmt_t;

typedef struct
{
	u_int16_t pid;
	u_int8_t ca_enable;
} pid_ca_enable_t;

typedef struct
{
	u_int16_t program_number;
	u_int8_t version_number;
	u_int8_t current_next;
	u_int8_t ca_enable;
	pid_ca_enable_t *pidcaenable;
} ca_pmt_reply_t;


#define LENGTH_SIZE_INDICATOR 0x80

#define CPLM_MORE    0x00
#define CPLM_FIRST   0x01
#define CPLM_LAST    0x02
#define CPLM_ONLY    0x03
#define CPLM_ADD     0x04
#define CPLM_UPDATE  0x05
  
#define CPCI_OK_DESCRAMBLING  0x01
#define CPCI_OK_MMI           0x02
#define CPCI_QUERY            0x03
#define CPCI_NOT_SELECTED     0x04

#define AOT_CA_INFO_ENQ             0x9F8030
#define AOT_CA_INFO                 0x9F8031
#define AOT_CA_PMT                  0x9F8032
#define AOT_CA_PMT_REPLY            0x9F8033

#define ST_SESSION_NUMBER           0x90
#define ST_OPEN_SESSION_REQUEST     0x91
#define ST_OPEN_SESSION_RESPONSE    0x92
#define ST_CREATE_SESSION           0x93
#define ST_CREATE_SESSION_RESPONSE  0x94
#define ST_CLOSE_SESSION_REQUEST    0x95
#define ST_CLOSE_SESSION_RESPONSE   0x96

#define DATA_INDICATOR 0x80

#define T_SB           0x80
#define T_RCV          0x81
#define T_CREATE_TC    0x82
#define T_CTC_REPLY    0x83
#define T_DELETE_TC    0x84
#define T_DTC_REPLY    0x85
#define T_REQUEST_TC   0x86
#define T_NEW_TC       0x87
#define T_TC_ERROR     0x88
#define T_DATA_LAST    0xA0
#define T_DATA_MORE    0xA1


DLL_SYMBOL int ci_cpl_find_caid_by_pid (int pid);
DLL_SYMBOL int ci_cpl_find_slot_by_caid_and_pid (int caid, int pid);
DLL_SYMBOL int ci_cpl_clear (int slot);
DLL_SYMBOL int ci_cpl_clear_pids (int slot);
DLL_SYMBOL int ci_cpl_clear_caids (int slot);
DLL_SYMBOL int ci_cpl_clear_capids (int slot);
DLL_SYMBOL int ci_decode_ll (uint8_t * data, int len);
