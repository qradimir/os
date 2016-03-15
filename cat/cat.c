#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

const size_t BUF_CAPACITY = 1000;



int main(size_t argc, char** argv) 
{
	size_t fd;
	if (argc == 2) {
		fd = open(argv[1], O_RDONLY);
	} else {
		fd = STDIN_FILENO;
	}
	char buffer[BUF_CAPACITY];
	ssize_t size;
	while ((size = read(fd, buffer, BUF_CAPACITY)) != 0) 
	{
		if (size == -1) {
			if (errno == EINTR) {
				continue;
                        } else {
				fprintf(stderr, "error : %s\n",  strerror(errno));
				return 1;
			}
		}
		ssize_t writen = 0;
		while (writen < size) 
		{
			ssize_t c = write(STDOUT_FILENO, buffer + writen, size - writen);
			if (c == -1) {
				if (errno == EINTR) {
					continue;
				} else {
					fprintf(stderr, "error : %s\n",  strerror(errno)); 
					return 1;
				}
			}
			writen += c;
		}	
	}
	if (fd != STDIN_FILENO) {
		close(fd);
	} 
	return 0;
}
