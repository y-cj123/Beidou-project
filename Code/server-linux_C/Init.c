#include "Init.h"
#include "Socket_recieve.h"
#include "BD_recieve.h"
#include "BD_write.h"
#include "Serial_port.h"

unsigned char my_addr[3];

int upload_signal;
char upload_filename[30];
int download_signal;
int command_lenth;
unsigned char download_command[1024];

time_t BD_last_sendtimer;
//int bd_send_notReady[4]={0xff,0x00,0xff,0x00};//当北斗没有准备好发送数据时，返回给服务器的告知消息，并让他等待ns后重发

//将字符串转换为16进制
unsigned char char2xchar(unsigned char *c,int n)
{
	unsigned char temp = 0;
  if(n==1){
	if(c[0] >= '0' && c[0] <= '9')
		temp += (c[0]-'0');
	else if(*c >= 'a' && *c <= 'z')
		temp +=(c[0]-'a'+10);
	else
		temp += (*c-'A'+10);
  }
  if(n==2){
        if(c[0] >= '0' && c[0] <= '9')
		temp += (c[0]-'0')*16;
	else if(*c >= 'a' && *c <= 'z')
		temp +=(c[0]-'a'+10)*16;
	else
		temp += (*c-'A'+10)*16;


	if(*(c+1) >= '0' && *(c+1) <= '9')
		temp += (*(c+1)-'0');
	else if(*(c+1) >= 'a' && *(c+1) <= 'z')
		temp += (*(c+1)-'a'+10);
	else
		temp += (*(c+1)-'A'+10);
  }

	return temp;
}
	
int main(int argc, char *argv[])
{
	upload_signal = 0;
	download_signal = 0;
	
/******读取Config文件***********/
	FILE *fp;
	if((fp=fopen("Config.txt","r"))==NULL)
	{
		printf("can't open the Config.txt!\n");
		exit(1);
	}

	unsigned char hostname[30];//主机IP地址
	unsigned char port[30];//主机监听端口
	unsigned char c_my_addr[30];//服务器北斗地址
	unsigned char c_dev[30];//服务器串口名
	unsigned char c_boud[30];//服务器串口波特率
	unsigned char c_parity[30];//服务器串口停止位、数据位、校验位（n-无、O-偶、e-奇）

	bzero(hostname,sizeof(hostname));
	bzero(port, sizeof(port));
	bzero(c_my_addr, sizeof(c_my_addr));
	bzero(c_dev, sizeof(c_dev));
	bzero(c_boud, sizeof(c_boud));
	bzero(c_parity, sizeof(c_parity));

	fgets(hostname, 30, fp);
	fgets(port, 30, fp);
	fgets(c_my_addr, 30, fp);
	fgets(c_dev, 30, fp);
	fgets(c_boud, 30, fp);
	fgets(c_parity, 30, fp);
	close(fp);	

	hostname[strlen(hostname)-1] = '\0';
	port[strlen(port)-1] = '\0';
	c_my_addr[strlen(c_my_addr)-1] = '\0';
	c_dev[strlen(c_dev)-1] = '\0';
	c_boud[strlen(c_boud)-1] = '\0';
	c_parity[strlen(c_parity)-1] = '\0';


	my_addr[0] = char2xchar(c_my_addr,2);
	my_addr[1] = char2xchar(c_my_addr + 2,2);
	my_addr[2] = char2xchar(c_my_addr + 4,2);	
/*
	printf("my addr is :");
	int index;
	for(index = 0; index != 3; ++index)
		printf("%X ", my_addr[index]);
	printf("\n");
*/
/*******串口设置*************/
	int port_f;
	int boud = atoi(c_boud);	//9600
	char *dev = c_dev;	//"/dev/ttyUSB0"
	char *set_parity = c_parity;	//81N
	port_f = set_port(boud, dev, set_parity);
	printf("serial port is open！The boud is %d\n",boud);
	//time(&BD_last_sendtimer);
	BD_last_sendtimer=time(NULL);
	printf("北斗计时开始时间为%ld \n",BD_last_sendtimer);


/**********创建BD串口接收进程*************/	
	pthread_t BD_receive_tid;
	pthread_attr_t BD_receive_attr;
	pthread_attr_init(&BD_receive_attr);
	pthread_create(&BD_receive_tid, &BD_receive_attr, (void *)BD_receive, port_f);
	
	printf("created serial port recivev thread\n");

/**********创建BD串口发送进程*************/
	pthread_t BD_write_tid;
	pthread_attr_t BD_write_attr;
	pthread_attr_init(&BD_write_attr);
	pthread_create(&BD_write_tid, &BD_write_attr, (void *)BD_write, port_f);
	
	printf("created serial port send thread\n");	

/******socket设置********/
	int sockfd,nread;
        unsigned char buffer[1024];
        unsigned char hexBuffer[568];
        struct sockaddr_in server_addr;
        struct hostent *host;
        int portnumber,nbytes;
        memset(hexBuffer,0,sizeof(hexBuffer));

        if((host=gethostbyname(hostname))==NULL)		//返回对应于给定主机名的主机信息
        {
                fprintf(stderr,"Gethostname error\n");
                exit(1);
        }

        if((portnumber=atoi(port))<0)
        {
                fprintf(stderr,"Usage:%s hostname portnumber\a\n",argv[0]);
                exit(1);
        }
	//printf("portnumber:%d\n", portnumber);

        /* 客户程序开始建立 sockfd描述符  */
        if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
        {
                fprintf(stderr,"Socket Error:%s\a\n",strerror(errno));
                exit(1);
        }

	//对sock_cli设置KEEPALIVE和NODELAY
	int nZero=0;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));

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

	printf("connect the server.\n");

