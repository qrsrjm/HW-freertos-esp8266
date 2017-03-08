#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "ssl/ssl_compat-1.0.h"

#include "j1stio.h"
#include "MQTTClient.h"



char jTmrExpired(jTimer* timer)
{
    long left = timer->end_time - xTaskGetTickCount();
    return left < 0;
}


void jTmrStart(jTimer* timer, unsigned int timeout)
{
    timer->end_time = xTaskGetTickCount() + timeout * 1000 / portTICK_RATE_MS; 
}


void jTmrStart_ms(jTimer* timer, unsigned int timeout)
{
    timer->end_time = xTaskGetTickCount() + timeout / portTICK_RATE_MS; 
}

int jTmrLeft(jTimer* timer)
{
    long left = timer->end_time - xTaskGetTickCount();
    return (left < 0) ? 0 : left * portTICK_RATE_MS;    
}


void jTmr(jTimer* timer, int id)
{
    timer->end_time = 0;
    timer->t_id = id;
}

char expired(Timer* timer)
{
    long left = timer->end_time - xTaskGetTickCount();
    return left < 0;
}

void countdown_ms(Timer* timer, unsigned int timeout)
{
    timer->end_time = xTaskGetTickCount() + timeout / portTICK_RATE_MS; 
}

void countdown(Timer* timer, unsigned int timeout)
{
    timer->end_time = xTaskGetTickCount() + timeout * 1000 / portTICK_RATE_MS; 
}

int left_ms(Timer* timer)
{
    long left = timer->end_time - xTaskGetTickCount();
    return (left < 0) ? 0 : left * portTICK_RATE_MS;    
}


void InitTimer(Timer* timer, int id)
{
    timer->end_time = 0;
    timer->t_id = id;
}

extern int ssl_readB(SSL *ssl, uint8_t **in_data);

// Read len bytes to buffer
// Returns
//   positive: bytes read
//         0 : timeout
//        -1 : error
//        -2 : timeout
//		  -3 : SSL
//		 -10 : unexpected
int secure_pread(Network* n, unsigned char* buffer, int len)
{
	int copied;
	
	do {
		copied = (len > n->remainbytes) ? n->remainbytes : len;
		if (copied > 0)
		{
			if (n->localbuf == NULL) return -10;
			memcpy(buffer, n->localbuf, copied);
			n->localbuf += copied;
			n->remainbytes -= copied;
			if (n->remainbytes == 0) n->localbuf = NULL;
			buffer += copied;
			len -= copied;
			return copied;
		}

		int ret = ssl_readB(n->ssl, &(n->localbuf));
		if (ret <= 0)
		{
			n->localbuf = NULL;
			return ret;
		}
		else n->remainbytes = ret;
	} while (1);
}

// Read len bytes from SSL connection
// returns
//   -2 : timeout
//   -1 : error
int secure_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
    {
        interval.tv_sec = 0;
        interval.tv_usec = 1000;
    }
    
    //printf("Read, timeout %d ms\n", timeout_ms);
    setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

	int bytes = 0;
	while (bytes < len)
	{
		int rc = secure_pread(n, buffer + bytes, len - bytes);
		if (rc == 0) return -2;
		else if (rc < 0) return -1;
		else bytes += rc;
	}
	return bytes;
}

int linux_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
    {
        interval.tv_sec = 0;
        interval.tv_usec = 1000;
    }
    
    //printf("Read, timeout %d ms\n", timeout_ms);
    setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

    int bytes = 0;
    while (bytes < len)
    {
        int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
        if (rc == -1)
        {
			if (errno == EAGAIN) return -2;
			else if (errno == EINTR) continue;
			else return -1;
//			if (errno != ENOTCONN && errno != ECONNRESET)
//			{
//			}
        }
        else if (rc == 0) return -1;
//        else if (rc==0) break;
        else bytes += rc;
    }
    return bytes;
}


int checkWaitingPacket(Network* n, Timer* timer)
{
    int left = left_ms(timer);
    if (left <= 0) return 0;

	if (n->ssl != NULL && n->remainbytes > 0) return 1;
    
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(n->my_socket, &readfds);

    struct timeval tv= {left / 1000, left % 1000 * 1000};
    if (select(n->my_socket+1, &readfds, NULL, NULL, &tv) > 0) return 1;
    else return 0;
}

