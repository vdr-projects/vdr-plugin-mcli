#include "headers.h"

//#define DBG 1
#define CRC32_CHECK 1

enum ca_desc_type { EMM, ECM };

//-----------------------------------------------------------------------------------
void printhex_buf(char *msg,unsigned char *buf,int len)
{
  int i,j,k;
  int width=8;

  i=k=0;
  sys ("%s: %d bytes (0x%04x)\n",msg,len,len);
  sys ("---------------------------------------------------------------\n");    
  while(len) {
    sys ("%04x	",k++*width*2);
    j=i;
    for(;i < j + width ; i++){ 
      if (i >= len) break; 
      sys ("%02x ",buf[i]);  
    }
    if (i >= len) {
      sys ("\n");
      break;
    }
    sys("	");
    j=i;
    for(;i < j + width ; i++){
      if (i >= len) break;
      sys("%02x ",buf[i]);      
    }
    sys("\n");
    if (i >= len) break;
  }    
  sys("---------------------------------------------------------------\n");    
}
//-----------------------------------------------------------------------------------
void writehex_buf(FILE *f, char *msg,unsigned char *buf,int len)
{
  int i,j,k;
  int width=8;

  i=k=0;
  fprintf(f,"%s: %d bytes (0x%04x)\n",msg,len,len);
  fprintf(f,"---------------------------------------------------------------\n");    
  while(len) {
    fprintf(f,"%04x	",k++*width*2);
    j=i;
    for(;i < j + width ; i++){ 
      if (i >= len) break; 
      fprintf(f,"%02x ",buf[i]);  
    }
    if (i >= len) {
      fprintf(f,"\n");
      break;
    }
    fprintf(f,"	");
    j=i;
    for(;i < j + width ; i++){
      if (i >= len) break;
      fprintf(f,"%02x ",buf[i]);      
    }
    fprintf(f,"\n");
    if (i >= len) break;
  }    
  fprintf(f,"---------------------------------------------------------------\n");    


}
//-----------------------------------------------------------------------------------
void print_ts_header(ts_packet_hdr_t *p)
{
  info("--------------------------------------------------------------\n");
  info("TS header data:\n");
  info("Sync-byte			: 0x%04x\n",p->sync_byte);
  info("Transport error indicator	: 0x%04x\n",p->transport_error_indicator);
  info("Payload unit start indicator	: 0x%04x\n",p->payload_unit_start_indicator);
  info("Transport priority		: 0x%04x\n",p->transport_priority);
  info("PID				: 0x%04x\n",p->pid);
  info("Transport scrambling control	: 0x%04x\n",p->transport_scrambling_control);
  info("Adaptation field control	: 0x%04x\n",p->adaptation_field_control);
  info("Continuity_counter		: 0x%04x\n",p->continuity_counter);

}
//-----------------------------------------------------------------------------------
void print_pmt(pmt_t *p)
{
  info("--------------------------------------------------------------\n");    
  info("PMT section:\n");  
  info("Table ID                 : %-5d (0x%04x)\n",p->table_id,p->table_id);
  info("(fixed):                 : %-5d (0x%04x)\n",0,0);  
  info("Section syntax indicator : %-5d (0x%04x)\n",p->section_syntax_indicator,p->section_syntax_indicator);		
  info("Reserved 1               : %-5d (0x%04x)\n",p->reserved_1,p->reserved_1);
  info("Section length           : %-5d (0x%04x)\n",p->section_length,p->section_length);
  info("Program number           : %-5d (0x%04x)\n",p->program_number,p->program_number);
  info("Reserved 2               : %-5d (0x%04x)\n",p->reserved_2,p->reserved_2);
  info("Version number           : %-5d (0x%04x)\n",p->version_number,p->version_number);
  info("Current next indicator   : %-5d (0x%04x)\n",p->current_next_indicator,p->current_next_indicator);
  info("Section number           : %-5d (0x%04x)\n",p->section_number,p->section_number);
  info("Last section number      : %-5d (0x%04x)\n",p->last_section_number,p->last_section_number);
  info("Reserved 3               : %-5d (0x%04x)\n",p->reserved_3,p->reserved_3);
  info("PCR pid                  : %-5d (0x%04x)\n",p->pcr_pid,p->pcr_pid);
  info("Reserved 4               : %-5d (0x%04x)\n",p->reserved_4,p->reserved_4);
  info("Program info length      : %-5d (0x%04x)\n",p->program_info_length,p->program_info_length);



  info("CRC32                    : 0x%04x\n",p->crc32);		
}
//-----------------------------------------------------------------------------------
void print_pat(pat_t *p, pat_list_t *pl, int pmt_num)
{
  info("--------------------------------------------------------------\n");
  info("PAT section:\n");
  info("Table_id                 : %-5d (0x%04x)\n",p->table_id,p->table_id);
  info("(fixed):                 : %-5d (0x%04x)\n",0,0);
  info("Section syntax indicator : %-5d (0x%04x)\n",p->section_syntax_indicator,p->section_syntax_indicator);
  info("Reserved_1               : %-5d (0x%04x)\n",p->reserved_1,p->reserved_1);
  info("Section length           : %-5d (0x%04x)\n",p->section_length,p->section_length);
  info("Transport stream id      : %-5d (0x%04x)\n",p->transport_stream_id,p->transport_stream_id);
  info("Reserved 2               : %-5d (0x%04x)\n",p->reserved_2,p->reserved_2);
  info("Version number           : %-5d (0x%04x)\n",p->version_number,p->version_number);
  info("Current next indicator   : %-5d (0x%04x)\n",p->current_next_indicator,p->current_next_indicator);
  info("Section number           : %-5d (0x%04x)\n",p->section_number,p->section_number);
  info("Last section number      : %-5d (0x%04x)\n",p->last_section_number,p->last_section_number);

  if (pl && pmt_num){
    int i;
    info("Number of PMTs in PAT : %-5d \n", pmt_num);
    for(i=0;i<pmt_num;i++) {
      pat_list_t *pat = pl + i;
      info("\nProgram number  : %-5d (0x%04x)\n",pat->program_number,pat->program_number);
      info("Reserved        : %-5d (0x%04x)\n",pat->reserved,pat->reserved);
      info("Network PMT PID : %-5d (0x%04x)\n",pat->network_pmt_pid,pat->network_pmt_pid);
    }
  }

  info("CRC32                   : 0x%04x\n",p->crc32);


}
//-----------------------------------------------------------------------------------
char *si_caid_to_name(unsigned int caid)
{

	str_table  table[] = {
		// -- updated from dvb.org 2003-10-16
            {  0x0000, 0x0000,  "Reserved" },
            {  0x0001, 0x00FF,  "Standardized Systems" },
            {  0x0100, 0x01FF,  "Canal Plus (Seca/MediaGuard)" },
            {  0x0200, 0x02FF,  "CCETT" },
            {  0x0300, 0x03FF,  "MSG MediaServices GmbH" },
            {  0x0400, 0x04FF,  "Eurodec" },
            {  0x0500, 0x05FF,  "France Telecom (Viaccess)" },
            {  0x0600, 0x06FF,  "Irdeto" },
            {  0x0700, 0x07FF,  "Jerrold/GI/Motorola" },
            {  0x0800, 0x08FF,  "Matra Communication" },
            {  0x0900, 0x09FF,  "News Datacom (Videoguard)" },
            {  0x0A00, 0x0AFF,  "Nokia" },
            {  0x0B00, 0x0BFF,  "Norwegian Telekom (Conax)" },
            {  0x0C00, 0x0CFF,  "NTL" },
            {  0x0D00, 0x0DFF,  "Philips (Cryptoworks)" },
            {  0x0E00, 0x0EFF,  "Scientific Atlanta (Power VU)" },
            {  0x0F00, 0x0FFF,  "Sony" },
            {  0x1000, 0x10FF,  "Tandberg Television" },
            {  0x1100, 0x11FF,  "Thompson" },
            {  0x1200, 0x12FF,  "TV/COM" },  
            {  0x1300, 0x13FF,  "HPT - Croatian Post and Telecommunications" },
            {  0x1400, 0x14FF,  "HRT - Croatian Radio and Television" },
            {  0x1500, 0x15FF,  "IBM" },
            {  0x1600, 0x16FF,  "Nera" },
            {  0x1700, 0x17FF,  "Beta Technik (Betacrypt)" },
            {  0x1800, 0x18FF,  "Kudelski SA"},
            {  0x1900, 0x19FF,  "Titan Information Systems"},
            {  0x2000, 0x20FF,  "TelefXnica Servicios Audiovisuales"},
            {  0x2100, 0x21FF,  "STENTOR (France Telecom, CNES and DGA)"},
            {  0x2200, 0x22FF,  "Scopus Network Technologies"},
            {  0x2300, 0x23FF,  "BARCO AS"},
            {  0x2400, 0x24FF,  "StarGuide Digital Networks  "},
            {  0x2500, 0x25FF,  "Mentor Data System, Inc."},
            {  0x2600, 0x26FF,  "European Broadcasting Union"},
            {  0x4700, 0x47FF,  "General Instrument"},
            {  0x4800, 0x48FF,  "Telemann"},
            {  0x4900, 0x49FF,  "Digital TV Industry Alliance of China"},
            {  0x4A00, 0x4A0F,  "Tsinghua TongFang"},
            {  0x4A10, 0x4A1F,  "Easycas"},
            {  0x4A20, 0x4A2F,  "AlphaCrypt"},
            {  0x4A30, 0x4A3F,  "DVN Holdings"},
            {  0x4A40, 0x4A4F,  "Shanghai Advanced Digital Technology Co. Ltd. (ADT)"},
            {  0x4A50, 0x4A5F,  "Shenzhen Kingsky Company (China) Ltd"},
            {  0x4A60, 0x4A6F,  "@SKY"},
            {  0x4A70, 0x4A7F,  "DreamCrypt"},
            {  0x4A80, 0x4A8F,  "THALESCrypt"},
            {  0x4A90, 0x4A9F,  "Runcom Technologies"},
            {  0x4AA0, 0x4AAF,  "SIDSA"},
            {  0x4AB0, 0x4ABF,  "Beijing Comunicate Technology Inc."},
            {  0x4AC0, 0x4ACF,  "Latens Systems Ltd"},
	    {  0,0, NULL }
	};
	
	int i = 0;
	while (table[i].str) {
		if (table[i].from <= caid && table[i].to >= caid)
			return (char *) table[i].str;
		i++;
	}
   	
	return (char *) "ERROR: Undefined!";
}
//-----------------------------------------------------------------------------------
void get_time_mjd (unsigned long mjd, long *year , long *month, long *day)
{
    if (mjd > 0) {
        long   y,m,d ,k;

        // algo: ETSI EN 300 468 - ANNEX C

        y =  (long) ((mjd  - 15078.2) / 365.25);
        m =  (long) ((mjd - 14956.1 - (long)(y * 365.25) ) / 30.6001);
        d =  (long) (mjd - 14956 - (long)(y * 365.25) - (long)(m * 30.6001));
        k =  (m == 14 || m == 15) ? 1 : 0;
        y = y + k + 1900;
        m = m - 1 - k*12;
        *year = y;
        *month = m;
        *day = d;

    } else {
	*year = 0;
	*month = 0;
	*day = 0;
    }

} 
//-----------------------------------------------------------------------------------
void print_tdt(tdt_sect_t *tdt, uint16_t mjd, uint32_t utc)
{
    info("--------------------------------------------------------------\n");
    info("TDT section:\n");
    info("Table_id                 : %-5d (0x%04x)\n",tdt->table_id,tdt->table_id);
    info("Reserved                 : %-5d (0x%04x)\n",tdt->reserved,tdt->reserved);
    info("Reserved_1               : %-5d (0x%04x)\n",tdt->reserved_1,tdt->reserved_1);
    info("Section length           : %-5d (0x%04x)\n",tdt->section_length,tdt->section_length);    
    info("UTC_time                 : 0x%2x%2x%2x%2x%2x\n",tdt->dvbdate[0],tdt->dvbdate[1],tdt->dvbdate[2],tdt->dvbdate[3],tdt->dvbdate[4]); 
    
    long y,m,d;
    get_time_mjd(mjd, &y, &m, &d);
    info("TIME: [= %02ld-%02ld-%02ld  %02x:%02x:%02x (UTC) ]\n\n",y,m,d,(utc>>16) &0xFF, (utc>>8) &0xFF, (utc) &0xFF);
    info("--------------------------------------------------------------\n");

}
//-----------------------------------------------------------------------------------
void print_ca_desc(si_desc_t *p)
{
  info("CA desc. tag    : %d (%#x)\n",p->descriptor_tag,p->descriptor_tag);
  info("CA desc. length : %d (%#x)\n",p->descriptor_length,p->descriptor_length);
  info("CA system id    : %d (%#x)\n",p->ca_system_id,p->ca_system_id);
  info("Reserverd       : %d (%#x)\n",p->reserved,p->reserved);
  info("CA pid          : %d (%#x)\n",p->ca_pid,p->ca_pid);

  printhex_buf((char *)"Private data",p->private_data,p->descriptor_length-4);

}
//-----------------------------------------------------------------------------------
void print_ca_bytes(si_desc_t *p)
{
  unsigned int i; 
  info("%x %x %x %x %x ",p->descriptor_tag, p->descriptor_length, p->ca_system_id, p->reserved, p->ca_pid);
  for (i = 0; i < p->descriptor_length - 4; i++)
      info("%x ",p->private_data[i]);
  info(";");

}
//-----------------------------------------------------------------------------------
void print_cad_lst(si_cad_t *l, int ts_id)
{
  int i;
  
  for (i = 0; i < l->cads; i++) {
    print_ca_desc(&l->cad[i]);
  }
  info("Total CA desc. for TS ID %d : %d\n",ts_id,l->cads);
}
//-----------------------------------------------------------------------------------
int parse_ca_descriptor(unsigned char *desc, si_desc_t *t)
{
  unsigned char *ptr=desc;
  int tag=0,len=0;
  
  tag=ptr[0];
  len=ptr[1]; 

  if (len > MAX_DESC_LEN) {
    info("descriptor():Descriptor too long !\n");
    return -1;
  }
  
  switch(tag){
    case 0x09: {  
        t->descriptor_tag=tag;
        t->descriptor_length=len; //???
        t->ca_system_id=((ptr[2] << 8) | ptr[3]);
        t->reserved=(ptr[4] >> 5) & 7;
        t->ca_pid=((ptr[4] << 8) | ptr[5]) & 0x1fff; 
        //header 4 bytes + 2 bytes
        memcpy(t->private_data,ptr+6,len-4);

        //print_ca_desc(t);
        
        break; 
      }
    default: 
        break;
    }

  return len + 2; //2 bytes tag + length
}
//--------------------------------------------------------------------------------------------
int ca_free_cpl_desc(ca_pmt_list_t *cpl)
{
	if (cpl->pm.size > 0 && cpl->pm.cad)
		free(cpl->pm.cad);
	if (cpl->es.size > 0 && cpl->es.cad)
		free(cpl->es.cad);

	memset(cpl,0,sizeof(ca_pmt_list_t));				

	return 0;
}
//--------------------------------------------------------------------------------------------
int descriptor(unsigned char *desc, si_cad_t *c)
{
  unsigned char *ptr=desc;
  int tag=0,len=0;
  
  tag=ptr[0];
  len=ptr[1]; 

  if (len > MAX_DESC_LEN) {
    info("descriptor():Descriptor too long !\n");
    return -1;
  }
  
  switch(tag){
    case 0x09: {
        c->cads++;
        c->cad = (si_desc_t*)realloc(c->cad,sizeof(si_desc_t)*c->cads);
        if (!c->cad) {
          c->cads--;
          info("descriptor():realloc error\n");
          return -1;
        }
        si_desc_t *t = c->cad + c->cads - 1;
        t->descriptor_tag=tag;
        t->descriptor_length=len; //???
        t->ca_system_id=((ptr[2] << 8) | ptr[3]);
        t->reserved=(ptr[4] >> 5) & 7;
        t->ca_pid=((ptr[4] << 8) | ptr[5]) & 0x1fff; 
        //header 4 bytes + 2 bytes
        if (len - 4 > 0) 
            memcpy(t->private_data,ptr+6,len-4);

        //print_ca_desc(t);
        break; 
      }
    default: { 
#if 0
        other_desc_t d;
        d.descriptor_tag=tag;
        d.descriptor_length=len;
        memcpy(d.data,ptr+2,len);
        //print_desc(d);
#endif
      }
    }

  return len + 2; //2 bytes tag + length
}

