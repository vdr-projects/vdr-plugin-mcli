#include "dvbipstream.h"

typedef struct
{
	int userValue;
	int driverValue;
} tChannelParameterMap;

const tChannelParameterMap RolloffValues[] = {
	{0, 0},
	{35, 0},
	{25, 0},
	{20, 0},
	{-1}
};

const tChannelParameterMap InversionValues[] = {
	{0, INVERSION_OFF},
	{1, INVERSION_ON},
	{999, INVERSION_AUTO},
	{-1}
};

const tChannelParameterMap BandwidthValues[] = {
	{6, BANDWIDTH_6_MHZ},
	{7, BANDWIDTH_7_MHZ},
	{8, BANDWIDTH_8_MHZ},
	{999, BANDWIDTH_AUTO},
	{-1}
};

const tChannelParameterMap CoderateValues[] = {
	{0, FEC_NONE},
	{12, FEC_1_2},
	{23, FEC_2_3},
	{34, FEC_3_4},
	{45, FEC_4_5},
	{56, FEC_5_6},
	{67, FEC_6_7},
	{78, FEC_7_8},
	{89, FEC_8_9},
	{999, FEC_AUTO},
	{-1}
};

const tChannelParameterMap CoderateValuesS[] = {
	{0, FEC_NONE},
	{12, FEC_1_2},
	{23, FEC_2_3},
	{34, FEC_3_4},
	{45, FEC_4_5},
	{56, FEC_5_6},
	{67, FEC_6_7},
	{78, FEC_7_8},
	{89, FEC_8_9},
	{13, FEC_1_3},		//S2
	{14, FEC_1_4},		//S2
	{25, FEC_2_5},		//S2
	{35, FEC_3_5},		//S2
	{910, FEC_9_10},	//S2        
	{999, FEC_AUTO},
	{-1}
};


const tChannelParameterMap ModulationValues[] = {
	{0, QPSK},
	{16, QAM_16},
	{32, QAM_32},
	{64, QAM_64},
	{128, QAM_128},
	{256, QAM_256},
	{999, QAM_AUTO},
	{-1}
};

const tChannelParameterMap ModulationValuesS[] = {
	{2, QPSK},
	{4, QPSK},
	{5, PSK8},
	{42, QPSK_S2},		// S2
	{8, PSK8},
	{16, QAM_16},
	{32, QAM_32},
	{64, QAM_64},
	{128, QAM_128},
	{256, QAM_256},
	{999, QAM_AUTO},
	{-1}
};

const tChannelParameterMap TransmissionValues[] = {
	{2, TRANSMISSION_MODE_2K},
	{8, TRANSMISSION_MODE_8K},
	{999, TRANSMISSION_MODE_AUTO},
	{-1}
};

const tChannelParameterMap GuardValues[] = {
	{4, GUARD_INTERVAL_1_4},
	{8, GUARD_INTERVAL_1_8},
	{16, GUARD_INTERVAL_1_16},
	{32, GUARD_INTERVAL_1_32},
	{999, GUARD_INTERVAL_AUTO},
	{-1}
};

const tChannelParameterMap HierarchyValues[] = {
	{0, HIERARCHY_NONE},
	{1, HIERARCHY_1},
	{2, HIERARCHY_2},
	{4, HIERARCHY_4},
	{999, HIERARCHY_AUTO},
	{-1}
};

#ifdef WIN32
#define strtok_r mystrtok
char *strtok_r(char *s, const char *d, char **m)
{
	char *p;
	char *q;

	if (s)
		*m = s;

	p = *m;
	if (!p)
		return NULL;

	while (*p && strchr(d,*p))
		p++;

	if (*p == 0) {
		*m = NULL;
		return NULL;
	}

	q = p;
	while (*q && !strchr(d,*q))
		q++;

	if (!*q)
		*m = NULL;
	else {
		*q++ = 0;
		*m = q;
	}

	return p;
}
#endif

char *strreplace (char *s, char c1, char c2)
{
	if (s) {
		char *p = s;
		while (*p) {
			if (*p == c1)
				*p = c2;
			p++;
		}
	}
	return s;
}


int UserIndex (int Value, const tChannelParameterMap * Map)
{
	const tChannelParameterMap *map = Map;
	while (map && map->userValue != -1) {
		if (map->userValue == Value)
			return map - Map;
		map++;
	}
	return -1;
}

int DriverIndex (int Value, const tChannelParameterMap * Map)
{
	const tChannelParameterMap *map = Map;
	while (map && map->userValue != -1) {
		if (map->driverValue == Value)
			return map - Map;
		map++;
	}
	return -1;
}

