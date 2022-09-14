/*
File Name : vt100_screen_parse.h
Description : The header file of vt100_screen_parse.cpp
*/

#pragma once

#include "debug_screen.h"

#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <regex>

#include <cstdio>
#include <typeinfo>

#define FG_DEFAULT 39
#define BG_DEFAULT 49
#define TEXT_DEFAULT 0
#define VT100_ESC "\x1b"

#define EntryType_UNKNOWN 0
#define EntryType_MENU 1
#define EntryType_DROP_DOWN 2
#define EntryType_CHECKBOX_CHECKED 3
#define EntryType_CHECKBOX_UNCHECKED 4
#define EntryType_INPUT_BOX 5
#define EntryType_SELECTABLE_TXT 6
#define EntryType_DISABLE_TXT 7
#define EntryType_SUBTITLE 8

struct ScreenStruct
{
    int heigh;
    int width;
    char data[31][100][4][25];
    ScreenStruct();
};

struct WholePage
{
    int heigh;
    int width;
    char data[31][100];
    WholePage();
    void Clear();
};

struct SelectPage
{
    char entries[31][3][500];
    int entries_count;
    char titles[1000];
    char description[1000];
    int highlight_idx;

    bool is_scrollable_up;
    bool is_scrollable_down;
    bool is_dialog_box;
    bool is_popup;

    SelectPage();

    void Print();
    void Clear();
};

string ParseWithoutEsc(string byte_input, vector<Vt100Cmd> &events);

struct ScreenCell
{
    char content_;
    int fg_color_;
    int bg_color_;
    int text_atr_;

    ScreenCell();
    ScreenCell(char content, int fg_color, int bg_color, int text_atr);
    void print();
};

struct ScreenItem
{
    std::string content_;
    int beg_;
    int end_;
    int fg_color_;
    int bg_color_;
    int text_atr_;

    ScreenItem();
    ScreenItem(std::string content, int beg, int end, int fg_color, int bg_color, int text_atr);
    void print();
};

class Vt100ScreenParser
{
private:
    std::vector<std::vector<ScreenItem>> screen_info_;
    DebugScreen debug_screen_;

    std::vector<Vt100Cmd> buff_;
    std::vector<std::vector<ScreenCell>> char_matrix_;

    int cur_fg_;
    int cur_bg_;
    int cur_text_attribute_;

    int width_;
    int height_;

    std::string platform_;

    int highlight_fg_;
    int highlight_bg_;
    int highlight_popup_fg_;
    int highlight_popup_bg_;
    int page_fg_;
    int page_bg_;

    int selectable_fg_; //
    int selectable_bg_;
    int disable_fg_;
    int disable_bg_;
    int disable_text_;

    int default_bg_;
    int home_footer_bg_;

    int subtitle_fg_;
    int subtitle_bg_;

    int header_beg_;
    int header_end_;

    int value_beg_;
    int des_beg_;

    int footer_beg_;
    int footer_end_;

    char scroll_down_char_;
    char scroll_up_char_;

    int workspace_beg_;
    int workspace_end_;

    std::regex *header_regex_top_ = NULL;
    std::regex *header_regex_mid_ = NULL;
    std::regex *header_regex_bottom_ = NULL;
    std::regex *footer_regex_top_ = NULL;
    std::regex *footer_regex_mid_ = NULL;
    std::regex *footer_regex_bottom_ = NULL;
    std::regex *popup_regex_top_ = NULL;
    std::regex *popup_regex_mid_ = NULL;
    std::regex *popup_regex_bottom_ = NULL;

    std::unordered_map<int, std::string> FG;
    std::unordered_map<int, std::string> BG;
    std::unordered_map<int, std::string> TEXT;
    std::unordered_map<std::string, int> FG_INV;
    std::unordered_map<std::string, int> BG_INV;
    std::unordered_map<std::string, int> TEXT_INV;

    const std::unordered_map<int, std::string> FG_ANSI = {
        {30, "black"},
        {31, "red"},
        {32, "green"},
        {33, "brown"},
        {34, "blue"},
        {35, "magenta"},
        {36, "cyan"},
        {37, "white"},
        {39, "default"} // white.
    };

    const std::unordered_map<int, std::string> BG_ANSI = {
        {40, "black"},
        {41, "red"},
        {42, "green"},
        {43, "brown"},
        {44, "blue"},
        {45, "magenta"},
        {46, "cyan"},
        {47, "white"},
        {49, "default"} // black.
    };

    const std::unordered_map<int, std::string> TEXT_ANSI = {
        {0, "default"},
        {1, "+bold"},
        {3, "+italics"},
        {4, "+underscore"},
        {7, "+reverse"},
        {9, "+strikethrough"},
        {22, "-bold"},
        {23, "-italics"},
        {24, "-underscore"},
        {27, "-reverse"},
        {29, "-strikethrough"},
    };

    struct Entry
    {
        std::string key;
        std::string value;
        int type;
    };

    struct Page
    {
        vector<Entry> entries;
        std::string titles;
        std::string description;
        int highlight_idx;
        bool is_scrollable_up;
        bool is_scrollable_down;
        bool is_dialog_box;
        bool is_popup;
    };

    void InitPlatformConfig();
    void InitScreenInfo();
    void InitCharMatrix();
    std::string CharReplace(std::string str);
    void ParseScreen();
    void InsertScreenInfo(int row, ScreenItem item);
    void MergeScreenInfo();

    string GetRowContent(int row_no);
    bool CheckRowFgBgText(int idx, int fg = -1, int bg = -1, int text = -1, int beg = 0, int end = -1);
    bool CheckExistRowFgBgText(int idx, int fg = -1, int bg = -1, int text = -1, int beg = 0, int end = -1, bool non_whitespace = true);
    bool CheckExistDisable(int idx, int beg = 0, int end = -1, bool non_whitespace = true);
    bool CheckExistSubtitle(int idx, int beg = 0, int end = -1, bool non_whitespace = true);
    bool CheckExistHighlight(int idx, int beg = 0, int end = -1, bool non_whitespace = true);
    bool CheckExistHighlightPopup(int idx, int beg = 0, int end = -1, bool non_whitespace = true);
    bool IsScrollableUp();
    bool IsScrollableDown();
    void InitWorkspace();
    void InitFooter();
    void InitHeader(Vt100ScreenParser::Page &page_ret);

    int String2Num(std::string str, int widthOrheight);
    Page get_whole_page_info(bool selectable_only, bool kv_sep);
    void InitPageDict(Vt100ScreenParser::Page &page);
    bool check_screen_available();
    void GetCompleteHighlightKeyThruRows(string *key, string *desc, int *idx);
    void GetCompleteKvThruRows(string *key, string *value, string *desc, int *idx, bool is_disable = false);

    bool popup_parse(Vt100ScreenParser::Page &page);
    void non_popup_parse(Vt100ScreenParser::Page &page, bool selectable_only = true, bool kv_sep = false);

public:
    Vt100ScreenParser(std::string platform);
    void CleanScreenData();
    void Feed(std::string input);
    ScreenStruct *GetScreenColored();
    int GetWidth();
    int GetHeight();
    vector<string> GetWholePage();
    SelectPage *GetSelectablePage();
    char *GetValueByKey(std::string key);
};
