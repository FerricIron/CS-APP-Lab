#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *user_connection="Connection: close";
static const char *user_proxy_connection="Proxy-Connection: close";
static const char *user_host="Host: www.cmu.edu";
typedef struct {
	char user_agent_hdr[MAXLINE];
	char user_connection[MAXLINE];
	char user_proxy_connection[MAXLINE];
	char user_host[MAXLINE];
}Headers;
void todo(int fd);
void PrintErrorHeader(int fd,char status[],char* shortMsg);
void ServerError(int fd, char *cause, char status[], char *shortMsg, char *longMsg);
void ServerPrintResponseHeader(char *header);
int getRequestType(char *buf);
int main(int argc,char **argv)
{
    //printf("%s", user_agent_hdr);
    int listenfd,connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE],port[MAXLINE];
    if(argc!=2)
    {
    	fprintf(stderr,"usage :%s <port> \n",argv[0]);
    	exit(0);
    }
    listenfd=Open_listenfd(argv[1]);
    while(1)
    {
    	clientlen=sizeof(clientaddr);
    	connfd=Accept(listenfd,&clientaddr,clientlen);
    	Getnameinfo((SA *)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0);
    	fprintf(stdout, "Accept: (%s,%s)\n",hostname,port);
    	todo(connfd);
    	Close(connfd);
    }
    return 0;
}
void todo(int fd)
{
	rio_t rio;
	char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
	Rio_readinitb(&rio,fd);
	Rio_readlineb(&rio,buf,MAXLINE);
	fprintf(stdout, "Header:%s\n",buf);
	sscanf(buf,"%s %s %s",method,uri,version);
	if(strcasecmp(method,"GET"))
	{
		ServerError(fd,method,"501","Not implemented","ProxyLab not implement this method\n");
	}
}
int getRequestType(char *buf)
{
	const static int MAX_TYPE_SIZE=4;
	const static char type[MAX_TYPE_SIZE][MAXLINE]={"Host","User-Agent","Connection","Proxy-Connection"};
	for(int i=0;i<MAX_TYPE_SIZE;++i)
	{
		if(strcasecmp(buf,type[i])==0)
		{
			return i;
		}
	}
	return -1;
}
void GetRequestHeaders(rio_t &rio,char headers[MAXLINE])
{
	char buf[MAXLINE],type[MAXLINE],value[MAXLINE];
	headers[0]='\0';
	int rc,tc;
	int vis[4]={0};
	while(rc=Rio_readlineb(&rio,buf,MAXLINE)&&(rc!=0||rc!=-1))
	{
		sscanf(buf,"%s: %s",type,value);
		tc=getRequestType(type);
		switch(tc)
		{
			case -1:
			case 0:vis[0]=1;
			case 1:
			case 2:
			case 3:
		}
	}


}























/*
 * help function
 */

void PrintErrorHeader(int fd,char status[],char* shortMsg)
{
    char filetype[MAXLINE], buf[MAXBUF];
    /*Send the HTTP response Header to client*/
    sprintf(buf, "HTTP/1.0 %s %s\r\n",status,shortMsg);
    sprintf(buf, "%sServer: Yu Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n\r\n", buf);

    /*send header to client && log*/
    rio_writen(fd, buf, strlen(buf));
    ServerPrintResponseHeader(buf);
}
void ServerPrintResponseHeader(char *header) {
    printf("Response headers:\n");
    printf("%s", header);
}
void ServerError(int fd, char *cause, char status[], char *shortMsg, char *longMsg) {
    PrintErrorHeader(fd,status,shortMsg);
}
