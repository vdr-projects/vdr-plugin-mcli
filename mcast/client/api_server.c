/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"

#if defined(API_SOCK) || defined(API_SHM) || defined (API_WIN)

static int process_cmd (api_cmd_t * api_cmd, tra_info_t * trl, netceiver_info_list_t * nci)
{
	if(api_cmd->magic != MCLI_MAGIC || api_cmd->version != MCLI_VERSION) {
	        api_cmd->magic = MCLI_MAGIC;
	        api_cmd->version = MCLI_VERSION;
	        api_cmd->state = API_RESPONSE;
	        return 0;
	}
	if (api_cmd->state != API_REQUEST) {
		return 0;
	}

	switch (api_cmd->cmd) {
	case API_GET_NC_NUM:
		api_cmd->parm[API_PARM_NC_NUM] = nci->nci_num;
		api_cmd->state = API_RESPONSE;
		break;

	case API_GET_NC_INFO:
		if (api_cmd->parm[API_PARM_NC_NUM] < nci->nci_num) {
			api_cmd->u.nc_info = nci->nci[api_cmd->parm[API_PARM_NC_NUM]];
			api_cmd->state = API_RESPONSE;
		} else {
			api_cmd->state = API_ERROR;
		}
		break;
	case API_GET_TUNER_INFO:
		if (api_cmd->parm[API_PARM_NC_NUM] < nci->nci_num) {
			if (api_cmd->parm[API_PARM_TUNER_NUM] < nci->nci[api_cmd->parm[API_PARM_NC_NUM]].tuner_num) {
				api_cmd->u.tuner_info = nci->nci[api_cmd->parm[API_PARM_NC_NUM]].tuner[api_cmd->parm[API_PARM_TUNER_NUM]];
				api_cmd->state = API_RESPONSE;
			} else {
				api_cmd->state = API_ERROR;
			}
		} else {
			api_cmd->state = API_ERROR;
		}
		break;
	case API_GET_SAT_LIST_INFO:
		if (api_cmd->parm[API_PARM_NC_NUM] < nci->nci_num) {
			if (api_cmd->parm[API_PARM_SAT_LIST_NUM] < nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list_num) {
				api_cmd->u.sat_list = nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list[api_cmd->parm[API_PARM_SAT_LIST_NUM]];
				api_cmd->state = API_RESPONSE;
			} else {
				api_cmd->state = API_ERROR;
			}
		} else {
			api_cmd->state = API_ERROR;
		}
		break;
	case API_GET_SAT_INFO:
		if (api_cmd->parm[API_PARM_NC_NUM] < nci->nci_num) {
			if (api_cmd->parm[API_PARM_SAT_LIST_NUM] < nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list_num) {
				if (api_cmd->parm[API_PARM_SAT_NUM] < nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list[api_cmd->parm[API_PARM_SAT_LIST_NUM]].sat_num) {
					api_cmd->u.sat_info = nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list[api_cmd->parm[API_PARM_SAT_LIST_NUM]].sat[api_cmd->parm[API_PARM_SAT_NUM]];
					api_cmd->state = API_RESPONSE;
				} else {
					api_cmd->state = API_ERROR;
				}
			} else {
				api_cmd->state = API_ERROR;
			}
		} else {
			api_cmd->state = API_ERROR;
		}
		break;

	case API_GET_SAT_COMP_INFO:
		if (api_cmd->parm[API_PARM_NC_NUM] < nci->nci_num) {
			if (api_cmd->parm[API_PARM_SAT_LIST_NUM] < nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list_num) {
				if (api_cmd->parm[API_PARM_SAT_NUM] < nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list[api_cmd->parm[API_PARM_SAT_LIST_NUM]].sat_num) {
					if (api_cmd->parm[API_PARM_SAT_COMP_NUM] < nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list[api_cmd->parm[API_PARM_SAT_LIST_NUM]].sat[api_cmd->parm[API_PARM_SAT_NUM]].comp_num) {
						api_cmd->u.sat_comp = nci->nci[api_cmd->parm[API_PARM_NC_NUM]].sat_list[api_cmd->parm[API_PARM_SAT_LIST_NUM]].sat[api_cmd->parm[API_PARM_SAT_NUM]].comp[api_cmd->parm[API_PARM_SAT_COMP_NUM]];
						api_cmd->state = API_RESPONSE;
					} else {
						api_cmd->state = API_ERROR;
					}
				} else {
					api_cmd->state = API_ERROR;
				}
			} else {
				api_cmd->state = API_ERROR;
			}
		} else {
			api_cmd->state = API_ERROR;
		}
		break;
	case API_GET_TRA_NUM:
		api_cmd->parm[API_PARM_TRA_NUM] = trl->tra_num;
		api_cmd->state = API_RESPONSE;
		break;
	case API_GET_TRA_INFO:
		if (api_cmd->parm[API_PARM_TRA_NUM] < trl->tra_num) {
			api_cmd->u.tra = trl->tra[api_cmd->parm[API_PARM_TRA_NUM]];
			api_cmd->state = API_RESPONSE;
		} else {
			api_cmd->state = API_ERROR;
		}
		break;
	default:
		api_cmd->state = API_ERROR;
	}
	return 1;
}
#endif
#ifdef API_SOCK
typedef struct
{
	pthread_t thread;
	int fd;
	struct sockaddr_un addr;
	socklen_t len;
	int run;
} sock_t;

