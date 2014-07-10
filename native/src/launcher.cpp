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

#ifdef WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <launcher.h>
#include <stdio.h>
#include <jni.h>
#include <string>
#include <iostream>
#include <fstream>
#include <picojson.h>

extern std::string getExecutableDir();
extern bool changeWorkingDir(std::string dir);
extern int g_argc;
extern char** g_argv;

typedef jint (JNICALL *PtrCreateJavaVM)(JavaVM **, void **, void *);

static char* copyStdString(std::string source) {
    char* target = (char*)malloc(source.length() + 1);
#ifdef WINDOWS
    strncpy_s(target, source.length() + 1, source.c_str(), source.length() + 1);
#else
    strncpy(target, source.c_str(), source.length() + 1);
#endif
    return target;
}

static PtrCreateJavaVM loadJavaVM(std::string execDir) {

#ifndef WINDOWS
    #ifdef MACOSX
        std::string jre = execDir + std::string("/jre/lib/server/libjvm.dylib");
    #elif defined(__LP64__)
        std::string jre = execDir + std::string("/jre/lib/amd64/server/libjvm.so");
    #else
        std::string jre = execDir + std::string("/jre/lib/i386/server/libjvm.so");
    #endif

    printf("jre: %s\n", jre.c_str());
    
    void* handle = dlopen(jre.c_str(), RTLD_LAZY);
    if(handle == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    PtrCreateJavaVM ptrCreateJavaVM = (PtrCreateJavaVM)dlsym(handle, "JNI_CreateJavaVM");
    if(ptrCreateJavaVM == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
#else
    HINSTANCE hinstLib = LoadLibrary(TEXT("jre\\bin\\server\\jvm.dll"));
    PtrCreateJavaVM ptrCreateJavaVM = (PtrCreateJavaVM)GetProcAddress(hinstLib,"JNI_CreateJavaVM");
#endif

    return ptrCreateJavaVM;
}

static void launchVMWithJNI(PtrCreateJavaVM ptrCreateJavaVM, std::string main, std::string classPath, picojson::array vmArgs) {
    JavaVMOption* options = (JavaVMOption*)malloc(sizeof(JavaVMOption) * (1 + vmArgs.size()));
    options[0].optionString = copyStdString(classPath);
    for(unsigned int i = 0; i < vmArgs.size(); i++) {
        options[i+1].optionString = copyStdString(vmArgs[i].to_str());
        printf("vmArg %d: %s\n", i, options[i+1].optionString);
    }
    
    JavaVMInitArgs args;
    args.version = JNI_VERSION_1_6;
    args.nOptions = 1 + (int)vmArgs.size();
    args.options = options;
    args.ignoreUnrecognized = JNI_FALSE;
    
    JavaVM* jvm = 0;
    JNIEnv* env = 0;

    jint res = ptrCreateJavaVM(&jvm, (void**)&env, &args);
    if(res < 0) {
        fprintf(stderr, "Failed to create Java VM\n");
        exit(EXIT_FAILURE);
    }

    jobjectArray appArgs = env->NewObjectArray(g_argc, env->FindClass("java/lang/String"), NULL);
    for(int i = 0; i < g_argc; i++) {
        jstring arg = env->NewStringUTF(g_argv[i]);
        env->SetObjectArrayElement(appArgs, i, arg);
    }
    
    jclass mainClass = env->FindClass(main.c_str());
    jmethodID mainMethod = env->GetStaticMethodID(mainClass, "main", "([Ljava/lang/String;)V");
    if(mainMethod == 0) {
        fprintf(stderr, "Failed to aquire main() method of class: %s:\n", main.c_str());
        exit(EXIT_FAILURE);
    }
    env->CallStaticVoidMethod(mainClass, mainMethod, appArgs);
    jvm->DestroyJavaVM();

    for(int i = 0; i < args.nOptions; i++) {
        free(options[i].optionString);
    }
    free(options);
}

static void launchVMWithExec(std::string main, std::string jarFile, picojson::array vmArgs) {
    char** args = (char**)malloc(vmArgs.size() + 4);
    unsigned int numVMArgs = vmArgs.size();

    args[0] = copyStdString(std::string("java"));

    for(unsigned int i = 0; i < numVMArgs; i++) {
        args[i+1] = copyStdString(vmArgs[i].to_str());
    }

    args[numVMArgs+1] = copyStdString(std::string("-jar"));
    args[numVMArgs+2] = copyStdString(jarFile);
    args[numVMArgs+3] = NULL;

    printf("command line:");
    for(unsigned int i = 0; i < numVMArgs+3; i++) {
        printf(" %s", args[i]);
    }
    printf("\n");

#ifndef WINDOWS
    execv("jre/bin/java", args);
#endif
    
    for(unsigned int i = 0; i < numVMArgs + 4; i++) {
        free(args[i]);
    }
    free(args);
}

void* launchVM(void* params) {
    std::string execDir = getExecutableDir();
    std::ifstream configFile;
    configFile.open((execDir + std::string("/config.json")).c_str());
    printf("config file: %s\n", (execDir + std::string("/config.json")).c_str());
    
    picojson::value json;
    configFile >> json;
    std::string err = picojson::get_last_error();
    if(!err.empty()) {
        printf("Couldn't parse json: %s\n", err.c_str());
    }
    
    std::string jarFile = execDir + std::string("/") + json.get<picojson::object>()["jar"].to_str();
    std::string main = json.get<picojson::object>()["mainClass"].to_str();
    std::string classPath = std::string("-Djava.class.path=") + jarFile;
    picojson::array vmArgs = json.get<picojson::object>()["vmArgs"].get<picojson::array>();
    printf("jar: %s\n", jarFile.c_str());
    printf("mainClass: %s\n", main.c_str());
    
#ifndef MACOSX
    PtrCreateJavaVM ptrCreateJavaVM = loadJavaVM(execDir);
#endif
    
    if(!changeWorkingDir(execDir)) {
        printf("Couldn't change working directory to: %s\n", execDir.c_str());
    }

#ifndef MACOSX
    launchVMWithJNI(ptrCreateJavaVM, main, classPath, vmArgs);
#else
    launchVMWithExec(main, jarFile, vmArgs);
#endif
    return 0;
}
