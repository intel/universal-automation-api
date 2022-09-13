/*
File Name : debug_screen.cpp
Description : This file is designed to analyze serial port data.
Author : S3E-FIA-FAI
*/

#ifdef __linux__
#define STRTOK strtok_r
#else
#define STRTOK strtok_s
#endif 

#include "debug_screen.h"
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

Vt100Cmd::Vt100Cmd(string description, vector<string> params)
{
    description_ = description;
    params_ = params;
}

void Strcpy(char chs[], string str, int len)
{
    len = str.length() < len - 1 ? str.length() : len - 1;
    // cout << sizeof(chs) << " " << len << endl;
    for (int i = 0; i < len; i++)
    {
        chs[i] = str[i];
    }
    chs[len] = '\0';
}

vector<string> strsplit(string input_str, string delim)
{
    /*
        Function Name       : strsplit()
        Parameters          : input_str: the input string
                              delim: the symbol that splits the string
        Functionality       : split the input string by delim
        Return Value        : vector string after splited
    */
    vector<string> res;
    if (input_str == "")
        return res;
    char *strs = new char[input_str.length() + 1];
    Strcpy(strs, input_str.c_str(), input_str.length() + 1);

    char *d = new char[delim.length() + 1];
    Strcpy(d, delim.c_str(), delim.length() + 1);

    char *next_token = nullptr;
    char *p = STRTOK(strs, d, &next_token);
    while (p)
    {
        string s = p;
        res.push_back(s);
        p = STRTOK(NULL, d, &next_token);
    }
    next_token = nullptr;
    delete[] strs;
    delete[] d;
    return res;
}

void strreplace(string &input_str, string old_str, string new_str)
{
    /*
        Function Name       : strreplace()
        Parameters          : input_str: the input string
                              old_str: the string need to be replaced
                              new_str: the new string to replace the old string
        Functionality       : replace the old string by new string in the input string
        Return Value        : None
    */
    for (string::size_type pos(0); pos != string::npos; pos += new_str.length())
    {
        if ((pos = input_str.find(old_str, pos)) != string::npos)
        {
            input_str.replace(pos, old_str.length(), new_str);
        }
        else
        {
            break;
        }
    }
}

DebugScreen::DebugScreen(bool is_edk_shell)
{
    /*
    Function Name       : DebugScreen()
    Parameters          : is_edk_shell: current status is in edk shell or not.
    Functionality       : Initialize config information
    Return Value        : None
    */
    ParseConfig display_accributes;
    if (is_edk_shell == true)
        display_accributes.RegularExpression = "\\d{1,2}m(.|\\n|\\r)*";
    else
        display_accributes.RegularExpression = "^\\d{1,2}m$";
    display_accributes.Description = "display_attributes";
    display_accributes.MarkChar = "m";
    regex da_pattern(display_accributes.RegularExpression);
    display_accributes.RegPattern = da_pattern;

    ParseConfig cursor_position;
    if (is_edk_shell == true)
        cursor_position.RegularExpression = ".{1,2};.{1,2}H(.|\\n|\\r)*";
    else
        cursor_position.RegularExpression = ".{1,2};.{1,2}H.*";
    cursor_position.Description = "cursor_position_start";
    cursor_position.MarkChar = ";,H";
    regex cp_pattern(cursor_position.RegularExpression);
    cursor_position.RegPattern = cp_pattern;

    cfg_file_info_ = {display_accributes, cursor_position};
}

vector<string> DebugScreen::GetSegmentWordInfo(string markchars, string pattern_result)
{
    /*
        Function Name   :GetSegmentWordInfo()
        Parameter       :markchars:the attribute name defined by vt100，such as: m,H
                         pattern_results:the chars include the attribute name and
                         value obtained by regular matching
        Functionality   :get the attribute value through markchars
                         example: pattern_results=37m;
                                  markchars='m'
                                  attribute_value=37
        Return          :the attribute value, else raise
    */
    vector<string> parm_attr_val;
    vector<string> markchars_split = strsplit(markchars, ",");
    string::size_type start_index = 0;
    for (auto it = markchars_split.begin(); it != markchars_split.end(); it++)
    {
        int end_index = pattern_result.find(*it);
        if (end_index != string::npos)
        {
            int val_length = end_index - start_index;
            parm_attr_val.push_back((pattern_result.substr(start_index, val_length)));
            start_index = end_index + 1;
        }
    }
    parm_attr_val.push_back(pattern_result.substr(start_index, pattern_result.size() - start_index));
    return parm_attr_val;
}

