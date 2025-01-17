/*******************************
北斗长报文可靠通信接收端程序
*******************************/
#include "BD_recieve.h"
#include "BD_split.h"
#include "BD_combine.h"

#define max(a,b) a>b?a:b
#define PSTR_SIZE 10
#define RLINE 1024
#define FILE_NAME_SIZE 30 
#define  SP_LEN 60	//子包pure data 长度

extern unsigned char my_addr[3];//服务器北斗地址
extern int upload_signal;//上传信号，置1可以上传
extern char upload_filename[30];//保存数据文件
extern time_t BD_last_sendtimer;
extern int acked;
time_t curr_timer;

int get_pure_data(char *rf_buff,char *pstr[])
{	
	int i = 0;
	char *colon = NULL;
	char *end_buf = NULL;
	char str[256];	
	char *number_of_package = strstr(rf_buff,"sub");
	int sequence = 0;
	if(number_of_package!=NULL)
	{
		number_of_package += strlen("sub");
		sequence = (int)(*number_of_package-0x30);
		//printf("number pack:%c %d\n",*number_of_package,sequence);
		if(pstr!= NULL)
		{
			colon = strchr(rf_buff,':');
			if(colon != NULL)
			{
				colon++;
				end_buf = rf_buff + strlen(rf_buff);
				//printf("\nstrlen rf_buff= %d\n",strlen(rf_buff));
				strncpy(str, colon, end_buf-colon);
		               // printf("1 strlen str is %d\n",strlen(str));
				for(i = 0; i < strlen(str); i++)//remove '\n' at the end of each line
				{
					if(*(str+i) == ' ' && *(str+i+1) == '\n')
					{
						*(str+i+1)='\0';
					}
				}
				for(i = 0; i <= strlen(str); i++)
				{
					pstr[sequence][i] = str[i];

				}              
			}
			else
			{
				printf("bad format of file_1\n");
				return -1;
			}				
		}
	}
	else
	{
		sequence=-1;
	}
	//show(str,pstr[sequence]);
        
	return sequence;
}


