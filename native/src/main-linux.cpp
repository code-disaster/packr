/*******************************************************************************
 * Copyright 2011 See AUTHORS file.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/
 
#include <launcher.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

std::string getExecutableDir() {
	char dest[PATH_MAX];
	int result = readlink("/proc/self/exe", dest, PATH_MAX - 1);
	if (result == -1) {
		return std::string(".");
	} else {
		dest[result] = 0;
		strrchr(dest, '/')[0] = 0;
		return std::string(dest);
	}
}

bool changeWorkingDir(std::string dir) {
	return chdir(dir.c_str()) == 0;
}

std::string getJavaHomeDir() {
    FILE* fp = popen("/bin/readlink -f /usr/bin/java | sed \"s:/jre/bin/java::\"", "r");
    if (fp == NULL) {
        return std::string("");
    }

    char buf[PATH_MAX];
    std::string output;
    while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
        output.append(buf);
    }

    size_t pos = 0;
    while ((pos = output.find("\n")) != std::string::npos) {
        output.erase(pos, 1);
    }

    printf("System JRE: %s\n", output.c_str());

    return output;
}

int g_argc;
char** g_argv;


int main(int argc, char** argv) {
	g_argc = argc;
	g_argv = argv;

	launchVM(0);
	return 0;
}