vector<Vt100Cmd> DebugScreen::GetScreenInfo(string serial_data, bool in_bios = true, bool ec_off = false)
{
    /*
        Function Name   :GetScreenInfo()
        Parameter       :serial_data:the serial data from serial port
                         in_bios: serial data get from bios or not
                         ec_off: turn off ec related data or not
        Functionality   :parse the serial data
        Return          :vector Vt100Cmd objects
    */
    vector<Vt100Cmd> screen_info;
    strreplace(serial_data, "\x1b[2J\x1b[01;01H", "");
    string split_chars = "\x1b";
    vector<string> serial_splitinfo = strsplit(serial_data, split_chars);
    for (auto it = serial_splitinfo.begin(); it != serial_splitinfo.end(); it++)
    {
        string segment_words = *it;
        if (segment_words != "")
        {
            if (in_bios == true)
            {
                string ec_end_mark = "\r\n";
                string::size_type ec_start_index = segment_words.find("EC Command:");
                string::size_type ec_end_index = segment_words.rfind(ec_end_mark);
                // in EC Command in segment_words
                if (ec_start_index != string::npos && ec_end_index != string::npos)
                {
                    segment_words = segment_words.substr(0, ec_start_index) + segment_words.substr((ec_end_index + ec_end_mark.size()));
                }
                string::size_type fv_start_index = segment_words.find("FvbProtocolWrite:");
                string::size_type fv_end_index = segment_words.rfind(ec_end_mark);
                if (fv_start_index != string::npos && fv_end_index != string::npos)
                {
                    segment_words = segment_words.substr(0, fv_start_index) + segment_words.substr((fv_end_index + ec_end_mark.size()));
                }
            }
            else if (ec_off == true)
            {
                while (true)
                {
                    string ec_end_mark = "\r\n";
                    string::size_type ec_start_index = segment_words.find("EC Command:");
                    if (ec_start_index == string::npos)
                        break;
                    string ec_data = segment_words.substr(ec_start_index);
                    string::size_type ec_end_index = ec_data.find("Receiving EC Data:");
                    if (ec_start_index != string::npos && ec_end_index != string::npos)
                    {
                        ec_end_index += ec_start_index;
                        string end_index_data = segment_words.substr(ec_end_index);
                        string::size_type end_index = end_index_data.find(ec_end_mark);
                        if (end_index != string::npos)
                        {
                            end_index = ec_end_index + end_index + ec_end_mark.size();
                        }
                        else
                        {
                            break;
                        }
                        segment_words = segment_words.substr(0, ec_start_index) + segment_words.substr(end_index);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if (segment_words.substr(0, 1) == "[")
                segment_words = segment_words.substr(1);
            for (auto it2 = cfg_file_info_.begin(); it2 != cfg_file_info_.end(); it2++)
            {
                ParseConfig cfg_info = *it2;
                string dec = cfg_info.Description;
                string markchars = cfg_info.MarkChar;

                bool is_matched = regex_match(segment_words.substr(0, segment_words.size() < 100 ? segment_words.size() : 100), cfg_info.RegPattern);
                if (is_matched == true)
                {
                    vector<string> parm_attr_val = GetSegmentWordInfo(markchars, segment_words);
                    if (parm_attr_val.size() > 2)
                    {
                        vector<string> parm = {parm_attr_val[0], parm_attr_val[1]};
                        screen_info.push_back(Vt100Cmd(dec, parm));
                        vector<string> content = {parm_attr_val[2]};
                        string content_key = "draw";
                        screen_info.push_back(Vt100Cmd(content_key, content));
                    }
                    else if (parm_attr_val.size() == 2)
                    {
                        vector<string> parm = {parm_attr_val[0]};
                        screen_info.push_back(Vt100Cmd(dec, parm));
                        if (parm_attr_val[1] != "")
                        {
                            string content_key = "draw";
                            vector<string> content = {parm_attr_val[1]};
                            screen_info.push_back(Vt100Cmd(content_key, content));
                        }
                    }
                }
            }
        }
    }
    return screen_info;
}

vector<Vt100Cmd> DebugScreen::SerialOutputSplit(string serial_output, bool in_bios, bool ec_off)
{
    /*
        Function Name   :SerialOutputSplit()
        Parameters      :serial_output: the serial data output from serial port
        Functionality   :Convert the serial output to 'Vt100Cmd' object that
                         describe the debug information.
        Return          :A vector of Vt100Cmd objects.
    */
    vector<Vt100Cmd> parse_ret = GetScreenInfo(serial_output, in_bios, ec_off);
    return parse_ret;
}
