#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
/* OpenSSL */
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#define BUFFSIZE 1024


void errck()
{
	if (errno != 0)
	{
		printf("Error %d -> %s\n", errno, strerror(errno));
		errno = 0;
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	int sockfd, result;
	char buff[BUFFSIZE];
	char *url = argv[1];
	char ip[32];
	long port;
	struct addrinfo hints, *res;
	struct in_addr *addr;
	SSL_CTX *ctx;
	SSL *ssl;

	/* Hists */
	memset(&hints, 0, sizeof(hints)); 
	hints.ai_family   = AF_INET;  

	/* Create a TLS client context with a CA certificate */
	ctx = SSL_CTX_new(TLS_client_method());

	/* Get host data */
	char *protocol = strtok(url, ":");
	url = strtok(NULL, "/");

	getaddrinfo(url, protocol, &hints, &res);
	errck();
	getnameinfo(res->ai_addr, res->ai_addrlen, ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
	port = ntohs(((struct sockaddr_in *)res->ai_addr)->sin_port);

	/* Create a socket and SSL session */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	errck();
	puts("Socket created!");

	/* Try to connect */
	result = connect(sockfd, res->ai_addr, res->ai_addrlen);
	errck();
	puts("Connected!");

	ssl = SSL_new(ctx);
	SSL_set_fd(ssl, sockfd);
	puts("Socket linked with SSL!");

	/* Run the OpenSSL handshake */
	result = SSL_connect(ssl);
	errck();

	/* Exchange some data if the connection succeeded */
	char *uri = strtok(url, "/");
	uri = strtok(NULL, "/");

	sprintf(buff, "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", url);
	printf("\nCLIENT > %s\n", buff);
	SSL_write(ssl, buff, strlen(buff) + 1);
	memset(buff, 0, BUFFSIZE);

	printf("SERVER >\n");
	while(SSL_read(ssl, buff, sizeof(buff)) > 0)
	{
		errck();
		printf("%s", buff);
		memset(buff, 0, BUFFSIZE);
	}

	/* Close connection */
	close(sockfd);
	SSL_free(ssl);
	SSL_CTX_free(ctx);

	return EXIT_SUCCESS;
}
