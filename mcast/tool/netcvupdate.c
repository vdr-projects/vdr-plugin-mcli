/*------------------------------------------------------------------------
 * netcvupdate - NetCeiver update tool
 *
 * Principle for firmware update
 * - Unpack given .tgz into host /tmp/mkdtemp()
 * - "md5sum -c md5sums.txt"
 * - read script file update.scr
 * - feed commands into tnftp 
 *
 *
 *------------------------------------------------------------------------*/
#define USE_MCLI_API

#ifdef USE_MCLI_API
#include "headers.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#endif

#define STATE_FILE "update.log"
#define FTP_CMD "ftp"
#define NC_CONFPATH "/mmc/etc/"
#define NC_CONFFILE "netceiver.conf"

char ftp_cmd[512]=FTP_CMD;
int verbose=1;
int no_reboot=0;
char username[256]="root";
char password[256]="root";
char device[256]="eth0";
char *uuids[256]={0};
char *versions[256]={0};
int num_uuids=0;
char socket_path[256]=API_SOCK_NAMESPACE;
int ftp_client_lftp = 0;
int debug = 0;

#ifdef USE_MCLI_API
/*------------------------------------------------------------------------*/
#define API_WAIT_RESPONSE(cmd) { cmd->state=API_REQUEST; \
		send (sock_comm, &sock_cmd, sizeof(api_cmd_t), 0); \
		recv (sock_comm, &sock_cmd, sizeof(api_cmd_t), 0); \
		if (cmd->state == API_ERROR) {warn ( "SHM parameter error, incompatible versions?\n"); exit(-1);}}

int sock_comm;

