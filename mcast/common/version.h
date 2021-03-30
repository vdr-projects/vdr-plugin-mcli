/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifdef P2P
  #define MCLI_P2PSTR "-P2P"
#else
  #define MCLI_P2PSTR ""
#endif
#define MCLI_APP_VERSION "0.99.35" MCLI_P2PSTR
#define MCLI_COMPILED __DATE__ " " __TIME__
#define MCLI_VERSION_STR MCLI_APP_VERSION " (" MCLI_COMPILED ")"
#define MCLI_MAGIC 0xDEADBEEF
#define MCLI_VERSION 0x14
