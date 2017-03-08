#ifndef J1STIO_H_INCLUDED
#define J1STIO_H_INCLUDED

#define MAX_SENDBUF	3000
#define MAX_RCVBUF	1000
#define KEEPALIVEINTERVAL 20
#define JTIMEOUT	3000

typedef struct jNet jNet;
typedef struct Network Network;

struct Network
{
    int my_socket;
	struct _SSL_CTX *sslctx;	// SSL flag
	struct _SSL *ssl;
	const char *finger;
	int remainbytes;	// TODO: Uninitialized
	unsigned char *localbuf;
    int (*mqttread) (Network*, unsigned char*, int, int);
    int (*mqttwrite) (Network*, unsigned char*, int, int);
    void (*disconnect) (Network*);
};

struct jNet
{
    struct Network *pNet;
    struct Client *pClient;
    unsigned char *pSendBuf;
    unsigned char *pRcvBuf;
};

int linux_read(Network*, unsigned char*, int, int);
int linux_write(Network*, unsigned char*, int, int);
//void linux_disconnect(jNet*);

extern jNet *jNetInit(void);
extern jNet *jNetSInit(const unsigned char *finger);

extern void jNetFree(jNet *);

extern int jNetConnect(jNet *pJnet, const char *host, short nPort,
                       const char *agentId, const char *token);
extern int jNetDisconnect(jNet *pJnet);

extern int jNetYield(jNet *pJnet);

extern int jNetPublishT(jNet *pJnet, const char *topic, char *payload);

extern int jVerifyCert(struct _SSL *ssl, const char *host, const char *finger);


typedef struct jTimer jTimer;

struct jTimer
{
    portTickType end_time;
    int	t_id;
};

void jTmrInit(jTimer*, int id);
char jTmrExpired(jTimer*);
void jTmrStart(jTimer*, unsigned int);
void jTmrStart_ms(jTimer*, unsigned int);
int jTmrLeft(jTimer*);

typedef struct Timer Timer;

struct Timer
{
    portTickType end_time;
    int	t_id;
};

char expired(Timer*);
void countdown_ms(Timer*, unsigned int);
void countdown(Timer*, unsigned int);
int left_ms(Timer*);

void InitTimer(Timer*, int id);

#endif // J1STIO_H_INCLUDED