int MapToDriver (int Value, const tChannelParameterMap * Map)
{
	int n = UserIndex (Value, Map);
	if (n >= 0)
		return Map[n].driverValue;
	return -1;
}

static const char *ParseParameter (const char *s, int *Value, 
	const tChannelParameterMap * Map)
{
	if (*++s) {
		char *p = NULL;
		int n;
		errno = 0;
		n = strtol (s, &p, 10);
		if (!errno && p != s) {
			*Value = MapToDriver (n, Map);
			if (*Value >= 0)
				return p;
		}
	}
	printf ("ERROR: invalid value for parameter '%c'\n", *(s - 1));
	return NULL;
}

bool StringToParameters (const char *s, channel_t * ch)
{
	int dummy;
	bool newformat = false;
	int deliverysystem = -1;
	const char * start = s;
		
	while (s && *s) {
		switch (toupper (*s)) {
		case 'B':
			s = ParseParameter (s, &ch->bandwidth, BandwidthValues);
			break;
		case 'C':
			s = ParseParameter (s, &ch->coderateH, CoderateValuesS);
			break;
		case 'D':
			s = ParseParameter (s, &ch->coderateL, CoderateValues);
			break;
		case 'O':
			newformat = true;
		case 'E':
			s = ParseParameter (s, &dummy, RolloffValues);
			break;
		case 'G':
			s = ParseParameter (s, &ch->guard, GuardValues);
			break;
		case 'H':
			ch->polarization = SEC_VOLTAGE_18;
			s++;
			break;
		case 'I':
			s = ParseParameter (s, &ch->inversion, InversionValues);
			break;
		case 'L':
			ch->polarization = SEC_VOLTAGE_18;
			s++;
			break;
		case 'M':
			s = ParseParameter (s, &ch->modulation, 
				ModulationValuesS);
			break;
		case 'R':
			ch->polarization = SEC_VOLTAGE_13;
			s++;
			break;
		case 'S':
			newformat = true;
			s++;
			if (*s == '1')
				deliverysystem = 1;
			else if (*s == '0')
				deliverysystem = 0;
			s++;
			break;
		case 'T':
			s = ParseParameter (s, &ch->transmission, 
				TransmissionValues);
			break;
		case 'V':
			ch->polarization = SEC_VOLTAGE_13;
			s++;
			break;
		case 'Y':
			s = ParseParameter (s, &ch->hierarchy, HierarchyValues);
			break;
		default:
			printf ("ERROR: unknown parameter key '%c' at pos %d\n",
				 *s, (int)((long)(s-start)));
			return 0;
		}
	}
	if (newformat)
	{
		//printf("Detected VDR-1.7.x parameter string format.\n");
		if (deliverysystem == 1 && ch->modulation == QPSK)
		{
			ch->modulation = QPSK_S2;
		}
	}
	return 1;
}

int SourceFromString (char *s)
{
	char t = 0, d = 0;
	float val = 0;

	sscanf (s, "%c%f%c", &t, &val, &d);
	if (t == 'S') {
		val = val * 10;
		if (d == 'W')
			val = 1800 - val;
		else
			val = val + 1800;
	} else if (t == 'T')
		val = -2;
	else if (t == 'C')
		val = -3;
	return (int) val;
}

