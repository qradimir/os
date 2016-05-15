#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#include <string>
#include <vector>
#include <sstream>

const size_t BUF_CAPACITY = 1000;

using namespace std;

void check_error(string const &error_str) {
	if (errno != EINTR) {
		fprintf(stderr, "error occurs during <%s> : %s\n", error_str.c_str(), strerror(errno));
		exit(errno);
	}
}

void check_error(ssize_t res, string const& error_str) {
	if (res == -1) check_error(error_str);	
}

vector<string> &split(const string &s, char delim, vector<string> &elems) {
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		if (item != "") elems.push_back(item);
	}
	return elems;
}

vector<pid_t> children{};

void exec_command(string command, int infd, int outfd) {
	vector<string> words{};
	split(command, ' ', words);
	
	const char*  *args = new const char *[words.size() + 1];
	for (size_t i = 0; i < words.size(); ++i) {
		args[i] = words[i].c_str();
	}
	args[words.size()] = NULL;

	pid_t cpid = fork();
	if (cpid == 0) {
		dup2(infd, STDIN_FILENO);
		dup2(outfd, STDOUT_FILENO);
		check_error(execvp(words[0].c_str(), const_cast<char* const*>(args)), "execution a command");
	}
	children.push_back(cpid);
}

int* exec_commandpipe(vector<string> cpipe) {
	int* pipefd[cpipe.size()];
	for (int i = 0; i < cpipe.size(); ++i) {
		pipefd[i] = new int[2];
		pipe2(pipefd[i], O_CLOEXEC);
	}
	for (int i = 1; i < cpipe.size(); ++i) {
		exec_command(cpipe[i - 1], pipefd[i - 1][0], pipefd[i][1]);
	}
	exec_command(cpipe[cpipe.size() - 1], pipefd[cpipe.size() - 1][0], STDOUT_FILENO);
	for (int i = 1; i < cpipe.size(); ++i) {
		close(pipefd[i][0]);
		close(pipefd[i][1]);
		delete [] pipefd[i];
	}
	return pipefd[0];
}

void move_buffer_pointer(const char * &buf, size_t &len, size_t offset) {
	buf += offset;
	len -= offset;
}

void write_all(int fd, const char *buf, size_t len) {
	while (len != 0) {
		ssize_t writen = write(fd, buf, len);
		if (writen == -1) {
			check_error("calling write");
			continue;
		}
		move_buffer_pointer(buf, len, writen);
	}	
}


size_t dead_children = 0;
bool sig_intr = false;

void signal_handler(int signal) {
	if (signal == SIGINT) sig_intr = true;
	if (signal == SIGCHLD) dead_children++; 	
}

const char * INV = "$ ";

int main() {
	struct sigaction sa;
	sa.sa_handler = &signal_handler;
	sa.sa_flags = 0;
	check_error(sigemptyset(&sa.sa_mask), "calling sigemptyset");
	check_error(sigaddset(&sa.sa_mask, SIGINT), "calling sigaddset with SIGINT");
	check_error(sigaddset(&sa.sa_mask, SIGCHLD), "calling sigaddset with SIGCHLD");
	check_error(sigaction(SIGINT, &sa, nullptr), "calling sigaction with SIGINT");
	check_error(sigaction(SIGCHLD, &sa, nullptr), "calling sigaction with SIGCHLD");
	
	char buffer[BUF_CAPACITY];
	ssize_t ssize = 0;
	string command{};
	size_t checked_symbols = 0;	
	write_all(STDOUT_FILENO, INV, 2);
	while ((ssize = read(STDIN_FILENO, buffer, BUF_CAPACITY)) != 0 || !command.empty()) {
		const char *buf = buffer;
		if (ssize == -1) {
			check_error("calling read");
			continue;
		}
		size_t size = (size_t) ssize;
		command	+= string{buffer, size};

		
		size_t newlinechar = command.find('\n', checked_symbols);
		if (newlinechar == string::npos) {
			checked_symbols = command.size();
			if (size != 0) continue;
		} else {
			checked_symbols = newlinechar;
		}

		if (newlinechar == 0) {
			command = command.substr(1);
			write_all(STDOUT_FILENO, INV, 2);
			continue;
		}	

		vector<string> subcommands{};
		split(command.substr(0, checked_symbols), '|', subcommands);
		int* pipefd = exec_commandpipe(subcommands);
		
		if (checked_symbols < command.size()) {
			const char * buf1 = command.c_str();
			size_t size1 = command.size(); 
			move_buffer_pointer(buf1, size1, checked_symbols + 1);	
			write_all(pipefd[1], buf1, size1);
		}	
		do {
			ssize = read(STDIN_FILENO, buffer, BUF_CAPACITY);
			if (ssize == -1) {
				check_error("calling read");
				continue;
			}
			size = (size_t) ssize;
			write_all(pipefd[1], buf, size);

		} while (!sig_intr && dead_children < children.size() && ssize > 0);
		
		if (sig_intr) {
			for (size_t pid : children) 
				kill(pid, SIGINT);		
		}


		children.clear();
		checked_symbols = 0;
		command.clear();

		close(pipefd[1]);
		while ((ssize = read(pipefd[0], buffer, BUF_CAPACITY)) != 0) {
			if (ssize == -1) {
				check_error("calling read");
				continue;
			}
			command += string{buffer, (size_t) ssize};
		}
		close(pipefd[0]);

		sig_intr = false;
		dead_children = 0;
		delete [] pipefd;

		write_all(STDOUT_FILENO, INV, 2);
	}
}
