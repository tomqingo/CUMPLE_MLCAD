#include "strPrint.h"
#include <chrono>
#include <ios>
#include <stdio.h>
#include <stdlib.h>
#include <string>

std::chrono::time_point<std::chrono::steady_clock> oriTime;

void print_cmd(std::string tmp_string)
{
    // printf("\x1b[%d;%dm%s\x1b[%dm \x1b[0;0m\x1b[0m %s\n", 40, 33, "Cumple CMD: ", 0, tmp_string.c_str());
}

void print_info(std::string tmp_string)
{
    // printf("\x1b[%d;%dm%s\x1b[%dm \x1b[0;0m\x1b[0m %s\n", 40, 34, "Cumple INFO: ", 0, tmp_string.c_str());
}

void print_status(std::string tmp_string)
{
    // auto nowTime = std::chrono::steady_clock::now();
    // auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - oriTime).count();
    // printf("\x1b[%d;%dm%s\x1b[%dm \x1b[0;0m\x1b[0m %s (elapsed time: %.3lf s)\n", 40, 32, "Cumple STATUS: ", 0,
    //        tmp_string.c_str(), (double)millis / 1000);
}

void print_error(std::string tmp_string)
{
    // printf("\x1b[%d;%dm%s\x1b[%dm \x1b[0;0m\x1b[0m %s\n", 43, 31, "Cumple ERROR: ", 0, tmp_string.c_str());
}

void print_warning(std::string tmp_string)
{
    // printf("\x1b[%d;%dm%s\x1b[%dm \x1b[0;0m\x1b[0m %s\n", 43, 31, "Cumple WARNING: ", 0, tmp_string.c_str());
}

std::string to_string_align3(int __val)
{
    return __gnu_cxx::__to_xstring<std::string>(&std::vsnprintf, 4 * sizeof(int), "%3d", __val);
}

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void printProgress(double percentage)
{
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}