int ParseLine (const char *s, channel_t * ch)
{
	bool ok = 1;
	memset (ch, 0, sizeof (channel_t));

	if (*s == ':') {
#if 0
		groupSep = true;
		if (*++s == '@' && *++s) {
			char *p = NULL;
			errno = 0;
			int n = strtol (s, &p, 10);
			if (!errno && p != s && n > 0) {
				ch->number = n;
				s = p;
			}
		}
		ch->name = strdup (skipspace (s));
		strreplace (ch->name, '|', ':');
#endif
		ok = 0;
	} else {
//              groupSep = false;
		char *namebuf = NULL;
		char *sourcebuf = NULL;
		char *parambuf = NULL;
		char *vpidbuf = NULL;
		char *apidbuf = NULL;
		char *caidbuf = NULL;
		int fields;
#if ! (defined WIN32 || defined APPLE)
		fields = sscanf (s, "%a[^:]:%d :%a[^:]:%a[^:] :%d :%a[^:]:"
			"%a[^:]:%d :%a[^:]:%d :%d :%d :%d ", &namebuf, 
			&ch->frequency, &parambuf, &sourcebuf, &ch->srate, 
			&vpidbuf, &apidbuf, &ch->tpid, &caidbuf, &ch->sid, 
			&ch->nid, &ch->tid, &ch->rid);
#else
		namebuf = (char *) malloc (1024);
		parambuf = (char *) malloc (1024);
		sourcebuf = (char *) malloc (1024);
		vpidbuf = (char *) malloc (1024);
		apidbuf = (char *) malloc (1024);
		caidbuf = (char *) malloc (1024);
		fields = sscanf (s, "%[^:]:%d :%[^:]:%[^:] :%d :%[^:]:%[^:]:"
			"%d :%[^:]:%d :%d :%d :%d ", namebuf, &ch->frequency, 
			parambuf, sourcebuf, &ch->srate, vpidbuf, apidbuf, 
			&ch->tpid, caidbuf, &ch->sid, &ch->nid, &ch->tid, 
			&ch->rid);

#endif
		if (fields >= 9) {
			if (fields == 9) {
				// allow reading of old format
				ch->sid = atoi (caidbuf);
				free (caidbuf);
				caidbuf = NULL;
				ch->caids[0] = ch->tpid;
				ch->caids[1] = 0;
				ch->tpid = 0;
			}
			ch->vpid = ch->ppid = 0;
			ch->apids[0] = 0;
			ch->dpids[0] = 0;
			ok = false;
			if (parambuf && sourcebuf && vpidbuf && apidbuf) {
				char *p, *dpidbuf, *q, *strtok_next;
//				int NumApids;
				ok = StringToParameters (parambuf, ch);
				ch->source = SourceFromString (sourcebuf);
				ok = ok && ((ch->source >= 0) || 
					(ch->source == -2)||(ch->source == -3));
				p = strchr (vpidbuf, '+');
				if (p)
					*p++ = 0;
				if (sscanf (vpidbuf, "%d", &ch->vpid) != 1)
					return false;
				if (p) {
					if (sscanf (p, "%d", &ch->ppid) != 1)
						return false;
				} else
					ch->ppid = ch->vpid;

				dpidbuf = strchr (apidbuf, ';');
				if (dpidbuf)
					*dpidbuf++ = 0;
				p = apidbuf;

				ch->NumApids = 0;

				while ((q = strtok_r (p, ",", &strtok_next))) {
					if (ch->NumApids < MAXAPIDS) {
						ch->apids[ch->NumApids++] = 
							strtol (q, NULL, 10);
					} else
						// no need to set ok to 'false'
						printf ("ERROR: too many APIDs!"
							"\n");
					p = NULL;
				}
				ch->apids[ch->NumApids] = 0;
				if (dpidbuf) {
				    char *p = dpidbuf;
				    char *q;
				    int NumDpids = 0;
				    char *strtok_next;
				    while ((q=strtok_r(p, ",", &strtok_next))) {
					if (NumDpids < MAXDPIDS) {
  						ch->dpids[ch->NumDpids++] = 
							strtol (q, NULL, 10);
					} else
						// no need to set ok to 'false'
						printf ("ERROR: too many DPIDs!"
							"\n");
					p = NULL;
				    }
				    ch->dpids[ch->NumDpids] = 0;
				}
#if 0
				if (caidbuf) {
				    char *p = caidbuf;
				    char *q;
				    int NumCaIds = 0;
				    char *strtok_next;
				    while ((q=strtok_r(p, ",", &strtok_next))) {
					if (NumCaIds < MAXCAIDS) {
					    caids[NumCaIds++] = 
						strtol (q, NULL, 16) & 0xFFFF;
					    if (NumCaIds == 1 && 
						caids[0] <= 0x00FF)
						    break;
					} else
					    // no need to set ok to 'false'
					    printf ("ERROR: too many CA ids!"
						\n");
					p = NULL;
				    }
				    caids[NumCaIds] = 0;
				}
#endif
			}
			strreplace (namebuf, '|', ':');
			{
				char *p = strchr (namebuf, ';');
				if (p) {
					*p++ = 0;
					ch->provider = strdup (p);
					if (!ch->provider) {
					    printf ("ERROR: out of memory!\n");
					    ok = 0;
					}
				}
				p = strchr (namebuf, ',');
				if (p) {
					*p++ = 0;
					ch->shortName = strdup (p);
					if (!ch->shortName) {
					    printf ("ERROR: out of memory!\n");
					    ok = 0;
					}
				}
			}
			ch->name = strdup (namebuf);
			if (!ch->name) {
				printf ("ERROR: out of memory!\n");
				ok = 0;
			}

		} else
			ok = 0;
#if (defined WIN32 || defined APPLE)
		free (parambuf);
		free (sourcebuf);
		free (vpidbuf);
		free (apidbuf);
		free (caidbuf);
		free (namebuf);
#endif
	}
	return ok;
}