/**********创建socket接收进程*************/	
	pthread_t socket_recieve_tid;
	pthread_attr_t socket_recieve_attr;
	pthread_attr_init(&socket_recieve_attr);
	pthread_create(&socket_recieve_tid, &socket_recieve_attr, (void *)socket_recieve, sockfd);
	
	printf("created socket receive thread\n");

/***********本进程执行的任务****************/

/****
send login datagram
****/
unsigned char login[20]={0x68,0x32,0x00,0x32,0x00,0x68,0xD9,0x11,0x20,0x70,0x14,0x00,0x02,0x73,0x00,0x00,0x01,0x00,0x04,0x16};
send(sockfd, login,20, 0);
printf("login in\n");
	while(1)
	{	
		if(upload_signal == 1)
		{
			//printf("my file name is %s\n", upload_filename);
			//send(sockfd, upload_filename, strlen(upload_filename), 0);	
	
			bzero(buffer,sizeof(buffer));
			FILE *fp;
			fp = fopen(upload_filename, "r");
			

			while(!feof(fp))
			{
				fgets(buffer, sizeof(buffer), fp);
				//printf("\n\nsize: %d\n\n", sizeof(buffer));
                buffer[strlen(buffer)]='\0';
				//printf("\n\nsend_to_server_buffer: %s21\n\n",buffer);
                // printf("\nstrlen of buffer is %d\n",strlen(buffer));
 				unsigned char*p=buffer;				
				unsigned char temp[2];
                int count=0;
				unsigned char* end = buffer+strlen(buffer)-1;
				unsigned char* space;
				int i=0;
                while(*p!='\0')
                {
   					space = strchr(p,' ');
					if(space!=NULL)
					{
                        strncpy(temp,p,space-p);
     					hexBuffer[count]= char2xchar(temp,space-p);
                        count++;
     					p=space;
				 		p++;
					}
					else{
					strncpy(temp,p,end-p+1);
					hexBuffer[count]= char2xchar(temp,space-p);
                    count++;
					p=end+1;
						}
 				 }
                // printf("\ncount is %d\n",count);
				send(sockfd, hexBuffer, count, 0);
				//send(sockfd,test,strlen(test),0);
				printf("\nsend data to server:\n");				
				printf("%s21\n",buffer);
			}
			fclose(fp);
			remove(upload_filename);
			upload_signal = 0;
			//nread = 0;
			/* 从标准输入设备取得字符串*/
			//nread=read(STDIN_FILENO,buffer,sizeof(buffer));
			/* 将字符串传给server端*/
			/*if(nread > 0)
			{
				if(send(sockfd,buffer,nread,0)<0)
					exit(1);
				printf("send data to server:\n");
				buffer[sizeof(buffer)]='\0';
				printf("%s\n",buffer);
			}*/
		}
	}

}
