#include <stdio.h>
#include<iostream>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
using namespace std;

void signExpand(string inputline) {
	char wd[256];
	char lastDir[256];
	getcwd(lastDir, 256);
	int stdincpy;
	int stdoutcpy;
	stdincpy = dup(0);
	stdoutcpy = dup(1);
	vector<string> inputCommands;
    string currString;

	if (inputline.find("$(", 0) == -1) {
		return;
	}

	int loc = inputline.find("$(", 0);
	int loc2 = inputline.find(")", loc + 1);
	inputline = inputline.substr(loc + 2, loc2 - loc - 2);
    
    while(inputline.find("|", 0) != -1) {
      int index = inputline.find("|", 0);
	  if (inputline.at(index - 1) == ' ' && inputline.at(index + 1) == ' ') {
		  while (inputline.at(index + 2) == ' ') {
			  inputline.erase(index + 2, 1);
		  }
		  while (inputline.at(index - 2) == ' ') {
			  inputline.erase(index - 2, 1);
			  index--;
		  }
		inputCommands.push_back(inputline.substr(0, index - 1));
      	inputline.erase(0, index + 2);
	  } else if (inputline.at(index - 1) == ' ') {
		  while (inputline.at(index - 2) == ' ') {
			  inputline.erase(index - 2, 1);
			  index--;
		  }
		  inputCommands.push_back(inputline.substr(0, index - 1));
      	  inputline.erase(0, index + 1);
	  } else if (inputline.at(index + 1) == ' ') {
		  while (inputline.at(index + 2) == ' ') {
			  inputline.erase(index + 2, 1);
		  }
		  inputCommands.push_back(inputline.substr(0, index));
      	  inputline.erase(0, index + 1);
	  } else {
		  inputCommands.push_back(inputline.substr(0, index));
      	  inputline.erase(0, index + 1);
	  }
    }
    inputCommands.push_back(inputline);
    
    //get a line from standard input
	string file_update;
	int outfd;
	int infd;
	char buf[256];

    for (size_t i = 0; i < inputCommands.size(); ++i) {
      int fd[2];
      pipe(fd);

      int pid = fork ();
      if (pid == 0){
		if (inputCommands[i] == "") {
			inputCommands.clear();
			break;
		}

		if (inputCommands[i].substr(0, 3) == "cd ") {
			inputCommands[i].erase(0, 3);
			if (inputCommands[i].at(0) != '/') {
				inputCommands[i].insert(0, "/");
			}
			getcwd(buf, 256);

			if (inputCommands[i] == "/-") {
				inputCommands[i] = lastDir;
			}

			if (inputCommands[i].substr(0, 5) != "/home" && inputCommands[i].substr(0, 6) != "/home/")  {
				inputCommands[i].insert(0, buf);
			}

			getcwd(lastDir, 256);

			chdir((char*)(inputCommands[i].substr(0, inputCommands[i].size())).c_str());
			getcwd(buf, 256);
			break;
		}

		if (inputCommands[i].find('>') != -1) {
			int index = inputCommands[i].find('>');

			if (inputCommands[i].at(index + 1) == ' ' && inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index, 2);
			} else if (inputCommands[i].at(index + 1) == ' ') {
				inputCommands[i].erase(index, 1);
				index++;
			} else if (inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index, 1);
			} else {
				inputCommands[i].erase(index, 1);
				inputCommands[i].insert(index, " ");
				index++;
			}
			while (inputCommands[i].at(index) == ' ') {
				inputCommands[i].erase(index, 1);
			}
			while (inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index - 1, 1);
				index--;
			}
			file_update = inputCommands[i].substr(index, inputCommands[i].length() - index);
			inputCommands[i].erase(index, inputCommands[i].length() - index + 1);
			outfd = open((char*)file_update.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			dup2(outfd, 1);
		}
		if (inputCommands[i].find('<') != -1) {
			int index = inputCommands[i].find('<');

			if (inputCommands[i].at(index + 1) == ' ' && inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index, 2);
			} else if (inputCommands[i].at(index + 1) == ' ') {
				inputCommands[i].erase(index, 1);
				index++;
			} else if (inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index, 1);
			} else {
				inputCommands[i].erase(index, 1);
				inputCommands[i].insert(index, " ");
				index++;
			}
			while (inputCommands[i].at(index) == ' ') {
				inputCommands[i].erase(index, 1);
			}
			while (inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index - 1, 1);
				index--;
			}
			file_update = inputCommands[i].substr(index, inputCommands[i].length() - index);
			inputCommands[i].erase(index, inputCommands[i].length() - index + 1);
			infd = open((char*)file_update.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			dup2(infd, 0);
		}
		//child process
		// preparing the input command for execution

		if (i < inputCommands.size() - 1) {
		  dup2(fd[1], 1);
		}

		int numArgs = 0;
		string space = " ";
		string currLine = inputCommands[i];
		char* currCommand = (char*)currLine.c_str();
		char* pos = currCommand;
		
		int numSpaces = 2;
		for (size_t m = 0; m < currLine.length(); ++m) {
		  if (currLine[m] == ' ') {
			numSpaces++;
		  }
		}
		
		char* args[numSpaces];

		for(size_t j = 0; j < currLine.length(); ++j) {
			if (*pos == '`') {
				*pos = ' ';
				pos++;
			}
		  if (currLine.find("'", 0) != -1) {
			  int index = currLine.find("'", 0);
			  currLine.replace(index, 1, "");
			  int index2 = currLine.find("'", index + 1);
			  currLine.replace(index2, 1, "");
			  while (currLine.find(" ", index) < index2) {
				  currLine.replace(currLine.find(" ", index), 1, "`");
			  }
		  } 
		  if (currLine.find('"', 0) != -1) {
			  int index = currLine.find('"', 0);
			  currLine.replace(index, 1, "");
			  int index2 = currLine.find('"', index + 1);
			  currLine.replace(index2, 1, "");
			  while (currLine.find(" ", index) < index2) {
				  currLine.replace(currLine.find(" ", index), 1, "`");
			  }
		  } else if ((*pos) == ' ') {
			(*pos) = NULL;
			if (*currCommand != '\0' && *currCommand != ' ') {
				args[numArgs] = currCommand;
				numArgs++;
			}
			currCommand = pos + 1;
		  } 
		  pos++;
		}
		  if ((*currCommand) != '&'){
			if (*currCommand != '\0' && *currCommand != ' ') {
				args[numArgs] = currCommand;
				numArgs++;
			}
		  }
		  args[numArgs] = NULL;
		  numArgs++;
		// for (size_t k = 0; k < numArgs; ++k) {
		// 	cout << "[" << k << "] " << "..." << args[k] << "..." << endl;
		// }
		execvp (args[0], args);
		close(outfd);
		close(infd);
		dup2(stdincpy, 0);
		dup2(stdoutcpy, 1);
		return;
      } else{
		if (i == inputCommands.size() - 1) {
			waitpid (pid, 0, 0);
			dup2(stdincpy, 0);
			dup2(stdoutcpy, 1);
		}
	  //parent waits for child process
	   if (i < inputCommands.size() - 1) {
	      dup2(fd[0], 0);
	   }

	   close (fd[1]);
	  }
    }
}

