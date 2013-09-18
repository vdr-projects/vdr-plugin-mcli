#ifndef __SIPARSER_H__
#define __SIPARSER_H__

#define TS_SECT_LEN(buf) \
	unsigned char *ptr = buf; \
	int slen = (((ptr[1] << 8) | ptr[2]) & 0x0fff); 
	

#define TS_PACKET_LEN (188)              /* TS RDSIZE is fixed !! */
#define TS_SYNC_BYTE  (0x47)             /* SyncByte for TS  ISO 138181-1 */
#define TS_BUF_SIZE   (256 * 1024)       /* default DMX_Buffer Size for TS */
#define PSI_BUF_SIZE  (2 * 4096)	/* Section length max. 12 bits */
#define READ_BUF_SIZE (256*TS_PACKET_LEN)  /* min. 2x TS_PACKET_LEN!!! */
#define BILLION  1000000000L;
#define MAX_DESC_LEN 255 //descriptor_length field 8-bit ISO/IEC 13818-1
#define MAX_ES_PIDS 32


#define VIDEO_11172_STREAM_TYPE 			0x1 // STREAMTYPE_11172_VIDEO
#define VIDEO_13818_STREAM_TYPE				0x2 // STREAMTYPE_13818_VIDEO
#define VISUAL_MPEG4_STREAM_TYPE			0x10 // 14496-2 Visual MPEG-4
#define VIDEO_H264_STREAM_TYPE              		0x1b // 14496-10 Video h.264
#define AUDIO_11172_STREAM_TYPE             		0x3 // STREAMTYPE_11172_AUDIO
#define AUDIO_13818_STREAM_TYPE 			0x4 // STREAMTYPE_13818_AUDIO
#define PRIVATE_13818_STREAM_TYPE	               	0x5 // STREAMTYPE_13818_PRIVATE
#define PRIVATE_13818_PES_STREAM_TYPE			0x6 // STREAMTYPE_13818_PES_PRIVATE 

enum DescriptorTag {
  // defined by ISO/IEC 13818-1
               VideoStreamDescriptorTag = 0x02,
               AudioStreamDescriptorTag = 0x03,
               HierarchyDescriptorTag = 0x04,
               RegistrationDescriptorTag = 0x05,
               DataStreamAlignmentDescriptorTag = 0x06,
               TargetBackgroundGridDescriptorTag = 0x07,
               VideoWindowDescriptorTag = 0x08,
               CaDescriptorTag = 0x09,
               ISO639LanguageDescriptorTag = 0x0A,
               SystemClockDescriptorTag = 0x0B,
               MultiplexBufferUtilizationDescriptorTag = 0x0C,
               CopyrightDescriptorTag = 0x0D,
               MaximumBitrateDescriptorTag = 0x0E,
               PrivateDataIndicatorDescriptorTag = 0x0F,
               SmoothingBufferDescriptorTag = 0x10,
               STDDescriptorTag = 0x11,
               IBPDescriptorTag = 0x12,
  // defined by ISO-13818-6 (DSM-CC)
               CarouselIdentifierDescriptorTag = 0x13,
               // 0x14 - 0x3F  Reserved
  // defined by ETSI (EN 300 468)
               NetworkNameDescriptorTag = 0x40,
               ServiceListDescriptorTag = 0x41,
               StuffingDescriptorTag = 0x42,
               SatelliteDeliverySystemDescriptorTag = 0x43,
               CableDeliverySystemDescriptorTag = 0x44,
               VBIDataDescriptorTag = 0x45,
               VBITeletextDescriptorTag = 0x46,
               BouquetNameDescriptorTag = 0x47,
               ServiceDescriptorTag = 0x48,
               CountryAvailabilityDescriptorTag = 0x49,
               LinkageDescriptorTag = 0x4A,
               NVODReferenceDescriptorTag = 0x4B,
               TimeShiftedServiceDescriptorTag = 0x4C,
               ShortEventDescriptorTag = 0x4D,
               ExtendedEventDescriptorTag = 0x4E,
               TimeShiftedEventDescriptorTag = 0x4F,
               ComponentDescriptorTag = 0x50,
               MocaicDescriptorTag = 0x51,
               StreamIdentifierDescriptorTag = 0x52,
               CaIdentifierDescriptorTag = 0x53,
               ContentDescriptorTag = 0x54,
               ParentalRatingDescriptorTag = 0x55,
               TeletextDescriptorTag = 0x56,
               TelephoneDescriptorTag = 0x57,
               LocalTimeOffsetDescriptorTag = 0x58,
               SubtitlingDescriptorTag = 0x59,
               TerrestrialDeliverySystemDescriptorTag = 0x5A,
               MultilingualNetworkNameDescriptorTag = 0x5B,
               MultilingualBouquetNameDescriptorTag = 0x5C,
               MultilingualServiceNameDescriptorTag = 0x5D,
               MultilingualComponentDescriptorTag = 0x5E,
               PrivateDataSpecifierDescriptorTag = 0x5F,
               ServiceMoveDescriptorTag = 0x60,
               ShortSmoothingBufferDescriptorTag = 0x61,
               FrequencyListDescriptorTag = 0x62,
               PartialTransportStreamDescriptorTag = 0x63,
               DataBroadcastDescriptorTag = 0x64,
               ScramblingDescriptorTag = 0x65,
               DataBroadcastIdDescriptorTag = 0x66,
               TransportStreamDescriptorTag = 0x67,
               DSNGDescriptorTag = 0x68,
               PDCDescriptorTag = 0x69,
               AC3DescriptorTag = 0x6A,
               AncillaryDataDescriptorTag = 0x6B,
               CellListDescriptorTag = 0x6C,
               CellFrequencyLinkDescriptorTag = 0x6D,
               AnnouncementSupportDescriptorTag = 0x6E,
               ApplicationSignallingDescriptorTag = 0x6F,
               AdaptationFieldDataDescriptorTag = 0x70,
               ServiceIdentifierDescriptorTag = 0x71,
               ServiceAvailabilityDescriptorTag = 0x72,
  // defined by ETSI (EN 300 468) v 1.7.1
               DefaultAuthorityDescriptorTag = 0x73,
               RelatedContentDescriptorTag = 0x74,
               TVAIdDescriptorTag = 0x75,
               ContentIdentifierDescriptorTag = 0x76,
               TimeSliceFecIdentifierDescriptorTag = 0x77,
               ECMRepetitionRateDescriptorTag = 0x78,
               S2SatelliteDeliverySystemDescriptorTag = 0x79,
               EnhancedAC3DescriptorTag = 0x7A,
               DTSDescriptorTag = 0x7B,
               AACDescriptorTag = 0x7C,
               ExtensionDescriptorTag = 0x7F,

