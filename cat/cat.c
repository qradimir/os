#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

const size_t BUF_CAPACITY = 1000;


#define CHECK_ERROR(t) if ((t) == -1) { fprintf(stderr, "error : %s\n",  strerror(errno)); return; }

int main() 
{
	char buffer[BUF_CAPACITY];
	ssize_t size;
	while ((size = read(STDIN_FILENO, buffer, BUF_CAPACITY)) != 0) 
	{
		CHECK_ERROR(size);		
		ssize_t writen = 0;
		while (writen < size) 
		{
			ssize_t c = write(STDOUT_FILENO, buffer + writen, size - writen);
			CHECK_ERROR(c);
			writen += c;
		}	
	} 
	return 0;
}
