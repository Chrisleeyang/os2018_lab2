#include "lib.h"
#include "types.h"
#include <sys/syscall.h>
#include <stdarg.h>

/*
 * io lib here
 * 库函数写在这
 */

static int ptr = 0;
static char buf[1024];

int32_t syscall(int num, uint32_t a1,uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret = 0;

	/* 内嵌汇编 保存 num, a1, a2, a3, a4, a5 至通用寄存器*/

	asm volatile("int $0x80"
		     :"=a"(ret) 
		     :"a"(num), 
		      "b"(a1), 
		      "c"(a2), 
		      "d"(a3),
		      "S"(a4),
		      "D"(a5));
		
	return ret;
}

void str2buf(char *str)
{
	while(*str)
		buf[ptr++] = *(str++);
}

void ch2buf(char ch)
{
	buf[ptr++] = ch;
}	

void minus2buf(int a)
{	
	if(a < 0)	
	{
		if(a == -2147483648)
		{ 
                      str2buf("-2147483648");
 			return;
		}
		ch2buf('-');
		minus2buf(-a);	
		return ;
	}
	if(a == 0)
		return ;
	minus2buf(a / 10);
	buf[ptr++] = (a % 10 + '0');
}

void hex2buf(unsigned a)
{
	if(a == 0)
  		return ;
	hex2buf(a / 16);
	a = a%16;
	if(a < 10)
		buf[ptr++] = a + '0';
	else
		buf[ptr++] = (a-10) + 'a';
}


void printf(const char *format,...){
        char tempChar;
	int tempInt;
	char *tempString;



	va_list vp;
	va_start(vp,format);

	char *stepPointer =(char *) format;
	while(*stepPointer)
	{
		if(*stepPointer=='%')
		{
			stepPointer++;
			switch(*stepPointer)
			{

				case 'c':
					tempChar=(char)va_arg(vp,int);	 /* need a cast here since va_arg only	*/
                                        ch2buf(tempChar);				/*takes fully promoted types 			*/
					stepPointer++;
					break;
				case 'd':
					tempInt=va_arg(vp,int);
					if(tempInt==0)
						ch2buf('0');
					else
						minus2buf(tempInt);
					stepPointer++;
					break;
				case 'x':
					tempInt=va_arg(vp,int);
					if(tempInt==0)
						ch2buf('0');
					else
						hex2buf(tempInt);
					stepPointer++;
					break;
				case 's':
					tempString = va_arg(vp, char*);
					str2buf(tempString);
					stepPointer++;
					break;
				case '%':
					ch2buf('%');
					stepPointer++;
					break;
			}
		}
		else
		{
			ch2buf(*(stepPointer++));
		}
	}
	va_end(vp);

	buf[ptr]='\0';
	syscall(SYS_write,1,(int32_t)buf,ptr,0,0);
	ptr=0;
}