static void *sock_cmd_loop (void *p)
{
	sock_t *s = (sock_t *) p;
	api_cmd_t sock_cmd;
	int n;
	netceiver_info_list_t *nc_list=nc_get_list();
	tra_info_t *tra_list=tra_get_list();

	dbg ("new api client connected\n");
	s->run = 1;
	while (s->run){
		n = recv (s->fd, &sock_cmd, sizeof (api_cmd_t), 0);
		if (n == sizeof (api_cmd_t)) {
			nc_lock_list();
			process_cmd (&sock_cmd, tra_list, nc_list);
			nc_unlock_list();
			send (s->fd, &sock_cmd, sizeof (api_cmd_t), 0);
		} else {
  	            sock_cmd.magic = MCLI_MAGIC;
                    sock_cmd.version = MCLI_VERSION;
                    sock_cmd.state = API_RESPONSE;
                    send (s->fd, &sock_cmd, sizeof (api_cmd_t), 0);
                    break;
		}
		pthread_testcancel();
	}

	close (s->fd);
	pthread_detach (s->thread);
	free (s);
	return NULL;
}

static void *sock_cmd_listen_loop (void *p)
{
	sock_t tmp;
	sock_t *s = (sock_t *) p;
	dbg ("sock api listen loop started\n");
	s->run = 1;

	while (s->run) {
		tmp.len = sizeof (struct sockaddr_un);
		tmp.fd = accept (s->fd, (struct sockaddr*)&tmp.addr, &tmp.len);
		if (tmp.fd >= 0) {
			sock_t *as = (sock_t *) malloc (sizeof (sock_t));
			if (as == NULL) {
				err ("Cannot get memory for socket\n");
			}
			*as=tmp;
			as->run = 0;
			pthread_create (&as->thread, NULL, sock_cmd_loop, as);
		} else {
			break;
		}
		pthread_testcancel();
	}
	pthread_detach (s->thread);
	return NULL;
}

