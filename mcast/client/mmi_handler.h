/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef _MMI_HANDLER_H
#define _MMI_HANDLER_H

#define MMI_TEXT_LENGTH 1024

typedef struct caid_mcg {
  
    int caid;
    struct in6_addr mcg;


} caid_mcg_t;

typedef struct mmi_info {
        
      int slot;
      caid_mcg_t  *caids;
      int caid_num;

      struct in6_addr ipv6;
      char uuid[UUID_SIZE];

      char mmi_text[MMI_TEXT_LENGTH];

} mmi_info_t;

DLL_SYMBOL void mmi_print_info(mmi_info_t *m);
DLL_SYMBOL int mmi_get_menu_text(int sockfd, char *buf, int buf_len, int timeout);
DLL_SYMBOL int mmi_send_menu_answer(int sockfd, char *buf, int buf_len);
DLL_SYMBOL UDPContext *mmi_broadcast_client_init(int port, char *iface);
DLL_SYMBOL void mmi_broadcast_client_exit(UDPContext *s);
DLL_SYMBOL int mmi_poll_for_menu_text(UDPContext *s, mmi_info_t *m, int timeout);
DLL_SYMBOL int mmi_open_menu_session(char *uuid, char *iface,int port, int cmd);
DLL_SYMBOL void mmi_close_menu_session(int s);
DLL_SYMBOL int mmi_cam_reset(char *uuid, char *intf, int port, int slot);
DLL_SYMBOL int mmi_cam_reinit(char *uuid, char *intf, int port, int slot);

#endif
