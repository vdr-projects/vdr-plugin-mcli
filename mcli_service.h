#ifndef MCLI_SERVICE_H
#define MCLI_SERVICE_H

#define MAX_TUNERS_IN_MENU 16

typedef struct
{
	int type[MAX_TUNERS_IN_MENU];
	char name[MAX_TUNERS_IN_MENU][128];
    int preference[MAX_TUNERS_IN_MENU];
} mclituner_info_t;


/**
 * struct used to get and set the number of tuner used by mcli
 * negative value indicates : use all available tuners
 */
typedef struct 
{
    int dvb_s;
    int dvb_s2;
    int dvb_t;
    int dvb_c;
} mcli_tuner_count_t;
#endif