//-----------------------------------------------------------------------------------
int si_get_video_pid(unsigned char *esi_buf, int size, int *vpid)
{
      int index, pid_num, es_len;
      unsigned char *ptr = esi_buf;

      index = pid_num = 0;      
      while(index < size) {
          //ptr[0] //stream type 
          if (ptr[0] == 2 || ptr[0] == 0x1b) 
          {
              *vpid = ((ptr[1] << 8) | ptr[2]) & 0x1fff;
              return 1;
          }
          es_len = ((ptr[3] << 8) | ptr[4]) & 0x0fff;
          index += 5 + es_len;
          ptr += 5 + es_len;   
      }

      *vpid = -1;
      return 0;

}
//-----------------------------------------------------------------------------------
int si_get_audio_pid(unsigned char *esi_buf, int size, int *apid)
{
      int index, pid_num, es_len;
      unsigned char *ptr = esi_buf;

      index = pid_num = 0;      
      while(index < size) {
          //ptr[0] //stream type 
          if (ptr[0] == 0x1 || ptr[0] == 0x3 || ptr[0] == 0x4) 
          {
              *apid = ((ptr[1] << 8) | ptr[2]) & 0x1fff;
              return 1;
          }
          es_len = ((ptr[3] << 8) | ptr[4]) & 0x0fff;
          index += 5 + es_len;
          ptr += 5 + es_len;   
      }

      *apid = -1;
      return 0;

}
//-----------------------------------------------------------------------------------
int si_get_private_pids(unsigned char *esi_buf, int size, int *upids)
{

      int index, pid_num, es_len;
      unsigned char *ptr = esi_buf;

      index = pid_num = 0;      
      while(index < size) {
          if (ptr[0] == 0x6)
          {
              upids[pid_num] = ((ptr[1] << 8) | ptr[2]) & 0x1fff;
              pid_num++;
              if (pid_num >= MAX_ES_PIDS) {
                    info ("error: ES pids number out of bounds !\n");
                    return -1;
              }
          }
          es_len = ((ptr[3] << 8) | ptr[4]) & 0x0fff;
          index += 5 + es_len;
          ptr += 5 + es_len;   
      }

      return pid_num;

}
//-----------------------------------------------------------------------------------
int get_pmt_es_pids(unsigned char *esi_buf, int size, int *es_pids, int all)
{
      int index, pid_num, es_len;
      unsigned char *ptr = esi_buf;

      index = pid_num = 0;      
      while(index < size) {
          //ptr[0] //stream type 
          //int pid = ((ptr[1] << 8) | ptr[2]) & 0x1fff;
          //printf("Stream type: %d (%#x) pid = %d (%#x)\n",ptr[0], ptr[0], pid, pid);
          if (ptr[0] == 0x1 || ptr[0] == 0x2 ||  ptr[0] == 0x3 || ptr[0] == 0x4 || ptr[0] == 0x6 || ptr[0] == 0x1b || all)
          {
              es_pids[pid_num] = ((ptr[1] << 8) | ptr[2]) & 0x1fff;
              pid_num++;
              if (pid_num >= MAX_ES_PIDS) {
                    info ("error: ES pids number out of bounds !\n");
                    return -1;
              }
          }
          es_len = ((ptr[3] << 8) | ptr[4]) & 0x0fff;
          index += 5 + es_len;
          ptr += 5 + es_len;   
      }

      return pid_num;
}
//-----------------------------------------------------------------------------------
int parse_pmt_ca_desc(unsigned char *buf, int size, int sid, si_ca_pmt_t *pm_cads, si_ca_pmt_t *es_cads, pmt_t *pmt_hdr, int *fta, ca_es_pid_info_t *espids, int *es_pid_num)
{
      unsigned char *ptr=buf, tmp[PSI_BUF_SIZE]; //sections can be only 12 bit long

      memset(pm_cads,0,sizeof(si_ca_pmt_t));
      memset(es_cads,0,sizeof(si_ca_pmt_t));  
      memset(pmt_hdr,0,sizeof(pmt_hdr));

      pmt_hdr->table_id=ptr[0];		         
      pmt_hdr->section_syntax_indicator=(ptr[1] >> 7) & 1;
      pmt_hdr->reserved_1=(ptr[1] >> 4) & 3; 
      pmt_hdr->section_length=((ptr[1] << 8) | ptr[2]) & 0xfff;

      if (pmt_hdr->section_length < 13 || pmt_hdr->section_length > 1021 || (int)pmt_hdr->section_length > size) {
            info("#####\nERROR: Invalid section length!\n");
            return -1;
      }

      u_long crc = dvb_crc32 ((char *)buf,pmt_hdr->section_length+3);  

#ifdef DBG
      info("CRCcc: 0x%lx\n",crc);
      info("len = %d\n", pmt_hdr->section_length+3);
#endif
      if (crc & 0xffffffff) { //FIXME: makr arch flags
            info("#####\nPMT -> ERROR: parse_pmt_ca_desc() : CRC err. crc = 0x%lx\n", crc);
            return -1;
      }

      pmt_hdr->program_number=(ptr[3] << 8) | ptr[4];      
      if ((int)pmt_hdr->program_number != sid) {
            info("#####\nERROR: Invalid SID in PMT !!!\n");
            return -1;
      }
      pmt_hdr->program_info_length=((ptr[10] << 8) | ptr[11]) & 0x0fff;	        
      if (pmt_hdr->program_info_length < 0 || pmt_hdr->program_info_length > 1021 - 9 || (int)pmt_hdr->program_info_length > size - 9) {
            info("#####\nERROR: Invalid PI length in PMT!\n");                  
            return -1;
      }	
    
      pmt_hdr->reserved_2=(ptr[5] >> 6) & 3;		        
      pmt_hdr->version_number=(ptr[5] >> 1) & 0x1f;		        
      pmt_hdr->current_next_indicator=ptr[5] & 1;	        
      pmt_hdr->section_number=ptr[6];		        
      pmt_hdr->last_section_number=ptr[7];	        
      pmt_hdr->reserved_3=(ptr[8] >> 5) & 7;		        
      pmt_hdr->pcr_pid=((ptr[8] << 8) | ptr[9]) & 0x1fff;		        
      pmt_hdr->reserved_4=(ptr[10] >> 4) & 0xf;		        
    
      //pmt_hdr->program_info_length=((ptr[10] << 8) | ptr[11]) & 0x0fff;	           
      //print_pmt(pmt_hdr);
            
      int buf_len=0,len=0;
      unsigned int i=0;
      
      buf_len = pmt_hdr->section_length - 9;
      ptr += 12; // 12 byte header
     
      pm_cads->size = pm_cads->cads =  0;
      for (i = 0; i < pmt_hdr->program_info_length;) {
        int dtag = ptr[0];
        int dlen = ptr[1] + 2;
        if (dlen > size || dlen > MAX_DESC_LEN) {
            info("PMT: Invalide CA desc. length!\n");
            return -1;
        }
        if (dtag == 0x09) { //we have CA descriptor
            memcpy(tmp + pm_cads->size, ptr, dlen);
            pm_cads->size+=dlen; 
            pm_cads->cads++;
            *fta=0;
        }
        i+=dlen;
        if (i > pmt_hdr->program_info_length) {
            info("PMT: Index out of bounds!\n");
            return -1;
        }

        ptr+=dlen; //desc. length plus 2 bytes for tag and header;
        if (ptr >= buf + size) {
            info("PMT: Invalid Buffer offset !\n");
            return -1;        
        }

        buf_len-=dlen;     
        if (buf_len < 0) {
            info("PMT: Index out of bounds!\n");
            return -1;        
        
        }
      }      

      //parsing ok we can take this program level descriptors
      if (pm_cads->size && pm_cads->cads) {
           pm_cads->cad = (unsigned char*)malloc(sizeof(unsigned char)*pm_cads->size);
           if (!pm_cads->cad) {
               info("ERROR: parse_ca_desc() : out of memory\n");
               return -1;
           }
           memcpy(pm_cads->cad, tmp, pm_cads->size);     
      }

#ifdef DBG  
      info("%d bytes remaining (program info len = %d bytes)\n",buf_len,i);    
#endif  
      
      int err = 0;
      es_pmt_info_t esi;
      es_cads->size = es_cads->cads = 0;
      *es_pid_num = 0;
      while (buf_len > 4) { //end of section crc32 is 4 bytes
        esi.stream_type=ptr[0];
        esi.reserved_1=(ptr[1] >> 5) & 7;
        esi.elementary_pid=((ptr[1] << 8) | ptr[2]) & 0x1fff;
        esi.reserved_2=(ptr[3] >> 4) & 0xf;
        esi.es_info_length=((ptr[3] << 8) | ptr[4]) & 0x0fff;

        if ((int)esi.es_info_length > buf_len) {
              info("PMT: Invalid ES info length !\n");
              err = -1;
              break;     
        }

        if (espids) {
              switch(esi.stream_type) {
                    case VIDEO_11172_STREAM_TYPE: 
                    case VIDEO_13818_STREAM_TYPE:				
                    case VISUAL_MPEG4_STREAM_TYPE:
                    case VIDEO_H264_STREAM_TYPE:
                    case AUDIO_11172_STREAM_TYPE:
                    case AUDIO_13818_STREAM_TYPE:
                          espids[*es_pid_num].pid = esi.elementary_pid;
                          espids[*es_pid_num].type = esi.stream_type;
                          break;
                    default:       
                          espids[*es_pid_num].pid = esi.elementary_pid;
                          espids[*es_pid_num].type = 0;        
              
              }
        }        
        memcpy(tmp + es_cads->size, ptr, 5);
        tmp[es_cads->size+1] &= 0x1f; //remove reserved value ???
        tmp[es_cads->size+3] &= 0x0f; //remove reserved value ???

        int es_info_len_pos = es_cads->size+3; //mark length position to set it later
        int cur_len = 0; //current ES stream descriptor length

        es_cads->size += 5;
        ptr += 5;
        buf_len -= 5;
        len=esi.es_info_length;
        while(len > 0) {
           int dtag = ptr[0];
           int dlen = ptr[1] + 2; //2 bytes for tag and len
           
           if (dlen > len || dlen > MAX_DESC_LEN) {
                info("PMT: Invalide CA desc. length!\n");
                err = -1;
                break;
           }
           
           if (dtag == 0x09) { //we have CA descriptor
               memcpy(tmp + es_cads->size, ptr, dlen);
               es_cads->size += dlen;   
               es_cads->cads++;           
               cur_len += dlen;
               *fta=0;
           }
           if (espids) {
               if (espids[*es_pid_num].type == 0) {
                       switch(dtag) {
                           case TeletextDescriptorTag:
                           case SubtitlingDescriptorTag:  
                           case AC3DescriptorTag:
                           case EnhancedAC3DescriptorTag:
                           case DTSDescriptorTag:
                           case AACDescriptorTag:
                                 espids[*es_pid_num].type = dtag;
                                 //go to next pid
                       }
               }            
           }
           
           ptr += dlen;      
           if (ptr >= buf + size) {
                info("PMT: Invalid Buffer offset !\n");
                err = -1;                   
                break;
           }
           
           len -= dlen;
           buf_len -= dlen;
        }
        if (err == -1) {
              break;
        }
        tmp[es_info_len_pos] = (cur_len >> 8) & 0xff;
        tmp[es_info_len_pos+1] = cur_len & 0xff;       
        if (espids) {
            if (espids[*es_pid_num].type) {
                  //go to next pid
                  (*es_pid_num)++;
                  if (*es_pid_num >= MAX_ES_PIDS) {
                        info ("ERROR: ES pids array index out bounds (pids %d sid %d)!\n", *es_pid_num, pmt_hdr->program_number);
                        break;
                  }          
            }                 
        }
      }
      
      //parsing ok we can take this ES level descriptors
      if (((es_cads->cads && es_cads->size) || (pm_cads->cads && es_cads->size)) || *fta) { //take ES stream info if we have PM or ES desc. 
           es_cads->cad = (unsigned char*)malloc(sizeof(unsigned char)*es_cads->size);
           if (!es_cads->cad) {
               info("ERROR: parse_ca_desc() : out of memory\n");
               if (pm_cads->cad) 
                   free(pm_cads->cad);
               return -1;
           }
           memcpy(es_cads->cad, tmp, es_cads->size);
           
      }
       
#ifdef DBG
      info("%d bytes remaining\n",buf_len);    
#endif

      pmt_hdr->crc32=(ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]; 

      if (len < 0 || err == -1) {
        info("ERROR: parse_ca_desc() : section index out of bounds %d or (CRC err.) crc in sec. = 0x%x crc calc. = 0x%lx\n", buf_len,pmt_hdr->crc32, crc);
#ifdef DBG 
        print_pmt(&pmt_hdr);
#endif
        //cleanup ...
        if (pm_cads->cad) 
            free(pm_cads->cad);
        if (es_cads->cad)
            free(es_cads->cad);
        *es_pid_num = 0;
        memset(pm_cads,0,sizeof(si_ca_pmt_t));
        memset(es_cads,0,sizeof(si_ca_pmt_t));  
        return -1;
      }

#ifdef DBG
      info("#####################################\n");
      info("parse_ca_desc(): section parsed: OK !\n");  
#endif
      return 0;
}  
//-----------------------------------------------------------------------------------
int parse_cat_sect(unsigned char *buf, int size, si_cad_t *emm)
{
          unsigned char *ptr=buf;
          int len,i,ret;
          cat_t c;

          c.table_id = ptr[0];		         
          c.section_syntax_indicator = (ptr[1] >> 7) & 1;
          c.reserved_1 = (ptr[1] >> 4) & 3; 
          c.section_length = ((ptr[1] << 8) | ptr[2]) & 0xfff;

          if (c.section_length < 9 || c.section_length > 1021 || (int)c.section_length > size) {
                info("CAT: Invalid section length!\n");
                return -1;          
          }

#ifdef CRC32_CHECK
          u_long crc = dvb_crc32 ((char *)buf,c.section_length+3);  
#ifdef DBG
          info("CRCcc: 0x%lx\n",crc);
#endif
          if (crc & 0xffffffff) {
            info("CAT:CRC32 error (0x%lx)!\n",crc);
            return -1;
          }
#endif

          c.reserved_2 = (ptr[3] << 10) | (ptr[4] << 2) | ((ptr[5] >> 6) & 3);		        
          c.version_number = (ptr[5] >> 1) & 0x1f;		        
          c.current_next_indicator = ptr[5] & 1;	        
          c.section_number = ptr[6];		        
          c.last_section_number = ptr[7];	        


          //do desc. here
          len = c.section_length - 5;
          ptr+=8; //go after hdr.

          i = len;
          while(i > 4) { //crc32 4 bytes
                ret = descriptor(ptr, emm);
                if (ret < 0) {
                    info ("cannot parse CA descriptor in CAT !\n");
                    return -1;
                }
                i-=ret;
                ptr+=ret;
                if (ptr >= buf + size) {
                     info("CAT: Invalid Buffer offset !\n");
                     break;
                }
          }
          if (i != 4) {
              info("CAT: index out of bounds !\n");
              return -1;                
          }               
#ifdef DBG  
          info("%d bytes remaining (program info len = %d bytes)\n",len-i,len);    
#endif  
          c.crc32 = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]; 


          return 0;
}
//-----------------------------------------------------------------------------------
int parse_pat_sect(unsigned char *buf, int size, pmt_pid_list_t *pmt)
{
        unsigned char *ptr=buf;
        pat_t p;
        pat_list_t *pat_info = NULL;
        
        memset(&p,0,sizeof(p));

        p.table_id=ptr[0];
        p.section_syntax_indicator=(ptr[1] & 0x80) >> 7;
        p.reserved_1=(ptr[1] & 0x30) >> 4;
        p.section_length=((ptr[1] << 8) | ptr[2]) & 0x0fff;

        if (p.section_length < 9 || p.section_length > 1021 || (int)p.section_length > size) {
              info("PAT: Invalid section length !\n");
              return -1;
        
        }

#ifdef CRC32_CHECK                
        u_long crc = dvb_crc32 ((char *)buf,p.section_length+3);  
        //FIXME: is it the right way ?
        if (crc & 0xffffffff) {
          info("PAT:CRC32 error (0x%lx)!\n",crc);
          return -1;
        }
#endif
        
        p.transport_stream_id=(ptr[3] << 8) | ptr[4];
        p.reserved_2=(ptr[5] & 0xc0) >> 6;
        p.version_number=(ptr[5] & 0x3e) >> 1;
        p.current_next_indicator=(ptr[5] & 1);
        p.section_number=ptr[6];
        p.last_section_number=ptr[7];

        int n,i,pmt_num;
        
        n = p.section_length - 5 - 4; //bytes following section_length field + crc32 chk_sum

        ptr+=8;
        pmt_num=0;
        if (n > 0 && ((ptr + n) < (buf + size))) {
            pat_info=(pat_list_t *)malloc(sizeof(pat_list_t)*n/4);
            if (!pat_info) {
                info ("PAT: out of memory\n");
                return -1;
            }
            for(i=0;i<n;i+=4) {
              pat_list_t *pat = pat_info + pmt_num;
              pat->program_number=(ptr[0] << 8) | (ptr[1]);
              pat->reserved=(ptr[2] & 0xe0) >> 5;
              pat->network_pmt_pid=((ptr[2] << 8) | ptr[3]) & 0x1fff;
              if (pat->network_pmt_pid != 0x10) { //NIT => FIXME: remove other known pids
          //      memset(&pat->desc,0,sizeof(pmt_desc_list_t));
                  pmt_num++;
              }
              ptr+=4;
            }

            p.crc32=(ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]; 
            if (n != pmt_num)
                pat_info=(pat_list_t *)realloc(pat_info,sizeof(pat_list_t)*pmt_num);
            if (!pat_info) {
              info("parse_pat_sect():realloc error\n");
              return -1;
            }
        }        
        if (pmt) {
          pmt->p=p;
          pmt->pl=pat_info;
          pmt->pmt_pids=pmt_num;
        }
        
        return 0;
}
int parse_tdt_sect(unsigned char *buf, int size, tdt_sect_t *tdt)
{
          unsigned char *ptr = buf;
                
          tdt->table_id=ptr[0];
          tdt->section_syntax_indicator=(ptr[1] & 0x80) >> 7;
          tdt->reserved_1=(ptr[1] >> 4) >> 3;
          tdt->section_length=((ptr[1] << 8) | ptr[2]) & 0x0fff;            
          
          if (tdt->section_length != 5) {
                info("TDT: Invalid section length !\n");
                return -1;
          }
          
          //copy UTC time MJD + UTC
          memcpy(tdt->dvbdate, ptr + 3, 5);  

          return 0;

}
//-----------------------------------------------------------------------------------
//TS packets handling
int get_ts_packet_hdr(unsigned char *buf, ts_packet_hdr_t *p)
{
        unsigned char *ptr=buf;

        memset(p,0,sizeof(p));

        p->sync_byte=ptr[0]; 
        p->transport_error_indicator=(ptr[1] & 0x80) >> 7;
        p->payload_unit_start_indicator=(ptr[1] & 0x40) >> 6;
        p->transport_priority=(ptr[1] & 0x20) >> 5;
        p->pid=((ptr[1] << 8) | ptr[2]) & 0x1fff;
        p->transport_scrambling_control=(ptr[3] & 0xC0) >> 6;
        p->adaptation_field_control=(ptr[3] & 0x30) >> 4;
        p->continuity_counter=(ptr[3] & 0xf);
          
#ifdef DBG
        print_ts_header(p);
#endif  

        return 0;

}
//-----------------------------------------------------------------------------------
int ts2psi_data(unsigned char *buf,psi_buf_t *p,int len, int pid_req)
{
        unsigned char *b=buf;
        ts_packet_hdr_t h;


        get_ts_packet_hdr(buf,&h);
       
        b+=4;
        len-=4;
        
        if (h.sync_byte != 0x47) {
#ifdef SERVER
              sys("%s:No sync byte in header !\n",__FUNCTION__);
#endif          
              return -1;
        }

        
        if (pid_req != (int)h.pid) {
#ifdef DBG
              info("%s:pids mismatch  (pid req = %#x ts pid = %#x )!\n", __FUNCTION__,pid_req, h.pid);
#endif
              return -1;
        }
        
        //FIXME:Handle adaptation field if present/needed 
        if (h.adaptation_field_control & 0x2) {
          int n;
          
          n=b[0]+1;
          b+=n;
          len-=n; 

        }
          
        if (h.adaptation_field_control & 0x1) {
          if (h.transport_error_indicator) {
#ifdef DBG
            info("Transport error flag set !\n");
#endif
            return -1;
          }
          if (h.transport_scrambling_control) {
#ifdef DBG
            info("Transport scrambling flag set !\n");
#endif          
            //return -1;
          }   
          
          if (h.payload_unit_start_indicator && p->start) { //whole section new begin packet
#ifdef DBG      
              info("%s:section read !\n",__FUNCTION__);
#endif
              return 1;
          }
          
          if (h.payload_unit_start_indicator && !p->start) { //packet beginning
            int si_offset=b[0]+1; //always pointer field in first byte of TS packet payload with start indicator set
            b+=si_offset;
            len-=si_offset; 
            if (len < 0 || len > 184) {
#ifdef DBG
                  info("WARNING 1: TS Packet damaged !\n");
#endif          
                  return -1;
            }
            //move to buffer              
            memcpy(p->buf,b,len);
            p->len=len;
            p->start=((b[1] << 8) | b[2]) & 0x0fff; //get section length, using start for length
            p->pid=h.pid;
            p->continuity=h.continuity_counter;

          } 

          if (!h.payload_unit_start_indicator && p->start) { //packet continuation
            //duplicate packet
            if ((p->pid == (int)h.pid) && (p->continuity == (int)h.continuity_counter)){
#ifdef DBG
              info("Packet duplicate ???\n");      
#endif
              return -1;
            }
            //new packet
            if (p->pid != (int)h.pid) {
#ifdef DBG
              info("New pid buf start %d len %d bytes (pid in buf = %d pid in ts = %d) !\n", p->start,p->len, p->pid, h.pid);
#endif
              return -1;
            }
            //discontinuity of packets
            if (((++p->continuity)%16) != (int)h.continuity_counter) {
#ifdef DBG
              info("Discontinuity of ts stream !!!\n");
#endif
              return -1;        
            }
            p->continuity=h.continuity_counter;
            if (len < 0 || len > 184) {
                  info("WARNING 2: TS Packet damaged !\n");
                  return -1;
            }
            //move to buffer
            memcpy(p->buf+p->len,b,len);      
            p->len+=len; //FIXME: circular buffer
            if (p->len + 188 > PSI_BUF_SIZE) {
              info("Error: Buffer full !\n");
              return -1;
              //FIXME:realloc
            }
          }
        }
        
#if 1  
        //3 bytes for bytes containing table id and section length
        TS_SECT_LEN(b);
        if (slen+3 <= len && h.payload_unit_start_indicator)  //len = 188 bytes - 4 bytes ts hdr. - adapt. field bytes - 1 byte offset - offset
              return 1;
#else //possible opt.
        /*if (p->start+3 == len)  
              return 1;*/
#endif  

        return 0;
}
//TS packets handling end
//-----------------------------------------------------------------------------------