int linux_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval tv;

    tv.tv_sec = 0;  /* 30 Secs Timeout */
    tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

    setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
    int	rc = write(n->my_socket, buffer, len);
    return rc;
}

int secure_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	struct timeval tv;

	tv.tv_sec = 0;  /* 30 Secs Timeout */
	tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
	int	rc = ssl_write(n->ssl, buffer, len);
	return rc;
}

jNet * jNetInit(void)
{
    jNet *tNet;

    struct Client *pClient;
    struct Network *pNetwork;
    unsigned char *pSendBuf;
    unsigned char *pRcvBuf;

    if ((pRcvBuf = (unsigned char *)malloc(MAX_RCVBUF)) == NULL)	goto err1;
    if ((pSendBuf = (unsigned char *)malloc(MAX_SENDBUF)) == NULL) goto err2;
    if ((pClient = (Client *)malloc(sizeof(struct Client))) == NULL) goto err3;
    if ((pNetwork = (Network *)malloc(sizeof(struct Network))) == NULL) goto err4;
    if ((tNet = (jNet *)malloc(sizeof(struct jNet))) == NULL) goto err5;

    pNetwork->my_socket = 0;
    pNetwork->mqttread = linux_read;
    pNetwork->mqttwrite = linux_write;
	pNetwork->remainbytes = 0;
	pNetwork->localbuf = NULL;
	pNetwork->sslctx = NULL;
	pNetwork->ssl = NULL;

	tNet->pNet = pNetwork;
    tNet->pClient = pClient;
    tNet->pSendBuf = pSendBuf;
    tNet->pRcvBuf = pRcvBuf;

    // TODO: ?
    return tNet;

err5:
    if (pNetwork)	free(pNetwork);
err4:
    if (pClient)	free(pClient);
err3:
    if (pSendBuf)	free(pSendBuf);
err2:
    if (pRcvBuf)	free(pRcvBuf);
err1:
    return NULL;
}

jNet * jNetSInit(const unsigned char *finger)
{
	SSL_CTX *ssl_ctx;
	uint32 options = SSL_SERVER_VERIFY_LATER | SSL_DISPLAY_CERTS | SSL_NO_DEFAULT_KEY;
	if ((ssl_ctx = ssl_ctx_new(options, SSL_DEFAULT_CLNT_SESS)) == NULL)
	{
		//printf("Error: SSL setup error\n");
		return NULL;
	}
	jNet *pJnet = jNetInit();
	if (pJnet == NULL)
	{
		ssl_ctx_free(ssl_ctx);
		return NULL;
	}
	pJnet->pNet->mqttread = secure_read;
	pJnet->pNet->mqttwrite = secure_write;
	pJnet->pNet->sslctx = ssl_ctx;
	pJnet->pNet->finger = finger;
	return pJnet;
}

void jNetFree(jNet *pNet)
{
	if (pNet->pNet->sslctx) ssl_ctx_free(pNet->pNet->sslctx);
    free(pNet->pNet);
    free(pNet->pClient);
    free(pNet->pRcvBuf);
    free(pNet->pSendBuf);
    free(pNet);
}

int CreateTCPConnect(char *srv, int port)
{
    int sockfd, error;
    struct sockaddr_in servaddr;
    socklen_t len;
	struct hostent *host;

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) return sockfd;

	host = gethostbyname(srv);
	if (NULL == host || host->h_addrtype != AF_INET)   // Cannot find resolve host name
	{
		close(sockfd);
		return -2;
	}

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = *((uint32_t **)host->h_addr_list)[0];
//    inet_pton(AF_INET, srv, &servaddr.sin_addr);

	// TODO: Use SetSockOpt to
    error = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    if (error == 0)
    {
        error = getsockname(sockfd, (struct sockaddr *)&servaddr, &len);
        //if (error  >= 0) printf("Server %s connected, local port %d\n", srv, ntohs(servaddr.sin_port));
        return sockfd;
    }
    else
    {
		//printf("Error connecting %d\n", error);
        close(sockfd);
		return error;
    }
}

int jVerifyCert(struct _SSL *ssl, const char *host, const char *finger)
{
	int ret;
//	if (strcmp(host, ssl->x509_ctx->cert_dn[0])) return -1;

	if (finger != NULL && memcmp(finger, ssl->x509_ctx->finger, 20))
		return -2;

	ret = ssl_verify_cert(ssl);
	if (ret == -518) return 0;	// Self signed cert
	return ret;	
}