int api_init(char *path)
{
        int sock_name_len = 0;
        struct sockaddr_un sock_name;
        sock_name.sun_family = AF_UNIX;

        strcpy(sock_name.sun_path, path);
        sock_name_len = sizeof(struct sockaddr_un);
        
        if((sock_comm = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        {
                warn ("socket create failure %d\n", errno);
                return -1;
        }

        if (connect(sock_comm, (struct sockaddr*)&sock_name, sock_name_len) < 0) {
                warn ("connect failure to %s: %s (are you root?)\n",path, strerror(errno));
		return -1;
        }
	return 0;
}
#endif
/*------------------------------------------------------------------------*/
void add_upload(char* cmd, char* localfile, char *remotepath, char *remotefile)
{
	char tmp[1024];
	sprintf(tmp,
		"cd %s\n"
		"site exec rm -f /tmp/update/%s\n"
		"put %s\n",		
		remotepath, remotefile, localfile);
	strcat(cmd,tmp);
}
/*------------------------------------------------------------------------*/
void add_download(char* cmd, char *remotepath, char *remotefile)
{
	char tmp[1024];
	sprintf(tmp,
		"cd %s\n"
		"get %s\n",
		remotepath,remotefile);
	strcat(cmd,tmp);
}
/*------------------------------------------------------------------------*/
int script_interpreter(char *script, char *line, 
		       char *ip, char *iface, char *user, char *pwd)
{
	char cmd[256],p1[256],p2[256];
	int end=0;

	*cmd=0;
	*p1=0;
	*p2=0;
	sscanf(line,"%s %s %s\n",cmd,p1,p2);

	if (cmd[0]=='#')
		return 0;
	if (!strcmp(cmd,"connect")) {
		char tmp[1024];
		sprintf(tmp,
			"open %s%%%s\n"
			"user %s %s\n",
			ip,iface,user,pwd);
		strcat(script,tmp);
	}
	else if (!strcmp(cmd,"upload")) {
		add_upload(script,p2,p1,p2);
	}
	else if (!strcmp(cmd,"download")) {
		add_download(script,p1,p2);
	}
	else if (!strcmp(cmd,"exit")) {
		strcat(script,"quit\n");
		end=1;
	}
	else {
		strcat(script,line);
	}
	return end;
}
/*------------------------------------------------------------------------*/
char script[128*1024];
int generate_script(char *filename, char *tmpname, char *ip, char *iface, char *user, char *pwd)
{
	FILE *f;
	f=fopen(filename,"r");
	if (!f) {
		fprintf(stderr,"Can't open script file <%s>: %s\n",filename,strerror(errno));
		return -1;
	}
	script[0]=0;

	while(!feof(f)) {
		char line[256];
		char *r = fgets(line,255,f);
		if (r == NULL) break;
		if (script_interpreter(script,line,ip,iface,user,pwd))
			break;
	}
	fclose(f);
	return 0;
}
/*------------------------------------------------------------------------*/
int check_state_file(char *tmpname)
{
	char cmd[512];
	int ret;
	printf("\nUPDATE RESULT:\n");
	snprintf(cmd,512,"cat %s/update.log",tmpname);
	ret=system(cmd);
	printf("\n");
	return ret;
}
/*------------------------------------------------------------------------*/
void sigground(int x)
{
}
/*------------------------------------------------------------------------*/
int run_ftp(char *tmpdir,char *script, int timeout, char *pipeout)
{
	FILE *f;
	char cmd[1024]; // avoid -Wformat-truncation
	int ret;
	if (!strlen(ftp_cmd))
		return -1;
	signal(SIGPIPE,sigground);
	if (strlen(tmpdir)) {
//		snprintf(cmd, sizeof(cmd),"cd %s; %s -q %i -n %s",tmpdir,ftp_cmd,timeout,verbose?"":"-V");
		if (ftp_client_lftp == 1) {
			snprintf(cmd, sizeof(cmd),"cd %s; %s --norc %s %s",tmpdir,ftp_cmd, (verbose == 0) ? "" : "-d", pipeout);
		} else {
			snprintf(cmd, sizeof(cmd),"cd %s; %s -n %s %s",tmpdir,ftp_cmd, (verbose == 0) ? "" : "-V", pipeout);
		};
	} else {
		if (ftp_client_lftp == 1) {
			snprintf(cmd, sizeof(cmd),"%s --norc %s %s",ftp_cmd,(verbose == 0) ? "" : "-d", pipeout);
		} else {
			snprintf(cmd, sizeof(cmd),"%s -q %i -n %s %s",ftp_cmd,timeout, (verbose == 0) ? "" : "-V", pipeout);
		};
	};

	if (debug > 0) {
		fprintf(stderr, "\nDEBUG : execute FTP command: %s\n", cmd);
	};
	
	f=popen(cmd,"w");
	if (!f)
		return -1;
	if (debug > 0) {
		fprintf(stderr, "DEBUG : execute FTP script\n------\n%s\n------\n", script);
	}
	fputs(script,f);
	ret=pclose(f);
	return ret;
}
/*------------------------------------------------------------------------*/
int do_reboot(char *tmpdir, char *ip, char* iface, char *user, char* pwd)
{
	sprintf(script,
		"open %s%%%s\n"
		"user %s %s\n"
		"site exec reboot -d 5\n"
		"quit\n"
		,
		ip,iface,user,pwd);
	return run_ftp(tmpdir, script, 15,"");
}
/*------------------------------------------------------------------------*/

int do_list_fw(char *tmpdir, char *ip, char* iface, char *user, char* pwd, int maxf, int *found, char **versions)
{
	char tmpfile[256]="/tmp/ncvup.XXXXXX";
	char pipeout[512]; // avoid -Wformat-truncation
	int n=0;
	int ret=0;	
	FILE *file;

	*found=0;

	if (!mkstemp(tmpfile)) {
		fprintf(stderr,"Can't make temporary directory %s!\n",tmpfile);
		return -2;
	}

	sprintf(script,
		"open %s%%%s\n"
		"user %s %s\n"
		"ls /mmc/\n"
		"quit\n"
		,
		ip,iface,user,pwd);
	sprintf(pipeout," > %s",tmpfile);
	ret=run_ftp(tmpdir, script, 15, pipeout);
	if (ret) {
		unlink(tmpfile);
		return ret;
	}

	file=fopen(tmpfile,"r");
	if (!file) {
		unlink(tmpfile); // ?
		perror("Can't read temp file");
		return ret;
	}

	while(!feof(file)) {
		char line[1024];
		char *p;
		*line=0;
		char *r = fgets(line, 1023,file);
		if (r == NULL) break;
		line[1023]=0;
		p=strstr(line,"etceivr.");
		if (p) {

			char *pp=strchr(p,'\n');
			if (pp)
				*pp=0;

			if (n < maxf) {
				n++;
				*versions++=strdup(p-1);
			}
		}
	}
	*found=n;
	fclose(file);
	unlink(tmpfile);
	return 0;
}
/*------------------------------------------------------------------------*/
int do_kill(char *tmpdir, char *ip, char* iface, char *user, char* pwd)
{
	sprintf(script,
		"open %s%%%s\n"
		"user %s %s\n" 
		"site exec killall -9 mserv\n"  
		"quit\n"
		,
		ip,iface,user,pwd);
	return run_ftp(tmpdir, script, 15,"");
}
/*------------------------------------------------------------------------*/
int do_single_update(char *tmpdir, char *uuid, char *device)
{
	char path[256];
	int ret;

	snprintf(path,255,"%s/%s",tmpdir,"update.scr");
	if (generate_script(path, tmpdir, uuid, device, username, password))
		return -1;
//  	puts(script);

	printf("Upload update... \n");
	fflush(stdout);

	ret=run_ftp(tmpdir, script, 600,"");
	if (ret)
		return ret;

	printf("check result... \n");
	fflush(stdout);

	if (check_state_file(tmpdir))
		return -1;

#if 1
	if (!no_reboot) {
		printf("Issue Reboot... ");
		fflush(stdout);
		ret=do_reboot(tmpdir, uuid, device, username, password);
		if (!ret) 
			return ret;
	}
#endif
	return 0;
}
/*------------------------------------------------------------------------*/
int do_single_upload( char *uuid, char *device, char *remote_path, char *fname, char *remote_file)
{
	int ret;
	sprintf(script,
		"open %s%%%s\n"
		"user %s %s\n"
		"cd %s\n"
		"put %s %s\n"
//		"site exec killall -HUP mserv\n"
		"quit",
		uuid,device,username,password,remote_path,fname,remote_file);
	ret=run_ftp("", script, 120,"");
	return ret;
}
/*------------------------------------------------------------------------*/
int do_single_download( char *uuid, char *device, char *remote_path, char *fname)
{
	int ret;
	sprintf(script,
		"open %s%%%s\n"
		"user %s %s\n"
		"cd %s\n"
		"get %s\n"
		"quit",
		uuid,device,username,password,remote_path,fname);
	ret=run_ftp("", script, 120,"");
	return ret;
}
/*------------------------------------------------------------------------*/
int fw_action(char *uuid, char* iface, char *user, char* pwd, int mode, char *version)
{
	int ret;
	if (mode==0) { // inactivate
		printf("Inactivating version %s\n",version);
		sprintf(script,
			"open %s%%%s\n"
	                "user %s %s\n"
        	        "cd /mmc\n"
			"rename netceivr.%s xetceivr.%s\n"
        	        "quit",
			uuid,device,username,password,version,version);
		ret=run_ftp("", script, 120,"");
		return ret;
	}
	else if (mode==1) { // enable
		printf("Enabling version %s\n",version);
		sprintf(script,
			"open %s%%%s\n"
	                "user %s %s\n"
        	        "cd /mmc\n"
			"rename xetceivr.%s netceivr.%s\n"
        	        "quit",
			uuid,device,username,password,version,version);
		ret=run_ftp("", script, 120,"");
		return ret;
	}
	else if (mode==2) { // delete
		printf("Removing version %s\n",version);
		sprintf(script,
			"open %s%%%s\n"
	                "user %s %s\n"
			"site exec rm -rf /mmc/netceivr.%s\n"
			"site exec rm -rf /mmc/xetceivr.%s\n"
        	        "quit",
			uuid,device,username,password,version,version);
		ret=run_ftp("", script, 120,"");
		return ret;
	}
	return 0;
}
/*------------------------------------------------------------------------*/
int cleanup(char *tmpdir)
{
	int ret;
	char cmd[1024];
	snprintf(cmd,1024,"rm -rf '%s'",tmpdir);
//	puts(cmd);
	ret=system(cmd);
	return ret;	
}
/*------------------------------------------------------------------------*/
int unpack(char *tmpdir, char *file)
{
	int ret;
	char cmd[1024];
	snprintf(cmd,1024,"tar xfz '%s' --directory %s",file,tmpdir);
//	puts(cmd);
	ret=system(cmd);
	return ret;
}
/*------------------------------------------------------------------------*/
int check_xml(char *file)
{
	int ret;
	char cmd[1024];
	snprintf(cmd,1024,"xmllint --noout '%s'\n",file);
//	puts(cmd);
	ret=system(cmd);
	return ret;
}
/*------------------------------------------------------------------------*/
int check_integrity(char *tmpdir)
{
	int ret;
	char cmd[1024];
	snprintf(cmd,1024,"cd %s; md5sum -c --status md5sums.txt \n",tmpdir);
//	puts(cmd);
	ret=system(cmd);
	return ret;	
}
/*------------------------------------------------------------------------*/
int do_update(char **uuids, int num_uuids, char *device, char *optarg)
{
	char tmpdir[256]="/tmp/ncvupXXXXXX";
	int n;
	int ret=0;
	if (!mkdtemp(tmpdir)) {
		fprintf(stderr,"Can't make temporary directory %s!\n",tmpdir);
		return -2;
	}
//	printf("TEMP DIR %s\n",tmpdir);
	if (unpack(tmpdir,optarg)) {
		fprintf(stderr,"Update file <%s> cannot be unpacked!\n",optarg);
		cleanup(tmpdir);
		return -2;
	}
	if (check_integrity(tmpdir)) {
		fprintf(stderr,"Update file <%s> corrupted!\n",optarg);
		cleanup(tmpdir);
		return -2;
	}
	printf("Update file integrity OK\n");
	printf("NUM uuids %i\n",num_uuids);
	for(n=0;n<num_uuids;n++) {
		if (!uuids[n])
			continue;

		printf("UUID %s: ",uuids[n]);
		fflush(stdout);
		ret=do_single_update(tmpdir, uuids[n], device);
		if (!ret)
			printf("-> Update done <-\n");
		else {
			printf("-> Update failed (ret=%i) <-\n",ret);
			uuids[n]=NULL;
		}
	}

	cleanup(tmpdir);
	return ret;
}
/*------------------------------------------------------------------------*/
int do_upload(char **uuids, int num_uuids, char *device, char *optarg)
{
	int n;
	int ret=0;
	if (check_xml(optarg)) {
		fprintf(stderr,"Configuration file <%s> not valid XML\n",optarg);
		return -2;
	}
	for(n=0;n<num_uuids;n++) {
		if (!uuids[n])
			continue;

		printf("UUID %s: Uploading %s ... \n",uuids[n], optarg);
		fflush(stdout);
		ret=do_single_upload(uuids[n], device, "/mmc/etc/", optarg, NC_CONFFILE);
		if (!ret)
			printf("Upload done\n");
		else {
			printf("Upload failed (ret=%i)\n",ret);
			uuids[n]=NULL;
		}
	}
	return ret;	
}
/*------------------------------------------------------------------------*/
int do_download(char **uuids, int num_uuids, char *device, char *remotepath, char *file)
{
	int n,ret=0;

	for(n=0;n<num_uuids;n++) {
		char newfile[2048];
		if (!uuids[n])
			continue;

		if (num_uuids!=1)
			snprintf(newfile,sizeof(newfile),"%s-%s",file,uuids[n]);
		else
			snprintf(newfile,sizeof(newfile),"%s", file);

		printf("UUID %s: Downloading %s ... \n",uuids[n], newfile);
		fflush(stdout);
		ret=do_single_download(uuids[n], device, remotepath, file);
		if (!ret) {
			printf("Download done\n");
			if (num_uuids!=1)
				rename(file,newfile);
		}
		else {
			printf("Download failed (ret=%i)\n",ret);
			uuids[n]=NULL;
		}
	}
	return ret;	
}
/*------------------------------------------------------------------------*/
int do_all_reboot(char **uuids, int num_uuids, char *device)
{
	int n,ret=0;

	for(n=0;n<num_uuids;n++) {
		if (!uuids[n])
			continue;
		printf("UUID %s: Issue Reboot... ",uuids[n]);
		fflush(stdout);
		ret=do_reboot("/tmp", uuids[n], device, username, password);
		if (!ret)
			printf("Reboot done\n");
		else
			printf("Reboot failed (ret=%i)\n",ret);
	}
	return ret;
}
/*------------------------------------------------------------------------*/
int do_all_kill(char **uuids, int num_uuids, char *device)
{
	int n,ret=0;

	for(n=0;n<num_uuids;n++) {
		if (!uuids[n])
			continue;
		printf("UUID %s: Issue Kill... ",uuids[n]);
		fflush(stdout);
		ret=do_kill("/tmp", uuids[n], device, username, password);
		if (!ret)
			printf("Kill done\n");
		else
			printf("Kill failed (ret=%i)\n",ret);
	}
	return ret;
}
/*------------------------------------------------------------------------*/
int get_uuids(char **uuids, int max)
{
	int count,n;
	api_cmd_t sock_cmd;
        api_cmd_t *api_cmd=&sock_cmd;

	if (api_init(socket_path)==-1) {
		exit(-1);
	}
	api_cmd->cmd=API_GET_NC_NUM;
	api_cmd->magic = MCLI_MAGIC;
	api_cmd->version = MCLI_VERSION;
        API_WAIT_RESPONSE(api_cmd);
	if(api_cmd->magic != MCLI_MAGIC || api_cmd->version != MCLI_VERSION) {
		err("API version mismatch!\n");
	}
	count=api_cmd->parm[API_PARM_NC_NUM];
	
	for(n=0;n<max && n<count;n++) {
		api_cmd->cmd=API_GET_NC_INFO;
                api_cmd->parm[API_PARM_NC_NUM]=n;
                API_WAIT_RESPONSE(api_cmd);
                if(api_cmd->u.nc_info.magic != MCLI_MAGIC || api_cmd->u.nc_info.version != MCLI_VERSION) {
                	err("API version mismatch!\n");
                }
                
		uuids[n]=strdup(api_cmd->u.nc_info.uuid);
		versions[n]=strdup(api_cmd->u.nc_info.FirmwareVersion);		
	}
	return count;
}
/*------------------------------------------------------------------------*/
int show_uuids(void)
{
	char *uuids[256]={0};
	int num_uuids,n;
	num_uuids=get_uuids(uuids,256);
	for(n=0;n<num_uuids;n++) {
		printf("%s    %s\n",uuids[n],versions[n]);
	}
	return 0;
}
/*------------------------------------------------------------------------*/
#define MAX_FWS 64
void show_firmwares(char *uuid, char *device, char *username, char *passwd)
{
	char *fwversions[MAX_FWS];
	int found,m;
	found=0;

	do_list_fw("/tmp", uuid, device, username, password, MAX_FWS, &found, fwversions);

	printf("Firmware versions found: %i\n     Versions: ", found);
	for(m=0;m<found;m++) {
		if (m!=0) printf(", ");
		if (fwversions[m][0]!='n')
			printf("%s (disabled)",fwversions[m]+9);
		else
			printf("%s",fwversions[m]+9);

		free(versions[m]);
	}
	puts("");
}
/*------------------------------------------------------------------------*/

int show_all_firmwares(char **uuids, int num_uuids)
{
	int n;

	for(n=0;n<num_uuids;n++) {
		printf("%s: ",uuids[n]);
		fflush(stdout);
		show_firmwares(uuids[n],device,username,password);
	}
	return 0;
}
/*------------------------------------------------------------------------*/
void do_fw_actions(char **uuids, int max, int mode, char *version)
{
	int n;
	
	if (strlen(version)!=3 || !strcmp(version,"000")) {
		fprintf(stderr,"Invalid version number\n");
		return;
	}

	for(n=0;n<3;n++)
		version[n]=toupper(version[n]);

	for(n=0;n<max;n++) {
		printf("UUID %s\n",uuids[n]);
		if (fw_action(uuids[n], device, username, password, mode, version)) {
			fprintf(stderr,"Failed\n");
			return;
		}
		show_firmwares(uuids[n],device,username,password);
	}
}
/*------------------------------------------------------------------------*/
void usage(void)
{
	fprintf(stderr,
		"netcvupdate - NetCeiver update tool, version " MCLI_VERSION_STR "\n"
		"(c) BayCom GmbH\n"
		"Usage: netcvupdate <options> <actions> \n"
		"Actions:  \n" 
		"           -l                List all seen NetCeivers and their UUID\n"
		"           -L                List available FWs\n"
		"           -X <Update.tgz>   Update with given file\n"
		"           -U <configfile>   Upload configfile\n"
		"           -D                Download configfile netceiver.conf\n" 
		"           -I <version>      Inactivate FW version\n"
		"           -E <version>      Enable FW version\n"
		"           -Z <version>      Remove FW version\n"
		"           -K                Restart streaming server\n"
		"           -R                Issue reboot\n"
		"Options:\n"
		"           -A                Use all found NetCeivers (mcli must be running)\n"
		"           -i <uuid>         Use specific UUID (can be used multiple times)\n"
		"           -e                Enable debugging\n"
		"           -h|?              This online help\n"
		"  *** Either -A or -i must be given for most actions! ***\n"
		"Rare options:\n"
		"           -d <device>       Set network device (default: eth0)\n"
		"           -F <ftp-command>  Set ftp command/path\n"
		" *** ftp command must understand the -q (timeout) option in case '-n' is not used! ***\n"
		"           -P <path>         Set API socket\n"
		"           -u <user>         Set username\n"
		"           -p <password>     Set password\n"
		"           -r                No reboot after update\n"
		"           -n                Use next-generation FTP client 'lftp'\n"
		"           -q                Be more quiet\n"
		);
	exit(0);
}
/*------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
	int ret=0;
	
	while(1) {
		int ret = getopt(argc,argv, "h?U:X:Di:AlLI:E:Z:d:F:P:u:p:rRqKne");
		if (ret==-1)
			break;
			
		char c=(char)ret;

		switch(c) {
		case 'F':
			strncpy(ftp_cmd,optarg,512);
			ftp_cmd[511]=0;
			break;
		case 'X':
			ret=do_update(uuids, num_uuids, device, optarg);
			if (ret==-2)
				exit(ret);
			break;
		case 'U':
			ret=do_upload(uuids, num_uuids, device, optarg);
			if (ret==-2)
				exit(ret);
			break;
		case 'D':
			ret|=do_download(uuids, num_uuids, device, NC_CONFPATH, NC_CONFFILE);
			break;
		case 'i':
			uuids[num_uuids]=strdup(optarg);
			num_uuids++;
			break;
		case 'A':
			num_uuids=get_uuids(uuids,255);
			break;
		case 'l':
			show_uuids();
			break;
		case 'd':
			strncpy(device,optarg,255);
			device[255]=0;
			break;
		case 'P':
			strncpy(socket_path,optarg,255);
			socket_path[255]=0;
			break;
		case 'p':
			strncpy(password,optarg,255);
			password[255]=0;
			break;
		case 'u':
			strncpy(username,optarg,255);
			username[255]=0;
			break;
		case 'r':
			no_reboot=1;
			break;
		case 'K':
			ret|=do_all_kill(uuids,num_uuids,device);
			break;	
		case 'R':
			ret|=do_all_reboot(uuids, num_uuids, device);
			break;
		case 'L':
			show_all_firmwares(uuids, num_uuids);
			break;
		case 'I':
			do_fw_actions(uuids, num_uuids, 0, optarg);
			break;
		case 'E':
			do_fw_actions(uuids, num_uuids, 1, optarg);
			break;
		case 'Z':
			do_fw_actions(uuids, num_uuids, 2, optarg);
			break;			
		case 'q':
			verbose = 0;
			fprintf(stderr, "INFO  : disable verbose mode\n");
			break;
		case 'n':
			ftp_client_lftp = 1;
			snprintf(ftp_cmd, sizeof(ftp_cmd), "%s", "lftp");
			fprintf(stderr, "INFO  : enable options for FTP client 'lftp'\n");
			break;
		case 'e':
			debug = 1;
			fprintf(stderr, "DEBUG : enable debugging\n");
			break;
		case 'h':
		case '?':
		default:
			usage();
			break;
		}
	}
	exit(ret);
}
