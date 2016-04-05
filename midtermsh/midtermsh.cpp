#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <string>
#include <vector>
#include <sstream>
#include <sys/wait.h>

const size_t BUF_CAPACITY = 1000;

#define SHOW_ERROR fprintf(stderr, "error : %s\n",  strerror(errno)); return 1;

using namespace std;


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
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
		execvp(words[0].c_str(), const_cast<char* const*>(args));
		exit(EXIT_SUCCESS);
	}
	children.push_back(cpid);
}

void exec_commandpipe(vector<string> cpipe) {
	if (cpipe.size() == 1) {
		exec_command(cpipe[0], STDIN_FILENO, STDOUT_FILENO);
		return;
	}
	int* pipefd[cpipe.size() - 1];
	for (int i = 0; i < cpipe.size() - 1; ++i) {
		pipefd[i] = new int[2];
		pipe(pipefd[i]);
	}
	exec_command(cpipe[0], STDIN_FILENO, pipefd[0][1]);
	for (int i = 0; i < cpipe.size() - 2; ++i) {
		exec_command(cpipe[i], pipefd[i - 1][0], pipefd[i][1]);
	}
	exec_command(cpipe[cpipe.size() - 1], pipefd[cpipe.size() - 2][0], STDOUT_FILENO);
}


int main() {
	char buffer[BUF_CAPACITY];
	ssize_t size = 0;
	while ((size = read(STDIN_FILENO, buffer, BUF_CAPACITY)) != 0)
	{
		if (size == -1) {
			if (errno == EINTR) {
				continue;
			} else {
				SHOW_ERROR;
			}
		}
		vector<string> commands{};
		split(string{buffer, (size_t)size}, '\n', commands);
		for (auto it = commands.cbegin(); it != commands.cend(); ++it) {
			string command = *it;
			vector<string> subcommands{};
			split(command, '|', subcommands);
			exec_commandpipe(subcommands);
			for (auto pit = children.cbegin(); pit != children.cend(); ++pit) {
				int res;
				waitpid(*pit, &res, WIFEXITED(res));
			}
			children.clear();
		}
	}
}
