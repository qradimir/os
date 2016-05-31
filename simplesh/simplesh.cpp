#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#include <cstring>
#include <string>
#include <vector>
#include <sstream>

const size_t BUF_CAPACITY = 1000;

using namespace std;

void write_all(int fd, const char *buf, size_t len);
void write_all(int fd, const string &str);

vector<pid_t> children{};

inline void check_error() {
	if (errno != EINTR) {
		exit(errno);
	}
}

inline void check_error(size_t res) {
	if (res == -1) check_error();
}	

void check_error(string const &error_str) {
	if (errno != EINTR) {
		write_all(STDERR_FILENO, "error occurs during " + error_str + " : " + strerror(errno) + "\n");
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

int* firstfd;

char **get_args(const string &command) {
	vector<string> words;
	split(command, ' ', words);
	
	char **args = new char *[words.size() + 1];
	for (size_t i = 0; i < words.size(); ++i) {
		args[i] = new char[words[i].size()];
		strcpy(args[i], words[i].c_str());
	}
	args[words.size()] = NULL;
	return args;
}

bool try_close(int fd) {
	while (close(fd) == -1) {
		if (errno != EINTR) 
			return false;
	}
	return true;
}

void try_duplicate(int fd1, int fd2) {
	while (dup2(fd1, fd2) == -1) {
		check_error();
	}
}

void close_pipe(int *pipefd) {
	try_close(pipefd[0]);
	try_close(pipefd[1]);
	delete [] pipefd;
}

bool exec_command(const string &command, int *infd, int *outfd) {
	char **args = get_args(command);
	pid_t cpid = fork();
	if (cpid == -1) return false;
	if (cpid == 0) {
		try_duplicate(infd[0], STDIN_FILENO);
		if (outfd != NULL) try_duplicate(outfd[1], STDOUT_FILENO);
		if (infd != firstfd) close_pipe(infd);
		if (outfd != NULL) close_pipe(outfd);
		close_pipe(firstfd);
		(execvp(args[0], const_cast<char* const*>(args)), "execution a command \'" + string{args[0]} + "\'");
	}
	children.push_back(cpid);
	return true;
}

bool exec_commandpipe(const vector<string> &cpipe) {
	int *infd = new int[2], *outfd = new int[2];
	firstfd = infd;
	if (pipe(infd) == -1) return false;

	for (int i = 0; i < cpipe.size() - 1; ++i) {
		if (pipe(outfd) == -1) {
			if (firstfd != infd) close_pipe(infd);
			close_pipe(firstfd);
			return false;
		}
		if (!exec_command(cpipe[i], infd, outfd)) {
			if (firstfd != infd) close_pipe(infd);
			close_pipe(firstfd);
			close_pipe(outfd);
			return false;
		}
		if (firstfd != infd) close_pipe(infd);
		infd = outfd;
		outfd = new int[2];
	}
	bool success = exec_command(cpipe[cpipe.size() - 1], infd, NULL);
	if (firstfd != infd) close_pipe(infd);
	return success;
}

void write_all(int fd, const char *buf, size_t len) {
	while (len > 0) {
		ssize_t writen = write(fd, buf, len);
		if (writen == -1) {
			check_error();
			continue;
		}
		buf += writen;
		len -= writen;
	}	
}

void write_all(int fd, const string &str) {
	write_all(fd, str.c_str(), str.size());
}

bool first_dead = false;
bool sig_intr = false;

void signal_handler(int signal, siginfo_t *siginfo, void *ptr) {
	if (signal == SIGINT) sig_intr = true;
	if (signal == SIGCHLD && siginfo->si_pid == children[0]) first_dead = true; 	
}

void check_sig_intr() {
	if (sig_intr) {
		for (pid_t child : children) 
			kill(child, SIGINT);
	}
	sig_intr = false;
}

const string INV{"$ "};

int main() {
	struct sigaction sa;
	sa.sa_sigaction = &signal_handler;
	sa.sa_flags = SA_SIGINFO;
	check_error(sigemptyset(&sa.sa_mask), "calling sigemptyset");
	check_error(sigaddset(&sa.sa_mask, SIGINT), "calling sigaddset with SIGINT");
	check_error(sigaddset(&sa.sa_mask, SIGCHLD), "calling sigaddset with SIGCHLD");
	check_error(sigaction(SIGINT, &sa, nullptr), "calling sigaction with SIGINT");
	check_error(sigaction(SIGCHLD, &sa, nullptr), "calling sigaction with SIGCHLD");
	
	char buffer[BUF_CAPACITY];
	ssize_t ssize = 0;
	string command{};
	size_t checked_symbols = 0;	
	write_all(STDOUT_FILENO, INV);
	while ((ssize = read(STDIN_FILENO, buffer, BUF_CAPACITY)) != 0 || !command.empty()) {

		if (ssize == -1) {
			check_error("calling read");
			continue;
		}
		size_t size = (size_t) ssize;
		command	+= {buffer, size};
	
		size_t newlinechar = command.find('\n', checked_symbols);
		if (newlinechar == string::npos) {
			checked_symbols = command.size();
			if (size != 0) continue;
		} else {
			checked_symbols = newlinechar;
		}

		if (checked_symbols == 0) {
			command = command.substr(1);
			write_all(STDOUT_FILENO, INV);
			continue;
		}	

		string com = command.substr(0, checked_symbols);
		string tail;
		if (checked_symbols < command.size()) tail = command.substr(checked_symbols + 1);

		vector<string> subcommands{};
		split(com, '|', subcommands);
		bool success = exec_commandpipe(subcommands);
		if (success) {
			write_all(firstfd[1], tail);
			while(!sig_intr && !first_dead) {
				ssize = read(STDIN_FILENO, buffer, BUF_CAPACITY);
				if (ssize == -1) {
					check_error("calling read");
					continue;
				}	
				if (ssize == 0) {
					try_close(firstfd[1]);
					break;
				}
	
				write_all(firstfd[1], buffer, (size_t) ssize);
			}
		}

		check_sig_intr();

		for (pid_t child : children) {
			while (true) {
				if (waitpid(child, NULL, 0) == -1) {
					check_sig_intr();
					continue;
				}
				break;
			}
		}

		children.clear();
		checked_symbols = 0;
		command.clear();

		if (success) {	
			try_close(firstfd[1]);
			while ((ssize = read(firstfd[0], buffer, BUF_CAPACITY)) != 0) {
				if (ssize == -1) {
					check_error("calling read");
					continue;
				}
				command += string{buffer, (size_t) ssize};
			}
			try_close(firstfd[0]);
			delete [] firstfd;
		}
		sig_intr = false;
		first_dead = false;

		write_all(STDOUT_FILENO, INV);
	}
}