int main (){
	vector<int> backgroundPIDS;
	char wd[256];
	char lastDir[256];
	getcwd(lastDir, 256);
	int stdincpy;
	int stdoutcpy;
	stdincpy = dup(0);
	stdoutcpy = dup(1);
	
  while (true){
	  string signEx = "";
	  for (size_t w = 0; w < backgroundPIDS.size(); ++w) {
		int state;
		if (waitpid(backgroundPIDS[w], &state, WNOHANG) != 0) {
			cout << "Background process " << backgroundPIDS[w] << " completed!" << endl;
			backgroundPIDS.erase(backgroundPIDS.begin() + w);
			w--;
		}
	  }
	
    cout << "rosss@Shell$ > "; //print a prompt
	getcwd(wd, 256);
	cout << wd << " ";
    time_t currTime;
    time(&currTime);
    cout << asctime(localtime(&currTime));
    string inputline;
    getline (cin, inputline);

    if (inputline == string("exit")){
      cout << "Bye!! End of shell" << endl;
      break;
    }
    
    vector<string> inputCommands;
    string currString;
	string expansion = "";

	int newfd[2];
	pipe(newfd);
	dup2(newfd[1], 1);
	dup2(newfd[0], 0);
	if (inputline.find("$(") != -1) {
		signExpand(inputline);
		int loc = inputline.find("$(", 0);
		int loc2 = inputline.find(")", loc + 1);
		inputline.erase(loc, loc2 - loc + 1);
		getline(cin, expansion);
		inputline.insert(loc, expansion);
	}
	dup2(stdincpy, 0);
	dup2(stdoutcpy, 1);
    
    while(inputline.find("|", 0) != -1) {
      int index = inputline.find("|", 0);
	  if (inputline.at(index - 1) == ' ' && inputline.at(index + 1) == ' ') {
		  while (inputline.at(index + 2) == ' ') {
			  inputline.erase(index + 2, 1);
		  }
		  while (inputline.at(index - 2) == ' ') {
			  inputline.erase(index - 2, 1);
			  index--;
		  }
		inputCommands.push_back(inputline.substr(0, index - 1));
      	inputline.erase(0, index + 2);
	  } else if (inputline.at(index - 1) == ' ') {
		  while (inputline.at(index - 2) == ' ') {
			  inputline.erase(index - 2, 1);
			  index--;
		  }
		  inputCommands.push_back(inputline.substr(0, index - 1));
      	  inputline.erase(0, index + 1);
	  } else if (inputline.at(index + 1) == ' ') {
		  while (inputline.at(index + 2) == ' ') {
			  inputline.erase(index + 2, 1);
		  }
		  inputCommands.push_back(inputline.substr(0, index));
      	  inputline.erase(0, index + 1);
	  } else {
		  inputCommands.push_back(inputline.substr(0, index));
      	  inputline.erase(0, index + 1);
	  }
    }
    inputCommands.push_back(inputline);
    
    //get a line from standard input
	string file_update;
	int outfd;
	int infd;
	char buf[256];

    for (size_t i = 0; i < inputCommands.size(); ++i) {
      int fd[2];
      pipe(fd);

      int pid = fork ();
      if (pid == 0){
		if (inputCommands[i] == "") {
			inputCommands.clear();
			break;
		}

		if (inputCommands[i].substr(0, 3) == "cd ") {
			inputCommands[i].erase(0, 3);
			if (inputCommands[i].at(0) != '/') {
				inputCommands[i].insert(0, "/");
			}
			getcwd(buf, 256);

			if (inputCommands[i] == "/-") {
				inputCommands[i] = lastDir;
			}

			if (inputCommands[i].substr(0, 5) != "/home" && inputCommands[i].substr(0, 6) != "/home/")  {
				inputCommands[i].insert(0, buf);
			}

			getcwd(lastDir, 256);

			chdir((char*)(inputCommands[i].substr(0, inputCommands[i].size())).c_str());
			getcwd(buf, 256);
			break;
		}

		if (inputCommands[i].find('>') != -1) {
			int index = inputCommands[i].find('>');

			if (inputCommands[i].at(index + 1) == ' ' && inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index, 2);
			} else if (inputCommands[i].at(index + 1) == ' ') {
				inputCommands[i].erase(index, 1);
				index++;
			} else if (inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index, 1);
			} else {
				inputCommands[i].erase(index, 1);
				inputCommands[i].insert(index, " ");
				index++;
			}
			while (inputCommands[i].at(index) == ' ') {
				inputCommands[i].erase(index, 1);
			}
			while (inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index - 1, 1);
				index--;
			}
			file_update = inputCommands[i].substr(index, inputCommands[i].length() - index);
			inputCommands[i].erase(index, inputCommands[i].length() - index + 1);
			outfd = open((char*)file_update.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			dup2(outfd, 1);
		}
		if (inputCommands[i].find('<') != -1) {
			int index = inputCommands[i].find('<');

			if (inputCommands[i].at(index + 1) == ' ' && inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index, 2);
			} else if (inputCommands[i].at(index + 1) == ' ') {
				inputCommands[i].erase(index, 1);
				index++;
			} else if (inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index, 1);
			} else {
				inputCommands[i].erase(index, 1);
				inputCommands[i].insert(index, " ");
				index++;
			}
			while (inputCommands[i].at(index) == ' ') {
				inputCommands[i].erase(index, 1);
			}
			while (inputCommands[i].at(index - 1) == ' ') {
				inputCommands[i].erase(index - 1, 1);
				index--;
			}
			file_update = inputCommands[i].substr(index, inputCommands[i].length() - index);
			inputCommands[i].erase(index, inputCommands[i].length() - index + 1);
			infd = open((char*)file_update.c_str(), O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			dup2(infd, 0);
		}
		//child process
		// preparing the input command for execution

		if (i < inputCommands.size() - 1) {
		  dup2(fd[1], 1);
		}

		int numArgs = 0;
		string space = " ";
		string currLine = inputCommands[i];
		char* currCommand = (char*)currLine.c_str();
		char* pos = currCommand;
		
		int numSpaces = 2;
		for (size_t m = 0; m < currLine.length(); ++m) {
		  if (currLine[m] == ' ') {
			numSpaces++;
		  }
		}
		
		char* args[numSpaces];

		for(size_t j = 0; j < currLine.length(); ++j) {
			if (*pos == '`') {
				*pos = ' ';
				pos++;
			}
		  if (currLine.find("'", 0) != -1) {
			  int index = currLine.find("'", 0);
			  currLine.replace(index, 1, "");
			  int index2 = currLine.find("'", index + 1);
			  currLine.replace(index2, 1, "");
			  while (currLine.find(" ", index) < index2) {
				  currLine.replace(currLine.find(" ", index), 1, "`");
			  }
		  } 
		  if (currLine.find('"', 0) != -1) {
			  int index = currLine.find('"', 0);
			  currLine.replace(index, 1, "");
			  int index2 = currLine.find('"', index + 1);
			  currLine.replace(index2, 1, "");
			  while (currLine.find(" ", index) < index2) {
				  currLine.replace(currLine.find(" ", index), 1, "`");
			  }
		  } else if ((*pos) == ' ') {
			(*pos) = NULL;
			if (*currCommand != '\0' && *currCommand != ' ') {
				args[numArgs] = currCommand;
				numArgs++;
			}
			currCommand = pos + 1;
		  } 
		  pos++;
		}
		  if ((*currCommand) != '&'){
			if (*currCommand != '\0' && *currCommand != ' ') {
				args[numArgs] = currCommand;
				numArgs++;
			}
		  }
		  args[numArgs] = NULL;
		  numArgs++;
		// for (size_t k = 0; k < numArgs; ++k) {
		// 	cout << "[" << k << "] " << "..." << args[k] << "..." << endl;
		// }
		execvp (args[0], args);
		close(outfd);
		close(infd);
		dup2(stdincpy, 0);
		dup2(stdoutcpy, 1);
		return 0;
      } else{
		if (inputCommands[i].at(inputCommands[i].length() - 1) != '&') {
			if (i == inputCommands.size() - 1) {
				waitpid (pid, 0, 0);
				dup2(stdincpy, 0);
				dup2(stdoutcpy, 1);
			}
		} else {
			backgroundPIDS.push_back(pid);
		}
	  //parent waits for child process
	   if (i < inputCommands.size() - 1) {
	      dup2(fd[0], 0);
	   }

	   close (fd[1]);
	  }
    }
	for (size_t w = 0; w < backgroundPIDS.size(); ++w) {
		int state;
		if (waitpid(backgroundPIDS[w], &state, WNOHANG) != 0) {
			cout << "Background process " << backgroundPIDS[w] << " completed!" << endl;
			backgroundPIDS.erase(backgroundPIDS.begin() + w);
			w--;
		}
	}
  }
}