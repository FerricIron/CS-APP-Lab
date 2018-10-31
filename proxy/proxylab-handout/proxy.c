#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define 	MAX_TYPE_SIZE     4
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *user_connection="Connection: close\r\n";
static const char *user_proxy_connection="Proxy-Connection: close\r\n";
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
void initHeader(Headers* header);
void ParseUrl(char url[],char uri[],char hostname[],char port[],Headers* header);
void GetRequestHeaders(rio_t *rio,char requestHeaders[],Headers *header);
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
    	clientlen=sizeof(struct sockaddr_storage);
    	connfd=Accept(listenfd,(SA *)&clientaddr,&clientlen);
    	Getnameinfo((SA *)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0);
    	fprintf(stdout, "Accept: (%s,%s)\n",hostname,port);
    	todo(connfd);
    	Close(connfd);
    }
    return 0;
}
void todo(int fd)
{
	rio_t rio,rioc;
	int rc;
	char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE],url[MAXLINE];
	char hostname[MAXLINE],port[MAXLINE],temp[MAXLINE];
	Rio_readinitb(&rio,fd);
	Rio_readlineb(&rio,buf,MAXLINE);
  fprintf(stdout,"Proxy recive header:\n%s\n",buf);
	sscanf(buf,"%s %s %s",method,url,version);
	if(strcasecmp(method,"GET"))
	{
		ServerError(fd,method,"501","Not implemented","ProxyLab not implement this method\n");
	}
  Headers header;
  ParseUrl(url,uri,hostname,port,&header);
	sprintf(buf,"%s %s %s",method,uri,"HTTP/1.0\r\n");
	GetRequestHeaders(&rio,buf,&header);
	sscanf(header.user_host,"%s %s",temp,hostname);
	char *ptr=strchr(hostname,':');
	if(ptr!=NULL){
		strcpy(port,ptr+1);
		*ptr='\0';
	}else
	{
		strcpy(port,"80");
	}
  fprintf(stdout,"Open_clientfd:hostname=%s port=%s\n",hostname,port);
	int connfd=Open_clientfd(hostname,port);
	if(connfd<0)
	{
		ServerError(fd,method,"500","Can not Connection","Proxy Can't connection to end server\n");
	}
  fprintf(stdout,"Proxy will send header:\n%s\n",buf);
	Rio_writen(connfd,buf,MAXLINE);
	Rio_readinitb(&rioc,connfd);
	while((rc=Rio_readlineb(&rioc,buf,MAXLINE))>0)
	{
    fprintf(stdout,"Proxy received %d bytes,ten send to end_server\n",rc);
		Rio_writen(fd,buf,rc);
	}
  Close(connfd);
	return ;
}
int getRequestType(char *buf)
{
	const static char type[MAX_TYPE_SIZE][MAXLINE]={"Host:","User-Agent:","Connection:","Proxy-Connection:"};
	int i;
  for(i=0;i<MAX_TYPE_SIZE;++i)
	{
		if(strcasecmp(buf,type[i])==0)
		{
			return i;
		}
	}
	return -1;
}
void ParseUrl(char url[],char uri[],char hostname[],char port[],Headers* header)
{
  char *ptr=strchr(url,':');
  char *uri_ptr=strchr(ptr+3,'/');
  strcpy(uri,uri_ptr);
  *uri_ptr='\0';
  strcpy(header->user_host,ptr+2);
  char *port_ptr=strchr(ptr+3,':');
  if(port_ptr==NULL)
    {
      strcpy(port,"80");
    }else{
      strcpy(port,port_ptr+1);
      *port_ptr='\0';
   }
  strcpy(hostname,ptr+3);
}
void initHeader(Headers* header)
{
	strcpy(header->user_agent_hdr,user_agent_hdr);
	strcpy(header->user_connection,user_connection);
	strcpy(header->user_proxy_connection,user_proxy_connection);
}
void GetRequestHeaders(rio_t *rio,char requestHeaders[],Headers *header)
{
	char buf[MAXLINE],type[MAXLINE];
	int rc,tc;
	initHeader(header);
	while((rc=Rio_readlineb(rio,buf,MAXLINE))>0)
	{
		if(strcmp(buf,"\r\n")==0)
		{
			break;
		}
		sscanf(buf,"%s",type);
		tc=getRequestType(type);
		//fprintf(stdout, "%s",buf );
		switch(tc)
		{
			case -1:strcat(requestHeaders,buf);break;
			case 0:strcpy(header->user_host,buf);
			case 1:
			case 2:
			case 3:
				break;
		}
	}
	strcat(requestHeaders,header->user_agent_hdr);
	strcat(requestHeaders,header->user_connection);
	strcat(requestHeaders,header->user_proxy_connection);
	strcat(requestHeaders,header->user_host);
	strcat(requestHeaders,"\r\n");
}























/*
 * help function
 */

void PrintErrorHeader(int fd,char status[],char* shortMsg)
{
    char buf[MAXBUF];
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