 // Defined by ETSI TS 102 812 (MHP)
               // They once again start with 0x00 (see page 234, MHP specification)
               MHP_ApplicationDescriptorTag = 0x00,
               MHP_ApplicationNameDescriptorTag = 0x01,
               MHP_TransportProtocolDescriptorTag = 0x02,
               MHP_DVBJApplicationDescriptorTag = 0x03,
               MHP_DVBJApplicationLocationDescriptorTag = 0x04,
               // 0x05 - 0x0A is unimplemented this library
               MHP_ExternalApplicationAuthorisationDescriptorTag = 0x05,
               MHP_IPv4RoutingDescriptorTag = 0x06,
               MHP_IPv6RoutingDescriptorTag = 0x07,
               MHP_DVBHTMLApplicationDescriptorTag = 0x08,
               MHP_DVBHTMLApplicationLocationDescriptorTag = 0x09,
               MHP_DVBHTMLApplicationBoundaryDescriptorTag = 0x0A,
               MHP_ApplicationIconsDescriptorTag = 0x0B,
               MHP_PrefetchDescriptorTag = 0x0C,
               MHP_DelegatedApplicationDescriptorTag = 0x0E,
               MHP_ApplicationStorageDescriptorTag = 0x10,
  // Premiere private Descriptor Tags
               PremiereContentTransmissionDescriptorTag = 0xF2,

               //a descriptor currently unimplemented in this library
               //the actual value 0xFF is "forbidden" according to the spec.
               UnimplementedDescriptorTag = 0xFF
};



typedef struct ts_packet_hdr 
{
  unsigned int sync_byte;
  unsigned int transport_error_indicator;
  unsigned int payload_unit_start_indicator;
  unsigned int transport_priority;
  unsigned int pid;
  unsigned int transport_scrambling_control;
  unsigned int adaptation_field_control;
  unsigned int continuity_counter;
} ts_packet_hdr_t;

typedef struct  pat {
  unsigned int table_id;
  unsigned int section_syntax_indicator;		
  unsigned int reserved_1;
  unsigned int section_length;
  unsigned int transport_stream_id;
  unsigned int reserved_2;
  unsigned int version_number;
  unsigned int current_next_indicator;
  unsigned int section_number;
  unsigned int last_section_number;

  // FIXME: list of programs

  unsigned int crc32;
} pat_t;

typedef struct _pat_list {
  unsigned int program_number; //SID
  unsigned int reserved;
  unsigned int network_pmt_pid;

  int cads_present;
  int cads_num;

} pat_list_t;

typedef struct pmt_pid_list {

  pat_t p;
  pat_list_t *pl;
  unsigned int pmt_pids;
 
} pmt_pid_list_t;

typedef struct psi_buf {

  unsigned char *buf;
  unsigned int len;//used for offset
  unsigned int start;  

  int pid;
  int continuity;

} psi_buf_t;

