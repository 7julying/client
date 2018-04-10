#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <wiringPi.h>

#define BUFSIZE 5 //缓冲区大小
#define RIGHT 26 //传感器output接在GPIO.26

//通过定义四个电机的状态定义小车状态
#define MOTOR_GO_FORWARD   softPwmWrite(4, 0);softPwmWrite(1, 51);softPwmWrite(6, 0);softPwmWrite(5, 51);
//#define MOTOR_GO_BACK	   digitalWrite(4,HIGH);digitalWrite(1,LOW);digitalWrite(6,HIGH);digitalWrite(5,LOW)
#define MOTOR_GO_RIGHT	  softPwmWrite(1, 51);softPwmWrite(4, 0);softPwmWrite(5, 0);softPwmWrite(6, 51);
#define MOTOR_GO_LEFT	   softPwmWrite(4, 51);softPwmWrite(1, 0);softPwmWrite(5, 51);softPwmWrite(6, 0);
#define MOTOR_GO_STOP	   	softPwmWrite(1, 0); softPwmWrite(4, 0);softPwmWrite(5, 0);softPwmWrite(6, 0);

int main(int argc, char *argv[])
{
	struct timeval tv;//定义时间检测结构tv
	char *servIP = argv[1];//获得服务器IP地址
	in_port_t servPort = atoi(argv[2]);//获得服务器端口号

	//创建套接字
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	int z = -1;

	/*RPI*/
	wiringPiSetup();
	/*WiringPi GPIO*/
	pinMode(1, OUTPUT);	//IN1
	pinMode(4, OUTPUT);	//IN2
	pinMode(5, OUTPUT);	//IN3
	pinMode(6, OUTPUT);	//IN4
	pinMode(26, INPUT);

	/*Init output*/
	softPwmCreate(1, 1, 100);
	softPwmCreate(4, 1, 100);
	softPwmCreate(5, 1, 100);
	softPwmCreate(6, 1, 100);

	if (argc != 3)//若系统参数不是两个，则需重新设定
	{
		printf("Please add servIP or servPort!");
		exit(1);//直接退出程序
	}
	if (servPort<8000)//若系统参数不正确，则输出错误，退出程序
	{
		printf("Enter Error");
		exit(1);
	}
	if (sockfd < 0)
	{
		printf("socket error");
		exit(1);
	}

	//设置服务器地址
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));//将addr的内容清零，再重新设置结构server_addr的内容	
	servAddr.sin_family = AF_INET;//指定地址族为AF_INET，表示TCP/IP协议.
	servAddr.sin_addr.s_addr = inet_addr(servIP);//inet_addr函数将用点分十进制字符串表示的IPv4地址转化为用网络字节序整数表示的IPv4地址 
	servAddr.sin_port = htons(servPort);//指定端口号

	//连接服务器
	int connectfd=connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr));
	if (connectfd < 0)
	{
		printf("connect error");
		exit(1);
	}
	printf("connecting to the server...\n");
	char buf[BUFSIZE]; //定义并初始化读空间
	char backbuf[BUFSIZE]; //定义并初始化写空间
	memset(buf, 0, BUFSIZE);
	memset(backbuf, 0, BUFSIZE);
    int obstacleFlag=0;//定义状态接收传感器输出状态
	int obstacle;
	fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFL,0)|O_NONBLOCK);//将套接字设置为非阻塞模式
	while (1)
	{	
	//小车遇到障碍，GPIO.26输出低电平，头车将状态返回给服务器
	if (obstacleFlag = 0)
	{
		obstacle = digitalRead(RIGHT);
		if (obstacle == LOW)
		{
			obstacleFlag = 2;
			MOTOR_GO_STOP;
			printf("obstacle\n"); 
			backbuf[1] = 0x05;
			write(sockfd, backbuf, 5);
			//usleep(500000);
		}
	}		
		if ((z = read(sockfd, buf, sizeof buf)) > 0)//读取服务器传输的数据，返回数据长度
		{
			if (z == 5)
			{
				if (buf[0] == 0x00)
				{
					switch (buf[1])
					{
					case 0x01:MOTOR_GO_FORWARD; printf("forward\n");memcpy(backbuf,buf,sizeof buf);break;
					case 0x02:MOTOR_GO_STOP;    printf("stop\n"); memcpy(backbuf,buf,sizeof buf);break;
					case 0x03:MOTOR_GO_LEFT;    printf("left\n"); memcpy(backbuf,buf,sizeof buf); break;
					case 0x04:MOTOR_GO_RIGHT;   printf("right\n"); memcpy(backbuf,buf,sizeof buf); break;
					default: break;
					}
					if (obstacleFlag)
					{
						obstacleFlag = obstacleFlag - 1;
					}
					write(sockfd, backbuf, 5);//头车将信息返回给服务器
				}
				else
				{
					MOTOR_GO_STOP;
				}
			}
		}
		else if (z == 0)
		{
			printf("read is done\n");
			break;
		}
	}
	//关闭套接字
	close(sockfd);
	return 0;
}

