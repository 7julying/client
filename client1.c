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

#define BUFSIZE 512 //缓冲区大小
#define RIGHT 26 //传感器output接在GPIO.26

//通过定义四个电机的状态定义小车状态
#define MOTOR_GO_FORWARD   digitalWrite(1,HIGH);digitalWrite(4,LOW);digitalWrite(5,HIGH);digitalWrite(6,LOW) 
#define MOTOR_GO_BACK	   digitalWrite(4,HIGH);digitalWrite(1,LOW);digitalWrite(6,HIGH);digitalWrite(5,LOW)
#define MOTOR_GO_RIGHT	   digitalWrite(1,HIGH);digitalWrite(4,LOW);digitalWrite(6,HIGH);digitalWrite(5,LOW)
#define MOTOR_GO_LEFT	   digitalWrite(4,HIGH);digitalWrite(1,LOW);digitalWrite(5,HIGH);digitalWrite(6,LOW)
#define MOTOR_GO_STOP	   digitalWrite(1, LOW);digitalWrite(4,LOW);digitalWrite(5, LOW);digitalWrite(6,LOW)

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
	digitalWrite(1, HIGH);
	digitalWrite(4, HIGH);
	digitalWrite(5, HIGH);
	digitalWrite(6, HIGH);

	digitalWrite(1, HIGH);

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
	char buf[BUFSIZE] = { 0xff, 0x00, 0x00, 0x00, 0xff }; //定义并初始化读空间
	char backbuf[BUFSIZE] = { 0xff, 0x00, 0x00, 0x00, 0xff }; //定义并初始化写空间
    int obstacle;//定义状态接收传感器输出状态
	fcntl(sockfd,F_SETFL,fcntl(sockfd,F_GETFL,0)|O_NONBLOCK);//将套接字设置为非阻塞模式
	while (1)
	{
	obstacle = digitalRead(RIGHT);
	//小车遇到障碍，GPIO.26输出低电平，头车将状态返回给服务器
		if (obstacle == LOW)
		{
			MOTOR_GO_STOP; printf("obstacle\n"); backbuf[0] = 0xff; backbuf[1] = 0x00; 
			backbuf[2] = 0x00; backbuf[3] = 0x00; backbuf[4] = 0xff;	write(sockfd, backbuf, 5);                     
		}
		if ((z = read(sockfd, buf, sizeof buf)) > 0)//读取服务器传输的数据，返回数据长度
		{

			buf[z] = '\0';
			//通过读入数据决定小车状态，0xff000000ff停止，0xff000100ff前进,0xff000200ff后退,0xff000300ff左转,0xff000400ff右转
			if (z == 5)
			{
				if (buf[1] == 0x00)
				{
					switch (buf[2])
					{
					case 0x01:MOTOR_GO_FORWARD; printf("forward\n");memcpy(backbuf,buf,sizeof buf);break;
					case 0x02:MOTOR_GO_BACK;    printf("back\n"); memcpy(backbuf,buf,sizeof buf);break;
					case 0x03:MOTOR_GO_LEFT;    printf("left\n"); memcpy(backbuf,buf,sizeof buf); break;
					case 0x04:MOTOR_GO_RIGHT;   printf("right\n"); memcpy(backbuf,buf,sizeof buf); break;
					case 0x00:MOTOR_GO_STOP;    printf("stop\n"); memcpy(backbuf,buf,sizeof buf); break;
					default: break;
					}
					digitalWrite(3, HIGH);
					write(sockfd, backbuf, 5);//头车将信息返回给服务器
				}
			}
			else if (z == 6)
			{
				if (buf[2] == 0x00)
				{
					switch (buf[3])
					{
					case 0x01:MOTOR_GO_FORWARD; printf("forward\n"); memcpy(backbuf,buf,sizeof buf);break;
					case 0x02:MOTOR_GO_BACK;    printf("back\n"); memcpy(backbuf,buf,sizeof buf); break;
					case 0x03:MOTOR_GO_LEFT;    printf("left\n"); memcpy(backbuf,buf,sizeof buf); break;
					case 0x04:MOTOR_GO_RIGHT;   printf("right\n"); memcpy(backbuf,buf,sizeof buf); break;
					case 0x00:MOTOR_GO_STOP;    printf("stop\n"); memcpy(backbuf,buf,sizeof buf); break;
					default: break;
					}
					digitalWrite(3, HIGH);
					write(sockfd, backbuf, 5);						
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

