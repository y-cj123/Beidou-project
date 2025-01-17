/******* 客户端程序  client.c ************/

int main(int argc, char *argv[])
{
        int sockfd,nread;
        char buffer[1024];
        struct sockaddr_in server_addr;
        struct hostent *host;
        int portnumber,nbytes;

        if(argc!=3)
        {
                fprintf(stderr,"Usage:%s hostname portnumber\a\n",argv[0]);
                exit(1);
        }

        if((host=gethostbyname(argv[1]))==NULL)		//返回对应于给定主机名的主机信息
        {
                fprintf(stderr,"Gethostname error\n");
                exit(1);
        }

        if((portnumber=atoi(argv[2]))<0)
        {
                fprintf(stderr,"Usage:%s hostname portnumber\a\n",argv[0]);
                exit(1);
        }

        /* 客户程序开始建立 sockfd描述符  */
        if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
        {
                fprintf(stderr,"Socket Error:%s\a\n",strerror(errno));
                exit(1);
        }

        /* 客户程序填充服务端的资料       */
        bzero(&server_addr,sizeof(server_addr));
        server_addr.sin_family=AF_INET;
        server_addr.sin_port=htons(portnumber);
        server_addr.sin_addr=*((struct in_addr *)host->h_addr);

        /* 客户程序发起连接请求         */ 
        if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
        {
                fprintf(stderr,"Connect Error:%s\a\n",strerror(errno));
                exit(1);
        }

        /* 连接成功了           */
/*        if((nbytes=read(sockfd,buffer,1024))==-1)
        {
                fprintf(stderr,"Read Error:%s\n",strerror(errno));
                exit(1);
        }
        buffer[nbytes]='\0';
        printf("I have received:%s\n",buffer);
*/


	/* 接收由server端传来的信息*/
	recv(sockfd,buffer,sizeof(buffer),0);
	buffer[sizeof(buffer)]='\n';
	printf("%s\n",buffer);
	while(1)
	{

		bzero(buffer,sizeof(buffer));
		/* 从标准输入设备取得字符串*/
		nread=read(STDIN_FILENO,buffer,sizeof(buffer));
		/* 将字符串传给server端*/
		if(send(sockfd,buffer,nread,0)<0)
		{
//			perror(“send”);
			exit(1);
		}



        /* 结束通讯     */
//        close(sockfd);
//	break;
//        exit(0);
	}
}
