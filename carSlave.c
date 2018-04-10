#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#define BUFSIZE 5

#define MOTOR_GO_FORWARD   softPwmWrite(4, 0);softPwmWrite(1, 21);softPwmWrite(6, 0);softPwmWrite(5, 21);
//#define MOTOR_GO_BACK	   digitalWrite(4,HIGH);digitalWrite(1,LOW);digitalWrite(6,HIGH);digitalWrite(5,LOW)
#define MOTOR_GO_RIGHT	  softPwmWrite(1, 21);softPwmWrite(4, 0);softPwmWrite(5, 0);softPwmWrite(6, 21);
#define MOTOR_GO_LEFT	   softPwmWrite(4, 21);softPwmWrite(1, 0);softPwmWrite(5, 21);softPwmWrite(6, 0);
#define MOTOR_GO_STOP	   	softPwmWrite(1, 0); softPwmWrite(4, 0);softPwmWrite(5, 0);softPwmWrite(6, 0);

int main(int argc, char *argv[])
{
	struct timeval tv;
	char *servIP = argv[1];
	in_port_t servPort = atoi(argv[2]);
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int z = -1;

	/*RPI*/
	wiringPiSetup();
	/*WiringPi GPIO*/
	pinMode(1, OUTPUT);	//IN1
	pinMode(4, OUTPUT);	//IN2
	pinMode(5, OUTPUT);	//IN3
	pinMode(6, OUTPUT);	//IN4


	/*Init output*/
	softPwmCreate(1, 1, 100);
	softPwmCreate(4, 1, 100);
	softPwmCreate(5, 1, 100);
	softPwmCreate(6, 1, 100);

	if (argc != 3)
	{
		printf("Please add servIP or servPort!");
		exit(1);
	}
	if (servPort<8000)
	{
		printf("Enter Error");
		exit(1);
	}
	if (sockfd < 0)
	{
		printf("socket error");
		exit(1);
	}
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(servIP);//inet_addr函数将用点分十进制字符串表示的IPv4地址转化为用网络字节序整数表示的IPv4地址 
	servAddr.sin_port = htons(servPort);

	int connectfd=connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr));
	if (connectfd < 0)
	{
		printf("connect error");
		exit(1);
	}
	printf("connecting to the server...\n");
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);
	while (1)
	{
		if ((z = read(sockfd, buf, sizeof buf)) > 0)//读取传输的数据，返回数据长度
		{
			if (z == 5)
			{
				if (buf[0] == 0x00)
				{
					switch (buf[1])
					{
					case 0x01:MOTOR_GO_FORWARD; printf("forward\n"); break;
					case 0x02:MOTOR_GO_STOP;    printf("stop\n");  break;
					case 0x03:MOTOR_GO_LEFT;    printf("left\n");  break;
					case 0x04:MOTOR_GO_RIGHT;   printf("right\n");  break;
					default: break;
					}
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
		else
		{
			printf("read error");
			continue;
		}
	}
	close(sockfd);
	return 0;
}