typedef struct pmt {
    unsigned int table_id;
    unsigned int section_syntax_indicator;		
    unsigned int reserved_1;
    unsigned int section_length;
    unsigned int program_number;
    unsigned int reserved_2;
    unsigned int version_number;
    unsigned int current_next_indicator;
    unsigned int section_number;
    unsigned int last_section_number;
    unsigned int reserved_3;
    unsigned int pcr_pid;
    unsigned int reserved_4;
    unsigned int program_info_length;

    // N descriptors
    
    // N1 stream types and descriptors

    unsigned int crc32;  
} pmt_t;

typedef struct es_pmt_info {
    unsigned int stream_type;
    unsigned int reserved_1; 
    unsigned int elementary_pid;
    unsigned int reserved_2;
    unsigned int es_info_length;

    // N2 descriptor

} es_pmt_info_t;

typedef struct  ca_descriptor {
    
    unsigned int descriptor_tag;
    unsigned int descriptor_length;		
    unsigned int  ca_system_id;
    unsigned int  reserved;
    unsigned int  ca_pid;
    unsigned char private_data[MAX_DESC_LEN];

} si_desc_t;

typedef struct pmt_descriptor {

    pmt_t pmt_hdr;

    int cas;
    si_desc_t *cad;

} si_pmt_desc_t;

typedef struct ca_descriptor_list {

    int cads;
    si_desc_t *cad;

} si_cad_t;

typedef struct ca_sid_info {

    int sid;
    int version;
    int offset;
    int len;
  
} ca_sid_t;

typedef struct ca_pmt_descriptors {

    int cads;
    int size;
    unsigned char *cad;
            
} si_ca_pmt_t;

typedef struct ca_es_pid_info {

    int pid;
    uint8_t type;

} ca_es_pid_info_t;

typedef struct ca_pmt_list {
    
    int sid;
    int pmt_pid;
    
    pmt_t p;
    si_ca_pmt_t pm;
    si_ca_pmt_t es;    

    ca_es_pid_info_t espids[MAX_ES_PIDS];    
    int es_pid_num;
  
} ca_pmt_list_t;


typedef struct ca_sid_list {

    int tc; //total number of CA desc.
    int num;
    ca_pmt_list_t *l;

} ca_sid_list_t;

typedef struct  _cat {
    unsigned int table_id;
    unsigned int section_syntax_indicator;		
    unsigned int reserved_1;
    unsigned int section_length;
    unsigned int reserved_2;
    unsigned int version_number;
    unsigned int current_next_indicator;
    unsigned int section_number;
    unsigned int last_section_number;

    // private section
    
    unsigned int crc32;
} cat_t;

typedef struct tdt_sect {

      uint8_t  table_id;
      uint8_t  section_syntax_indicator;
      uint8_t  reserved; //0 future use
      uint8_t  reserved_1;
      uint16_t section_length;
      uint8_t  dvbdate[5];
} tdt_sect_t;

typedef struct _str_table {
    unsigned int       from;          
    unsigned int       to;            
    const char  	*str;          
} str_table;
            

int parse_ca_descriptor(unsigned char *desc, si_desc_t *t);

int ts2psi_data(unsigned char *buf,psi_buf_t *p,int len, int pid_req);	
int parse_pat_sect(unsigned char *buf, int size,  pmt_pid_list_t *pmt);
int parse_pmt_ca_desc(unsigned char *buf, int size, int sid, si_ca_pmt_t *pm_cads, si_ca_pmt_t *es_cads, pmt_t *pmt_hdr, int *fta, ca_es_pid_info_t *espid, int *es_pid_num);
int parse_cat_sect(unsigned char *buf, int size, si_cad_t *emm);
int parse_tdt_sect(unsigned char *buf, int size, tdt_sect_t *tdt);
int get_ts_packet_hdr(unsigned char *buf, ts_packet_hdr_t *p);
int si_get_video_pid(unsigned char *esi_buf, int size, int *vpid);
int si_get_audio_pid(unsigned char *esi_buf, int size, int *apid);
int si_get_private_pids(unsigned char *esi_buf, int size, int *upids);
int get_pmt_es_pids(unsigned char *esi_buf, int size, int *es_pids, int all);
void print_pat(pat_t *p, pat_list_t *pl, int pmt_num);
void printhex_buf(char *msg,unsigned char *buf,int len);
void writehex_buf(FILE *f,char *msg,unsigned char *buf,int len);
void print_cad_lst(si_cad_t *l, int ts_id);
void print_ca_bytes(si_desc_t *p);
void get_time_mjd (unsigned long mjd, long *year , long *month, long *day);
void print_tdt(tdt_sect_t *tdt, uint16_t mjd, uint32_t utc);
int ca_free_cpl_desc(ca_pmt_list_t *cpl);
char *si_caid_to_name(unsigned int caid);

#endif





