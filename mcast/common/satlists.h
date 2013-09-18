/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#define DISEQC_MAX_EXTRA 8
#define MAX_EXTRA_DATA 16

typedef enum
{
//the defines for circular polarisation are taken from official DiSEqC-Spec at
//http://www.eutelsat.com/satellites/pdf/Diseqc/Reference%20docs/bus_spec.pdf
	POL_V = SEC_VOLTAGE_13,
	POL_H = SEC_VOLTAGE_18,
	POL_R = SEC_VOLTAGE_13,
	POL_L = SEC_VOLTAGE_18,
} polarisation_t;

typedef struct
{
	int magic;
	int version;
	
	polarisation_t Polarisation;	// H/V/L/R
	int RangeMin;		// 11700
	int RangeMax;		// 12750

// SEC Settings to be used for the specification above
	int LOF;		// 9750
	recv_sec_t sec;
	struct dvb_diseqc_master_cmd diseqc_cmd[DISEQC_MAX_EXTRA];
	int diseqc_cmd_num;
} satellite_component_t;

typedef enum
{
	SAT_SRC_LNB=0,
	SAT_SRC_ROTOR=1,
	SAT_SRC_UNI=2,       // !!! match DISEQC_* values in dvb_server.h !!!
} satellite_source_t;

typedef struct
{
	int magic;
	int version;
	
// Specification of satellite parameters
	char Name[UUID_SIZE];	// Astra 19,2
	int SatPos;		// 1920
	int SatPosMin;		// Only used for SAT_SRC_ROTOR
	int SatPosMax;		// Only used for SAT_SRC_ROTOR
	satellite_source_t type;	// see above

	satellite_component_t *comp;	// What to do for polarisation and range for SEC?
	int comp_num;		// Number of components
	int AutoFocus;
	int Latitude;
	int Longitude;
	int num_extra_data;
	int extra_data[MAX_EXTRA_DATA];  // reserved
} satellite_info_t;

typedef struct satellite_list
{
	int magic;
	int version;
	
	char Name[UUID_SIZE];	// Magic unique identifier
	satellite_info_t *sat;
	int sat_num;
} satellite_list_t;

typedef struct
{
	int magic;
	int version;
	
	int netceiver;
	int sat_list;
	int sat;
	int comp;
	int position;  // for rotor
} satellite_reference_t;

DLL_SYMBOL int satellite_find_by_diseqc (satellite_reference_t * ref, recv_sec_t *sec, struct dvb_frontend_parameters *fep, int mode);
DLL_SYMBOL int satellite_get_pos_by_ref (satellite_reference_t * ref);
DLL_SYMBOL int satellite_get_lof_by_ref (satellite_reference_t * ref);
DLL_SYMBOL polarisation_t satellite_find_pol_by_ref (satellite_reference_t * ref);
DLL_SYMBOL recv_sec_t *satellite_find_sec_by_ref (satellite_reference_t * ref);
