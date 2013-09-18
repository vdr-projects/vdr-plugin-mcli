#include "headers.h"

#ifdef __MINGW32__
#include <getopt.h>
extern void bzero(void *s, size_t n);
#endif

#define HDR_CHK_PACKET_LENGTH 1424
#define HDR_CHK_LENGTH 16

static int quit=0;

void sighandler(int sig) {
        quit=1;
}

int main (int argc, char **argv)
{
	UDPContext *s;
	unsigned char buf[UDP_TX_BUF_SIZE];
	memset (buf, 0x55, sizeof (buf));
	int ret, len, i, mode = 1, mcg_num = 1, port = 23000, c, needhelp = 0, wait = 0, file = 0, loop = 0, header = 0, lost = 0;
	char mcg[10][1024];
	char *ifname = NULL;
	strcpy (mcg[0], "ff18:5100::");

	do {
		ret = getopt_long (argc, argv, "hrtp:g:xi:w:flH", NULL, NULL);
		if(ret<0) {
			break;
		}
		c=(char)ret;
		switch (c) {
		case 'i':
			ifname = optarg;
			break;
		case 'f':
			file = 1;
			break;
		case 'r':
			mode = 1;
			break;
		case 't':
			mode = 2;
			break;
		case 'x':
			mode = 3;
			break;
		case 'H':
			header = 1;
			break;
		case 'l':
			loop = 1;
			break;
		case 'p':
			port = atoi (optarg);
			break;
		case 'w':
			wait = atoi (optarg);
			break;
		case 'g':
			for (mcg_num = 0, optind--; optind < argc; optind++, mcg_num++) {
				if (argv[optind][0] != '-') {
					strcpy (mcg[mcg_num], argv[optind]);
				} else {
					break;
				}
			}
			break;
		case 'h':
			needhelp = 1;
			break;
		}
	}
	while (c >= 0);

	if (needhelp) {
		fprintf (stderr, "usage: netcvlogview -i <network interface> <-r|-t> -g <multicast groups> -p <port> -w <seconds timeout> -H\n");
		return -1;
	}

	fprintf (stderr, "mode:%d port:%d mcg:%d [ ", mode, port, mcg_num);
	for (i = 0; i < mcg_num; i++) {
		fprintf (stderr, "%s ", mcg[i]);
	}
	fprintf (stderr, "]\n");

#ifdef __MINGW32__
	recv_init (ifname, port);
#endif

	switch (mode) {

	case 1:
		s = client_udp_open_host (mcg[0], port, ifname);
		if (s) {
			struct sockaddr_in6 addr;
			addr.sin6_family = AF_INET6;
			addr.sin6_port = htons (port);

			for (i = 1; i < mcg_num; i++) {
				fprintf (stderr, "mcg: [%s]\n", mcg[i]);
				inet_pton (AF_INET6, mcg[i], &addr.sin6_addr);
				if (udp_ipv6_join_multicast_group (s->udp_fd, s->idx, (struct sockaddr *) &addr) < 0) {
					err ("Cannot join multicast group !\n");
				}

			}
			signal(SIGTERM, sighandler);
			signal(SIGINT, sighandler);
                        
                        FILE *f;
                        time_t first;
                        time_t last;
                        int hc, i;
			do {
        			first=last=hc=lost=0;
				if(file) {
					f=fopen("rawfile.temp", "wb");
					if(f==NULL) {
						perror("Cannot open file for writing\n");
						return -1;
					}
				} else {
					f=stdout;
				}
				while (!quit &&(!wait || !last || ((time(NULL)-last) < wait))) {
					len = udp_read (s, buf, sizeof (buf), 50, NULL);
					if(len>0) {
						if(header) {
						        if(len!=HDR_CHK_PACKET_LENGTH)  {
						                fprintf(stderr, "Expected header length mismatch %d != %d!\n", len, HDR_CHK_PACKET_LENGTH);
						        }
						        uint32_t *cnt=(uint32_t *)buf;
						        int hv=ntohl(*cnt);
						        if(hv == hc) {
                						fwrite (buf+HDR_CHK_LENGTH, len-HDR_CHK_LENGTH, 1, f);
                						hc++;
                                                        } else {
                                                                bzero(buf, HDR_CHK_PACKET_LENGTH);
                                                                for(i=hc; i<hv; i++) {
                                                                    fwrite(buf, HDR_CHK_PACKET_LENGTH-HDR_CHK_LENGTH, 1, f);
                                                                }
                                                                lost+=(hv-hc);
                                                                hc=i;
                                                        }
						} else {
        						fwrite (buf, len, 1, f);
                                                }
						last=time(NULL);	
						if(!first) {
						        first=last;
						}
					} 
				}
				fclose(f);
				if(file) {
				        if(quit) {
				                unlink("rawfile.temp");
				        } else {
	        				struct tm *now=localtime(&first);
        				        char fname[80];
		        		        sprintf(fname, "%04d%02d%02d-%02d%02d%02d.raw", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
			        	        rename("rawfile.temp", fname);
				                fprintf(stderr, "[%s] New log file: %s (%u packets)\n",ifname, fname, hc);
				                if(lost) {
				                        fprintf(stderr, "Warning: Lost %d frames of %d payload bytes, now filled with padding bytes (0)\n", lost, HDR_CHK_PACKET_LENGTH-HDR_CHK_LENGTH);
				                }
                                        }
				}
			} while(loop && ! quit);
			udp_close (s);
		}
		break;

	case 2:
		s = server_udp_open_host (mcg[0], port, ifname);
		if (s) {
			while (1) {
				if(!fread (buf, 1316, 1, stdin)) {
					break;
				}
				udp_write (s, buf, 1316);
			}
			udp_close (s);
		}
		break;

	case 3:
		s = server_udp_open_host (mcg[0], port, ifname);
		if (s) {
			int i;
			for (i = 0; i < sizeof (buf); i++) {
				buf[i] = rand ();
			}
			while (1) {
				i = rand ();
				udp_write (s, buf, ((i % 4) + 4) * 188);
			}
			udp_close (s);
		}
		break;
	}

	return 0;
}
