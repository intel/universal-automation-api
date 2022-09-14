/*
File Name : debug_screen.h
Description : The header file of debug_screen.cpp
*/

#pragma once

#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <regex>

#define DEFAULT_SERIAL_KEYWORDS_CONFIG_FILE "serial_keywords.ini"
#define REGEX_MAX_STACK_COUNT 100 // the maximum length of string when regular matching

using namespace std;

struct Vt100Cmd
{
    string description_;
    vector<string> params_;
    Vt100Cmd(string description, vector<string> values);
};

struct ParseConfig
{
    string RegularExpression;
    string Description;
    string MarkChar;
    regex RegPattern;
};

void Strcpy(char chs[], string str, int len);

class DebugScreen
{
private:
    vector<Vt100Cmd> GetScreenInfo(string serial_data, bool in_bios, bool ec_off);
    vector<string> GetSegmentWordInfo(string markchars, string pattern_result);

public:
    DebugScreen(bool is_edk_shell = false);
    vector<ParseConfig> cfg_file_info_;
    vector<Vt100Cmd> SerialOutputSplit(string serial_output, bool in_bios = true, bool ec_off = false);
};

vector<string> strsplit(string input_str, string delim);