static sock_t s;
int api_sock_init (const char *cmd_sock_path)
{
	s.addr.sun_family = AF_UNIX;
	strcpy (s.addr.sun_path, cmd_sock_path);
	s.len = sizeof(struct sockaddr_un); //strlen (cmd_sock_path) + sizeof (s.addr.sun_family);

	if ((s.fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
		warn ("Cannot get socket %d\n", errno);
		return -1;
	}
	unlink (cmd_sock_path);
	if (bind (s.fd, (struct sockaddr*)&s.addr, s.len) < 0) {
		warn ("Cannot bind control socket\n");
		return -1;
	}
	if (chmod(cmd_sock_path, S_IRWXU|S_IRWXG|S_IRWXO)) {
		warn ("Cannot chmod 777 socket %s\n", cmd_sock_path);
	}
	if (listen (s.fd, 5) < 0) {
		warn ("Cannot listen on socket\n");
		return -1;
	}
	return pthread_create (&s.thread, NULL, sock_cmd_listen_loop, &s);
}

void api_sock_exit (void)
{
	//FIXME memory leak on exit in context structres
	s.run=0;
	close(s.fd);
	
	if(pthread_exist(s.thread) && !pthread_cancel (s.thread)) {
		pthread_join (s.thread, NULL);
	}
}
#endif
#ifdef API_SHM
static api_cmd_t *api_cmd = NULL;
static pthread_t api_cmd_loop_thread;

static void *api_cmd_loop (void *p)
{
	netceiver_info_list_t *nc_list=nc_get_list();
	tra_info_t *tra_list=tra_get_list();
	while (1) {
		nc_lock_list();
		process_cmd (api_cmd, tra_list, nc_list);
		nc_unlock_list();
		usleep (1);
		pthread_testcancel();
	}

	return NULL;
}

int api_shm_init (void)
{
	int fd = shm_open (API_SHM_NAMESPACE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		warn ("Cannot get a shared memory handle\n");
		return -1;
	}
	if (ftruncate (fd, sizeof (api_cmd_t)) == -1) {
		err ("Cannot truncate shared memory\n");
	}
	api_cmd = mmap (NULL, sizeof (api_cmd_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (api_cmd == MAP_FAILED) {
		err ("MMap of shared memory region failed\n");
	}
	close (fd);

	memset (api_cmd, 0, sizeof (api_cmd_t));

	pthread_create (&api_cmd_loop_thread, NULL, api_cmd_loop, NULL);
	return 0;
}

void api_shm_exit (void)
{
	if(pthread_exist(api_cmd_loop_thread) && !pthread_cancel (api_cmd_loop_thread)) {
		pthread_join (api_cmd_loop_thread, NULL);
	}
	shm_unlink (API_SHM_NAMESPACE);
}
#endif
#ifdef API_WIN
 
void *api_cmd_loop(void *lpvParam) 
{ 
	netceiver_info_list_t *nc_list=nc_get_list();
	tra_info_t *tra_list=tra_get_list();
	api_cmd_t sock_cmd;
	DWORD cbBytesRead, cbWritten; 
	BOOL fSuccess; 
	HANDLE hPipe; 
  
	hPipe = (HANDLE) lpvParam; 
 
	while (1) { 
		fSuccess = ReadFile( 
			hPipe,        // handle to pipe 
			&sock_cmd,    // buffer to receive data 
			sizeof(sock_cmd), // size of buffer 
			&cbBytesRead, // number of bytes read 
			NULL);        // not overlapped I/O 

		if (! fSuccess || cbBytesRead == 0) {
			break;
		}

		if (cbBytesRead == sizeof (api_cmd_t)) {
			nc_lock_list();
			process_cmd (&sock_cmd, tra_list, nc_list);
			nc_unlock_list();
			
			fSuccess = WriteFile( 
				hPipe,        // handle to pipe 
				&sock_cmd,      // buffer to write from 
				sizeof(sock_cmd), // number of bytes to write 
				&cbWritten,   // number of bytes written 
				NULL);        // not overlapped I/O 

			if (! fSuccess || sizeof(sock_cmd) != cbWritten) {
				break; 
			}
		} else {
  	            sock_cmd.magic = MCLI_MAGIC;
                    sock_cmd.version = MCLI_VERSION;
                    sock_cmd.state = API_RESPONSE;
			fSuccess = WriteFile( 
				hPipe,        // handle to pipe 
				&sock_cmd,      // buffer to write from 
				sizeof(sock_cmd), // number of bytes to write 
				&cbWritten,   // number of bytes written 
				NULL);        // not overlapped I/O 

			if (! fSuccess || sizeof(sock_cmd) != cbWritten) {
				break; 
			}
			break;
		}
	} 
  
   FlushFileBuffers(hPipe); 
   DisconnectNamedPipe(hPipe); 
   CloseHandle(hPipe); 

   return NULL;
}

#define BUFSIZE 2048
void *api_listen_loop(void *p)
{	
	BOOL fConnected; 
	pthread_t api_cmd_loop_thread; 
	HANDLE hPipe;
	LPTSTR lpszPipename=(LPTSTR)p;
  
	while(1) {
		hPipe = CreateNamedPipe( 
					lpszPipename,             // pipe name 
					PIPE_ACCESS_DUPLEX,       // read/write access 
					PIPE_TYPE_MESSAGE |       // message type pipe 
					PIPE_READMODE_MESSAGE |   // message-read mode 
					PIPE_WAIT,                // blocking mode 
					PIPE_UNLIMITED_INSTANCES, // max. instances  
					BUFSIZE,                  // output buffer size 
					BUFSIZE,                  // input buffer size 
					0,                        // client time-out 
					NULL);                    // default security attribute 

		if (hPipe == INVALID_HANDLE_VALUE) {
			err ("CreatePipe failed"); 
			return NULL;
		}
		pthread_testcancel();
		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
		if (fConnected) { 
			if(pthread_create(&api_cmd_loop_thread, NULL, api_cmd_loop, hPipe)) {
				err ("CreateThread failed"); 
				return NULL;
			} else { 
				pthread_detach(api_cmd_loop_thread);
			}
		} 
		else {
			CloseHandle(hPipe); 
		}
	} 
	return NULL; 
} 

pthread_t api_listen_loop_thread;

int api_init (LPTSTR cmd_pipe_path)
{
	return pthread_create (&api_listen_loop_thread, NULL, api_listen_loop, cmd_pipe_path);
}

void api_exit (void)
{
	if(pthread_exist(api_listen_loop_thread) && !pthread_cancel (api_listen_loop_thread)) {
		TerminateThread(pthread_getw32threadhandle_np(api_listen_loop_thread),0);
		pthread_join (api_listen_loop_thread, NULL);
	}
}
 
#endif