void BD_receive(int port_f)
{

	time_t time_present; //get the time in the form of seconds
	char length_of_time;
	char file_name[FILE_NAME_SIZE];
	FILE *fp_process=NULL;//document describer
	FILE *fp=NULL;
	int i=0,j=0,l=0,m=0;	//计数
	int n_lost_packs=0;
	int p;		//压缩方式
	int lack[256]={0};
	extern unsigned char my_addr[3];
	unsigned char from_addr[3];

	fd_set rdfds; //file describe
	struct timeval tv;	
	unsigned char receive_data[512];
	unsigned char hour,min;
	int receive_length=0,flag=0;


	char rf_buff[RLINE];
	int seq_number=0;
	int number_of_packages;
	int ack_num=0;
/**********等待接收数据*********/
	struct DT
	{
		unsigned char data[SP_LEN+5];
		int stat;
	}msg; //receive buffer	
	memset(&msg, 0, sizeof(struct DT));


while(1)
{
//	printf("等待数据......\n");
	
	FD_ZERO(&rdfds);
	FD_SET(port_f,&rdfds);

//	tv.tv_sec=5; //wait time period
//	tv.tv_usec=0;

	flag=0;
	flag=select(port_f+1,&rdfds,NULL,NULL,NULL);

	if(flag<0)
	{
		printf("select 错误！\n");
		break;
	}
	else if(flag==0) printf("等待超时！重载...\n\n");
	else
	{
		if(FD_ISSET(port_f,&rdfds))
		{
	
		/*****************拆北斗报文头********************/
		printf("北斗返回数据打印length = -1\n");
		receive_length = BD_read(port_f,from_addr,receive_data);
		printf("lenth = %d\n", receive_length);
		if(receive_length>0)
		{
				memset(&msg, 0, sizeof(struct DT));
				number_of_packages=receive_data[4];

				/***************************识别地址，标识***************************/
				printf("\n收到来自%x %x %x的北斗报文：\n",from_addr[0],from_addr[1],from_addr[2]);
				/***************get the length of each part in subpackages*********/
				printf("receive_data的报文\n"); 
				for(i=0;i<receive_length;i++)	//存储 to buffer msg
				{
					msg.data[i]=receive_data[i];
					msg.stat = 1;
					printf("%x ",receive_data[i]);        
				}
				printf("\n\n");
				if(msg.data[5]==0x4F&&msg.data[6]==0x4B)
				{
					ack_num=0;
					printf("通信成功\n");
				}
				/**************up_load_enquiry*****************/
				/*
				if(msg.data[5]==0x68)
				{
					printf("\nenquiry.\n");
					//snprintf(file_name,25,"./%x%x%x",from_addr[0],from_addr[1],from_addr[2]);
					file_name[0] = 'd';
					file_name[1] = 'a';
					file_name[2] = 't';
					file_name[3] = 'a';
					file_name[4] = '\0';
					if(access(file_name,F_OK)==0)
					{
						fp=fopen(file_name,"a");
						printf("open fp in the mode of a\n");
					}
					else
					{
						fp=fopen(file_name,"w+");
						printf("open fp in the mode of w+\n");
					}
					if(fp==NULL) 
					{
						printf("fp open fail\n");
					}	

					printf("写入文件 %s\n", file_name);
					//fprintf(fp,"%s:",time_buff);//time_buff :receive time	
					//for(i = 1; i < receive_length; i++)
					//{
					// fputs(receive_data,fp);
					//}
					for(i=1;i<receive_length;i++)
					{
						fprintf(fp,"%x ",receive_data[i]);
					}
					for (i=1; i<receive_length; i++)
						printf("%x ", receive_data[i]);
					printf("\n\n");
					//fclose(fp);
					//while (1);
					int cnt = 0;
					for(cnt = 0; cnt != strlen(file_name); ++ cnt)
						upload_filename[cnt] = file_name[cnt];
					fclose(fp);
					upload_signal = 1;
				}
				
				/*******************************************/	
		
				
				
				/****************判别报文类型************************/
				if (msg.data[3] != 0)				//普通报文
				{
					printf("普通报文\n");
					snprintf(file_name, 20,"./%x%x%x_%x%x_temp",from_addr[0],from_addr[1],from_addr[2],receive_data[1],receive_data[2]);
					if(access(file_name,F_OK)==0)
					{	
						//查重
						fp_process=fopen(file_name,"r");
						while(!feof(fp_process))
						{
							fgets(rf_buff,RLINE,fp_process);
						    seq_number=get_pure_data(rf_buff,NULL);
							if(seq_number==msg.data[3]) 
							{	
								printf("是重复报文，删除！\n");
								msg.stat=0;
								memset(&msg, 0, sizeof(struct DT));
								break;
							}	
						}
						//将数据写入已有文件中
						if(msg.stat==1)
						{	
							fclose(fp_process);
							fp_process=fopen(file_name,"a");
							fseek(fp_process,0,SEEK_END);//将文件指针指向文件结尾
							time(&time_present);
							length_of_time=(int)strlen(ctime(&time_present));
							fprintf(fp_process,"\n");
							fprintf(fp_process,"%sFrom:%x %x %x\n",ctime(&time_present),from_addr[0],from_addr[1],from_addr[2]);
							fprintf(fp_process,"sub%x in %x:",receive_data[3],receive_data[4]);
							for(i=5;i<receive_length;i++)
							{
								fprintf(fp_process,"%x ",msg.data[i]);
							}
							fprintf(fp_process,"\n");
							msg.stat=0; //msg is saved
						}
					}
					//新建一个文件，写入数据
					else
					{	
						fp_process=fopen(file_name,"w+");
	/********get the current time and add it into the save files***************/
						time(&time_present);
						length_of_time=(int)strlen(ctime(&time_present));

						fprintf(fp_process,"\n");
						fprintf(fp_process,"%sFrom:%x %x %x\n",ctime(&time_present),from_addr[0],from_addr[1],from_addr[2]);
						fprintf(fp_process,"sub%x in %x:",receive_data[3],receive_data[4]);
						for(i=5;i<receive_length;i++)
						{
							fprintf(fp_process,"%x ",msg.data[i]);
					    }						
						msg.stat=0; 
					}
/**clsoe temp file after save one submessage*/	
					fclose(fp_process);
				}
								
				
				if(msg.data[3]==0 && msg.data[5] != 0x68)		//确认报文
				{	
					printf("收到确认报文，开始检查缺少报文......\n");
					/****open the temp file********/
					//格式化拷贝字符串到file_name中
					snprintf(file_name, 20,"./%x%x%x_%x%x_temp",from_addr[0],from_addr[1],from_addr[2],receive_data[1],receive_data[2]);
					if(access(file_name,F_OK)==0)//判断文件是否存在
					{					
						fp_process=fopen(file_name,"r");
						for(i = 1; i <= number_of_packages; i++)
						{
							lack[i]=1;
						}
						n_lost_packs=0;
						while(!feof(fp_process))
						{
							fgets(rf_buff,RLINE,fp_process);
							//printf("rf_buff = %s\n", rf_buff);
							seq_number = get_pure_data(rf_buff,NULL);
							//printf("seq_number = %d\n", seq_number);
							if(seq_number>0) 
							{
								lack[seq_number]=0;
								printf("找到子包 %d\n",seq_number);
							}
						}
						for(i = 1; i <= number_of_packages; i++)
						{	
							if(lack[i] == 1)
							{
								printf("缺少子包 %d\n",i);
								n_lost_packs++;
							}
						}
						printf("报文检查完毕！\n\n");
/***temp file fp_process is open***/				
						unsigned char ACK[3+n_lost_packs], ACK_send[3+n_lost_packs+19];
						memset(ACK,0,3+n_lost_packs);
						memset(ACK_send,0,3+n_lost_packs+19);
						ACK[0]=msg.data[1];
						ACK[1]=msg.data[2];
						ACK[2]=n_lost_packs;
						i=0;
						for(j = 1; j <= number_of_packages; j++)
						{	
							if(lack[j]==1)
							{
								ACK[3+i]=j;
								i++;
							}
						}
						printf("待发送的ACK为：\n");
						for(i = 0; i < 3 + n_lost_packs; i++)
						{
							printf("%x ",ACK[i]);
						}
						printf("\n");
						printf("\n开始封装北斗报文头......\n");
						printf("本机地址为:\n");
						for(i = 0; i < 3; i++)
						{
							printf("%x ",my_addr[i]);
						}
						printf("\n");
						printf("目的地址为:\n");
						for(i=0;i<3;i++)
						{
							printf("%x ",from_addr[i]);
						}
						printf("\n");

						printf("封装ACK:\n");
						BD_send(ACK,3+n_lost_packs,my_addr,from_addr,ACK_send);
						for(i=0;i<3+n_lost_packs+19;i++)
						{
							printf("%x ",ACK_send[i]);
						}
						printf("\n\n");

						printf("发送ACK...\n\n");
						while(1){
							sleep(2);
							time(&curr_timer);
							printf("BD_last_sendtimer is %d\n",BD_last_sendtimer);
							printf("curr_timer is %d\n",curr_timer);
							if(curr_timer-BD_last_sendtimer>60){
								if(write(port_f,ACK_send,3+n_lost_packs+19)!=-1)
								{
									receive_length=BD_read(port_f,from_addr,receive_data);
									if(receive_length==-1&&acked==1)
									{
										printf("already send data to Beidou\n");
										BD_last_sendtimer=curr_timer;
										acked=0;
										break;
									}
								}
								else printf("串口写入失败\n");
							}
							else{
								time_t wait_time=60-(curr_timer-BD_last_sendtimer);
								printf("北斗设备未准备就绪，%ds后重发\n", wait_time);	
								sleep(wait_time+1);
							} 
						}
						
						if(n_lost_packs==0)
						{

							char time_buff[30];
							char day_year[16];
							char str_pd[SP_LEN*3+1];
							char *pstr[PSTR_SIZE];
							for(i = 0; i < PSTR_SIZE; i++)
							{
 								pstr[i] = (char*)malloc(RLINE);
							}
							rewind(fp_process);
							//fgets(time_buff,length_of_time,fp_process); 
							fgets(time_buff,length_of_time,fp_process); // read receive time
							//printf("time_buf:%s\n",time_buff);
							for(i=0;i<15;i++)
							{
								if(i<=10)
								{
									day_year[i]=time_buff[i];
								}
								else
								{
									day_year[i]=time_buff[i+9];
								}
							}
							day_year[15]='\0';

							while(!feof(fp_process)) 
							{	int j=0;
								fgets(rf_buff,RLINE,fp_process);
       								//printf("\n rf_buff %s\n",rf_buff);
								get_pure_data(rf_buff,pstr);
								
						                //if(i!=-1){
									//printf("\n%d fp_process pstr: ", i); 								   
									//for(j=0;j<strlen(pstr[i]);j++){
							                 //  printf("%c ",pstr[i][j]);}
									 // printf("\n");
									//}	
							}
							fclose(fp_process);
							remove(file_name);
//func -open file
							//printf("day_year :%s\n", day_year);
							snprintf(file_name,25,"./%x%x%x %s",from_addr[0],from_addr[1],from_addr[2],day_year);
							if(access(file_name,F_OK)==0)
							{
								fp=fopen(file_name,"a");
								//printf("open fp in the mode of a\n");
							}
							else
							{
								fp=fopen(file_name,"w+");
								//printf("open fp in the mode of w+\n");
							}
							if(fp==NULL) 
							{
								printf("fp open fail\n");
							}	

							printf("写入文件-- %s\n", file_name);
							printf("number_of_packages: %d\n", number_of_packages);
							//fprintf(fp,"%s:",time_buff);//time_buff :receive time	
							for(i = 1; i <= number_of_packages; i++)
							{
							 	fputs(pstr[i],fp);
								printf("%s\n", pstr[i]);
							}
							printf("\n");
							//fprintf(fp,"\n");	//不要写入'\n'，会与读文件的方式发送冲突，出现最后一行读两次的bug

							/*
							printf("数据为：");
							rewind(fp);
							fgets(str_pd,RLINE,fp);
							printf("%s\n",str_pd);
							*/

							int cnt = 0;
							for(cnt = 0; cnt != strlen(file_name); ++ cnt)
								upload_filename[cnt] = file_name[cnt];
							fclose(fp);

							//while(1);
							upload_signal = 1;	//上传数据同步信号

							/*
							fp=fopen(file_name,"r");	
							while(!feof(fp)) 
							{
								fgets(str_pd,RLINE,fp);
								printf("str_pure_data:%s",str_pd);
							}
							printf("\n");
							fclose(fp);
							*/
						}//??end_of_if(t==0)??
						//else
							//fclose(fp_process);
					}//end_of_if(access(file_name,F_OK)==0)
					else
					{
						printf("false enquiry package\n");
					}
				}
			}
	}
}

	if(flag<0) printf("接收失败！\n");
}
}