void DisplayCert(X509_CTX *cert)
{
	//char temp[30];
	while (cert)
	{
		printf("DN:%s\n", cert->cert_dn[0]);
		//printf("X509 Sig(%d), Len %d, Digest", cert->sig_type, cert->sig_len);
		//int i;
#if 0
		printf("Digest: ");
		int *j = cert->digest->comps;
		for (i = 0; i<cert->digest->size - 1; i++)
			printf(":%x", *j++);
		printf("\n");
#endif

#if 0
		printf("Fingerprint: ");
		for (i = 0; i<20; i++)
			printf("%02x ", cert->finger[i]);
		printf("\n");
#endif
		cert = cert->next;
	}
}

int jNetConnect(jNet *pJnet, const char *host, short nPort,
                const char *agentId, const char *token)
{
    int sockfd, ret;
	SSL *ssl = NULL;

	MQTTPacket_connectData gMData = MQTTPacket_connectData_initializer;

    sockfd = CreateTCPConnect((char *)host, nPort);
    if (sockfd < 0) {
        printf("No IP interface\n");
        return -1;	// No IP address or stack not ready
    }
    pJnet->pNet->my_socket = sockfd;
//	printf("Server TCP connected\n");

	// SSL
	if (pJnet->pNet->sslctx) {
//		printf("Creating SSL\n");
		ssl = ssl_client_new(pJnet->pNet->sslctx, sockfd, NULL, 0);
		if (ssl == NULL) {
			printf("Cannot make SSL connection\n");
			close(sockfd);
			return -12;
		}
		if ((ret = ssl_handshake_status(ssl)) == SSL_OK){
			printf("SSL Connected to %s:%d ok\n", host, nPort);
			// printf("Cipher suite: %x\n", ssl_get_cipher_id(ssl));
			// TODO:
			ret = jVerifyCert(ssl, host, pJnet->pNet->finger);
			DisplayCert(ssl->x509_ctx);
			x509_free(ssl->x509_ctx);
			ssl->x509_ctx = NULL;
			if (ret < 0)
			{
				printf("Certificate error: %d.\n", ret);
				ret = -13;
				goto quit;
			}
		}
		else {
			printf("Error in SSL handshake.\n");
			ret = -14;
			goto quit;
		}
		pJnet->pNet->ssl = ssl;
	}

    MQTTClient(pJnet->pClient, pJnet->pNet, JTIMEOUT,
               pJnet->pSendBuf, MAX_SENDBUF, pJnet->pRcvBuf, MAX_RCVBUF);
	printf("Client done!\n");
    // TODO
    gMData.willFlag = 0;
    gMData.MQTTVersion = 3;
    gMData.clientID.cstring = (char *)agentId;
    gMData.username.cstring = (char *)agentId;
    gMData.password.cstring = (char *)token;
    gMData.keepAliveInterval = KEEPALIVEINTERVAL;
    gMData.cleansession = 1;
    ret = MQTTConnect(pJnet->pClient, &gMData);
    printf("MQTT Conn: %d\n", ret);
quit:
	if (ret != 0) {
		if (ssl != NULL) ssl_free(ssl);
		close(pJnet->pNet->my_socket);
	}
    return ret;
}

int jNetDisconnect(jNet *pJnet)
{
    MQTTDisconnect(pJnet->pClient);
	if (pJnet->pNet->sslctx && pJnet->pNet->ssl) 
		ssl_free(pJnet->pNet->ssl);
	return close(pJnet->pNet->my_socket);
}

int jNetYield(jNet *pJnet)
{
    return MQTTYield(pJnet->pClient, 1000);
}

int jNetSubscribeT(jNet *pJnet, const char *topic,
				   enum QoS q, messageHandler f)
{
	return MQTTSubscribe(pJnet->pClient, topic, q, f);
}

int jNetPublishT(jNet *pJnet, const char *topic, char *payload)
{
    MQTTMessage msg;
    msg.qos = QOS2;
    msg.retained = 0;
    msg.dup = 0;
    msg.payload = payload;
    msg.payloadlen = strlen(payload);
    return MQTTPublish(pJnet->pClient, topic, &msg);
}


