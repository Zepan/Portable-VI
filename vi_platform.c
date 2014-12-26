#define STANDALONE
#ifdef STANDALONE
#define vi_main			main
#endif

#ifndef PLATFORM 
#define PLATFORM linux
#endif

//buf for screen
#define ROWS 24
#define COLUMNS 80
//buf for read buffer, notice buffer, etc.. should >= 64
#define _BUFSIZ  512	
//buf for hold the whole file*2, orginal is 10KB
#define MIN_FILE (1024*4)
//buf for status line.. should >= 64
#define STATUS_LEN (200)



#if PLATFORM == linux
//platform header
#include <termios.h>
//functions
/****************** stdin_select *******************/
#if 1 
//wait for stdin input for t ms
static int stdin_select(int t)
{
	fd_set rfds;		// use select() for small sleeps
	struct timeval tv;	// use select() for small sleeps
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = t/1000;
	tv.tv_usec = (t%1000)*1000;
	return select(1, &rfds, NULL, NULL, &tv);
}

#endif

/******************file & console operation*******************/
#if 1

FILE file;
static int _open(char* fn, int flag)
{
    const char* mode;
	int res;
    switch(flag)
    {
    case O_RDWR: mode = "a+";break;
    case O_RDONLY: mode = "r";break;
    case O_WRONLY: mode = "w";break;
    case O_WRONLY|O_CREAT|O_TRUNC: mode = "w+";break;
    default: mode = "r";break;
    }
    return (int)(long)fopen(fn, (char*)mode);
} 

int c_read(Byte* p, int size)	//stdin read
{
    return read(0, p, size);
}
int _read(int fd, Byte* p, int size)
{   int tmp=size;
    if((int)fd>10) tmp = fread (p, 1, size, (FILE*)(long)fd) ;
    else tmp = c_read(p, size);
    return tmp;
    //return read(fd, p, size);
}

int c_write(Byte* p, int size)	//stdin write
{//	for(; size > 0; size--,p++) putchar(*p);return size;
    return write(1, p ,size);
}
int _write(int fd, Byte* p, int size)
{   int tmp=size;
    if((int)fd>10) tmp = fwrite(p, 1, size, (FILE*)(long)fd);
    else tmp = c_write(p, size);
    return tmp;
    //return write(fd, p, size);
}

void _close(int fd)
{
    fclose((FILE*)(long)fd);
    //close(fd);
}

static int file_size(Byte * fn) // what is the byte size of "fn"
{
  FILE *fp;
	int cnt;

	if (fn == 0 || strlen((const char*)fn) <= 0)
		return (-1);
	cnt = -1;
    
  fp = (FILE*)(long)_open((char *)fn, O_RDONLY);
  if(fp == NULL) return -1;
    
  fseek(fp,0,SEEK_END); 
  cnt =  ftell(fp);
  _close((int)(long)fp);
  return (cnt);
}
#endif


/****************** Set terminal attributes *******************/
#if 1
static struct termios term_orig, term_vi;	// remember what the cooked mode was
static void rawmode(void)
{
	tcgetattr(0, &term_orig);
	term_vi = term_orig;
	term_vi.c_lflag &= (~ICANON & ~ECHO);	// leave ISIG ON- allow intr's
	term_vi.c_iflag &= (~IXON & ~ICRNL);
	term_vi.c_oflag &= (~ONLCR);
#ifndef linux
	term_vi.c_cc[VMIN] = 1;
	term_vi.c_cc[VTIME] = 0;
#endif
	erase_char = 8; //back space
	tcsetattr(0, TCSANOW, &term_vi);
}

static void cookmode(void)
{
	tcsetattr(0, TCSANOW, &term_orig);
}

static void vi_init(void)
{
}
#endif

#endif

#if PLATFORM == stm32
//platform header
#include "common.h"
#include "cbuf.h"
#include "buf.h"
#include "delay.h"
#include "driver.h"

extern CBUF* u1_cbuf;
#define VI_UART USART1
//functions
/****************** stdin_select *******************/
#if 1 
static int stdin_available(void)
{
	return cbuf_len(u1_cbuf);
}

//wait for stdin input for t ms
static int stdin_select(int t)
{
	int t0, flag;
	flag = 0;
	t0 = systime;
	while(systime - t0 < t)
	{
		if(stdin_available() > 0) 
		{	
			flag = 1;
			break;
		}
	}
	return flag;
}

#endif

/******************file & console operation*******************/
#if 1

FIL file;
static int _open(char* fn, int flag)
{
    //const char* mode;
	  int res;
    /*switch(flag)
    {
    case O_RDWR: mode = "a+";break;
    case O_RDONLY: mode = "r";break;
    case O_WRONLY: mode = "w";break;
    case O_WRONLY|O_CREAT|O_TRUNC: mode = "w+";break;
    default: mode = "r";break;
    }*/
    //return (int)fopen(fn, (char*)mode);
		res = (int)f_open( &file , fn , FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
		if(res != 0) return 0;
	  else return (int)&file;
    //return open(fn,flag);
} 

int c_read(Byte* p, int size)	//console read
{
		while(stdin_available() <= 0);	//block to read at least one char
		return cbuf_copy(u1_cbuf, (char*)p, size);
    //return read(0, p, size);
}
int _read(int fd, Byte* p, int size)
{   int tmp=size;
    if((int)fd>10) f_read((FIL*)fd, p, size, (u32*)&tmp );
    else tmp = c_read(p, size);
    return tmp;
    //return read(fd, p, size);
}

int c_write(Byte* p, int size)	//console write
{//for(; size > 0; size--,p++) putchar(*p);return size;
		USART_SendBytes(VI_UART, (char*)p, size);
		return size;
    //return write(1, p ,size);
}
int _write(int fd, Byte* p, int size)
{   int tmp=size;
    if((int)fd>10) f_write((FIL*)fd, p, size, (u32*)&tmp);
    else tmp = c_write(p, size);
    return tmp;
    //return write(fd, p, size);
}

void _close(int fd)
{
    f_close((FIL*)fd);
    //close(fd);
}

static int file_size(Byte * fn) // what is the byte size of "fn"
{
  FIL *fp;
	int cnt;

	if (fn == 0 || strlen((const char*)fn) <= 0)
		return (-1);
	cnt = -1;
    
  fp = (FIL*)_open((char *)fn, O_RDONLY);
  if(fp == NULL) return -1;
    
  cnt = fp->fsize;
	f_close(fp);
	return (cnt);
}
#endif


/****************** Set terminal attributes *******************/
#if 1
static void rawmode(void)   //set to rawmode
{
	erase_char = 8;	//back space
}

static void cookmode(void)  //return to orig mode
{
	
}

static void vi_init(void)
{
	Init_USART(VI_UART, 115200);	
	USART_ITConfig(VI_UART, USART_IT_RXNE, ENABLE); //Ê¹ÄÜÖÐ¶Ï
}
#endif
#endif
