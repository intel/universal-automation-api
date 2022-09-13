/*
File Name : vt100_screen_parse.cpp
Description : This file is designed to analyze and draw the bios screen.
Author : S3E-FIA-FAI
*/

#ifdef __linux__
#define DLLEXPORT extern "C"
#else
#define DLLEXPORT extern "C" __declspec(dllexport)
#endif

#include <iostream>
#include <sstream>
#include <vector>

#include "vt100_screen_parse.h"
#include "debug_screen.h"

using namespace std;

SelectPage select_page;
ScreenStruct screen;
Vt100ScreenParser *vt100_screen_parser = NULL;
WholePage whole_page;
char temp_value[1000];
char *edk_shell_string = NULL;

ScreenItem::ScreenItem()
{
    content_ = "";
    beg_ = -1;
    end_ = -1;
    fg_color_ = FG_DEFAULT;
    bg_color_ = BG_DEFAULT;
    text_atr_ = TEXT_DEFAULT;
}

ScreenItem::ScreenItem(std::string content, int beg, int end, int fg_color, int bg_color, int text_atr)
{
    content_ = content;
    beg_ = beg;
    end_ = end;
    fg_color_ = fg_color;
    bg_color_ = bg_color;
    text_atr_ = text_atr;
}

void ScreenItem::print()
{
    cout << "({" << content_ << "},{" << beg_ << "},{" << end_ << "},{" << fg_color_ << "},{" << bg_color_ << "},{" << text_atr_ << "})" << endl;
}

ScreenCell::ScreenCell()
{
    content_ = ' ';
    fg_color_ = FG_DEFAULT;
    bg_color_ = BG_DEFAULT;
    text_atr_ = TEXT_DEFAULT;
}

ScreenCell::ScreenCell(char content, int fg_color, int bg_color, int text_atr)
{
    content_ = content;
    fg_color_ = fg_color;
    bg_color_ = bg_color;
    text_atr_ = text_atr;
}

void ScreenCell::print()
{
    cout << "({" << content_ << "},{" << fg_color_ << "},{" << bg_color_ << "},{" << text_atr_ << "})" << endl;
}

int Vt100ScreenParser::String2Num(std::string str, int widthOrheight)
{
    // if width == 100, ":0" represents 100, ":1" represents 99
    if (str[0] == ':')
        return widthOrheight - atoi(str.substr(1, str.length() - 1).c_str());
    else
        return atoi(str.c_str());
}

int Vt100ScreenParser::GetWidth()
{
    /*
       Function Name       : GetWidth()
       Parameters          :
       Functionality       : return the width of screen
       Function Invoked    : None
       Return Value        : int: the width of screen
   */
    return width_;
}

int Vt100ScreenParser::GetHeight()
{
    /*
       Function Name       : GetHeight()
       Parameters          :
       Functionality       : return the height of screen
       Function Invoked    : None
       Return Value        : int: the height of screen
   */
    return height_;
}

string Vt100ScreenParser::GetRowContent(int row_no)
{
    /*
     get whole row content text
    */
    string content = "";
    vector<ScreenItem> row = screen_info_[row_no];
    for (auto it = row.begin(); it != row.end(); it++)
    {
        ScreenItem screen_item = *it;
        content += screen_item.content_;
    }
    return content;
}

vector<string> Vt100ScreenParser::GetWholePage()
{
    /*
    Function Name       : GetWholePage()
        Parameters          : None
        Functionality       : parse serial input,
                              return whole screen in vector(str)
        Function Invoked    :
        Return Value        :
                              [
                                "/---------------------------------\" # line 0
                                "|     Config TDP Configurations   |" # line 1
                                "\---------------------------------/" # line 2
                              ]
    */
    vector<string> page;
    for (int i = 0; i < height_; i++)
    {
        string content = GetRowContent(i);
        page.push_back(content);
    }
    return page;
}

bool Vt100ScreenParser::CheckRowFgBgText(int idx, int fg, int bg, int text, int beg, int end)
{
    /*
        Check if row[beg:end] is all in fg, bg color, text attribute.
        If one char not in the given format, return False
        Only check the given parameter
    */
    if (end == -1)
        end = width_;
    vector<ScreenItem> screen_info_idx = screen_info_[idx];
    if (screen_info_idx.size() == 0)
        return false;
    for (auto it = screen_info_idx.begin(); it != screen_info_idx.end(); it++)
    {
        ScreenItem screen_item = *it;
        if (screen_item.beg_ < end && screen_item.end_ > beg)
        {
            if (fg != -1 && screen_item.fg_color_ != fg)
                return false;
            else if (bg != -1 && screen_item.bg_color_ != bg)
                return false;
            else if (text != -1 && screen_item.text_atr_ != text)
                return false;
        }
    }
    return true;
}

bool Vt100ScreenParser::CheckExistRowFgBgText(int idx, int fg, int bg, int text, int beg, int end, bool non_whitespace)
{
    /*
        Check if row[beg:end] exist at least one char in fg, bg color
        and text attribute.
        Only check the given parameter.
        If non_whitespace set, check visible char in those format.
    */
    if (end == -1)
        end = width_;
    vector<ScreenItem> screen_item_info = screen_info_[idx];
    for (auto it = screen_item_info.begin(); it != screen_item_info.end(); it++)
    {
        ScreenItem screen_item = *it;
        if (screen_item.beg_ < end && screen_item.end_ > beg)
        {
            bool fg_flag = true;
            bool bg_flag = true;
            bool text_flag = true;
            string non_whitespace_flag = "true";
            if (fg != -1 && screen_item.fg_color_ != fg)
                fg_flag = false;
            if (bg != -1 && screen_item.bg_color_ != bg)
                bg_flag = false;
            if (text != -1 && screen_item.text_atr_ != text)
                text_flag = false;
            if (non_whitespace == true)
            {
                int max_beg = max(0, beg - screen_item.beg_);
                int min_end = min(end, screen_item.end_) - screen_item.beg_;
                string content_no_strip = screen_item.content_.substr(max_beg, min_end - max_beg);
                if (content_no_strip.empty())
                    non_whitespace_flag = content_no_strip;
                else
                {
                    content_no_strip.erase(0, content_no_strip.find_first_not_of(" "));
                    content_no_strip.erase(content_no_strip.find_last_not_of(" ") + 1);
                    non_whitespace_flag = content_no_strip;
                }
            }
            if (fg_flag == true && bg_flag == true && text_flag == true && non_whitespace_flag != "")
                return true;
        }
    }
    return false;
}

bool Vt100ScreenParser::CheckExistDisable(int idx, int beg, int end, bool non_whitespace)
{
    bool check_exist_disable = CheckExistRowFgBgText(idx, disable_fg_, disable_bg_, disable_text_, beg, end, non_whitespace);
    return check_exist_disable;
}

bool Vt100ScreenParser::CheckExistSubtitle(int idx, int beg, int end, bool non_whitespace)
{
    bool check_exist_subtitle = CheckExistRowFgBgText(idx, subtitle_fg_, subtitle_bg_, -1, beg, end, non_whitespace);
    return check_exist_subtitle;
}

bool Vt100ScreenParser::CheckExistHighlight(int idx, int beg, int end, bool non_whitespace)
{
    bool check_exist_highlight = CheckExistRowFgBgText(idx, highlight_fg_, highlight_bg_, -1, beg, end, non_whitespace);
    return check_exist_highlight;
}

bool Vt100ScreenParser::CheckExistHighlightPopup(int idx, int beg, int end, bool non_whitespace)
{
    bool check_exist_highlight_popup = CheckExistRowFgBgText(idx, highlight_popup_fg_, highlight_popup_bg_, -1, beg, end, true);
    return check_exist_highlight_popup;
}

bool Vt100ScreenParser::IsScrollableUp()
{
    // check if ^ exist after header, and in color page_fg, page_bg
    int row_no = workspace_beg_;
    string content = GetRowContent(row_no);
    string::size_type idx = content.find(scroll_up_char_);
    // find scroll_up_char_
    if (idx != string::npos)
    {
        bool check_row_fg_bg_text = CheckRowFgBgText(row_no, page_fg_, page_bg_, -1, idx, idx + 1);
        return check_row_fg_bg_text;
    }
    return false;
}

bool Vt100ScreenParser::IsScrollableDown()
{
    int row_no = workspace_end_ - 1;
    string content = GetRowContent(row_no);
    string::size_type idx = content.find(scroll_down_char_);
    if (idx != string::npos)
    {
        bool check_row_fg_bg_text = CheckRowFgBgText(row_no, page_fg_, page_bg_, -1, idx, idx + 1);
        return check_row_fg_bg_text;
    }
    return false;
}

void Vt100ScreenParser::InitWorkspace()
{
    workspace_beg_ = header_end_;
    workspace_end_ = footer_beg_;
    if (workspace_beg_ >= workspace_end_)
        cout << "no workspace" << endl;
}

void Vt100ScreenParser::InitFooter()
{
    /*
        set self._footer_beg and self._footer_end,
        indicate the begin row number and end row number of footer
        first match /-----------------\
        if not matched, match black bg
    */
    bool bottom_set = false;
    for (int idx = height_ - 1; idx >= 0; idx--)
    {
        string content = GetRowContent(idx);
        if (bottom_set == false)
        {
            if (idx < footer_beg_)
            {
                cout << "footer bottom not matched, change scheme to match color." << endl;
                break;
            }
            bool match = regex_match(content, *footer_regex_bottom_);
            if (match == true)
            {
                footer_end_ = idx + 1;
                bottom_set = true;
            }
        }
        else
        {
            if (idx < footer_end_ - 5)
            {
                cout << "footer begin not found in 5 lines before footer end, use default." << endl;
                break;
            }
            bool match = regex_match(content, *footer_regex_top_);
            if (match == true)
            {
                footer_beg_ = idx;
                break;
            }
        }
    }
    if (bottom_set == false)
    {
        for (int idx = height_ - 1; idx >= 0; idx--)
        {
            if (bottom_set == false)
            {
                if (idx < footer_beg_)
                {
                    cout << "footer not found, use default config" << endl;
                    break;
                }
                bool check_row_fg_bg_text = CheckRowFgBgText(idx, -1, home_footer_bg_, -1, 0, -1);
                if (check_row_fg_bg_text == true)
                {
                    bottom_set = true;
                    footer_end_ = idx + 1;
                }
            }
            else
            {
                if (idx < footer_end_ - 7)
                {
                    cout << "footer begin not found in 7 lines begfore footer end, use default" << endl;
                    break;
                }
                bool check_row_fg_bg_text = CheckRowFgBgText(idx, -1, home_footer_bg_, -1, 1, width_ - 1);
                if (check_row_fg_bg_text == true)
                    continue;
                else
                {
                    footer_beg_ = idx;
                    break;
                }
            }
        }
    }
}

void Vt100ScreenParser::InitHeader(Vt100ScreenParser::Page &page_ret)
{
    /*
        Set self.header_beg, self.header_end, return title.
        If no header return None.
        Match /--------\ for header_beg in the first 6 rows,
        if not matched set to default [0, 6).
        Then match \--------/ for header_end at row header_beg + 2.
    */
    string title = "";
    int search_beg = header_beg_;
    int search_end = header_end_;
    // header may not start from the first row
    for (int i = search_beg; i < search_end; i++)
    {
        string top = GetRowContent(i);
        bool top_match = regex_search(top, *header_regex_top_);
        if (top_match == true)
        {
            header_beg_ = i;
            string mid = GetRowContent(i + 1);
            smatch mid_match_result;
            bool mid_match = regex_search(mid, mid_match_result, *header_regex_mid_);
            if (mid_match == true)
            {
                title = mid_match_result[0];
                // strip
                title.erase(0, title.find_first_not_of(" "));
                title.erase(title.find_last_not_of(" ") + 1);
            }
            else
                cout << "no title after header top, leave it None" << endl;
            string bottom = GetRowContent(i + 2);
            bool bottom_match = regex_search(bottom, *header_regex_bottom_);
            if (bottom_match == true)
            {
                header_end_ = i + 3;
                break;
            }
            else
                cout << "header has no bottom, use default" << endl;
        }
    }
    page_ret.titles = title;
    cout << "page title is " << title << endl;
}

string ParseWithoutEsc(string byte_input, vector<Vt100Cmd> &events)
{
    /*
        If an input not start with \x1b take it as draw string cmd
        append to events, return the left bytes start with \x1b
    */
    int idx = byte_input.find(VT100_ESC);
    if (idx > 0)
    {
        vector<string> draw_content = {byte_input.substr(0, idx)};
        events.push_back(Vt100Cmd("draw", draw_content));
        string content_after_idx = byte_input.substr(idx, byte_input.size() - idx);
        return content_after_idx;
    }
    else if (idx == 0)
        return byte_input;
    else
    {
        vector<string> draw_content = {byte_input};
        events.push_back(Vt100Cmd("draw", draw_content));
        return "";
    }
}

Vt100ScreenParser::Vt100ScreenParser(std::string platform)
{
    if (platform != "client" && platform != "server")
    {
        std::cout << "Error: Vt100ScreenPaser platform must be client or server!\n";
        platform = "client";
    }
    // begin_ = -1;
    // row_ = -1;
    platform_ = platform;
    cur_fg_ = FG_DEFAULT;
    cur_bg_ = BG_DEFAULT;
    cur_text_attribute_ = TEXT_DEFAULT;
    buff_.clear();
    FG = FG_ANSI;
    BG = BG_ANSI;
    TEXT = TEXT_ANSI;

    FG_INV.clear();
    for (auto it = FG.begin(); it != FG.end(); it++)
        FG_INV.insert(make_pair(it->second, it->first));

    BG_INV.clear();
    for (auto it = BG.begin(); it != BG.end(); it++)
        BG_INV.insert(make_pair(it->second, it->first));

    TEXT_INV.clear();
    for (auto it = TEXT.begin(); it != TEXT.end(); it++)
        TEXT_INV.insert(make_pair(it->second, it->first));

    InitPlatformConfig();
    InitCharMatrix();
    InitScreenInfo();
}

void Vt100ScreenParser::InitPlatformConfig()
{
    if (platform_ == "client")
    {
        width_ = 100;
        height_ = 31;

        highlight_fg_ = FG_INV["white"];
        highlight_bg_ = BG_INV["black"];

        highlight_popup_fg_ = FG_INV["white"];
        highlight_popup_bg_ = BG_INV["cyan"];

        page_fg_ = FG_INV["red"];
        page_bg_ = BG_INV["white"];
        selectable_fg_ = FG_INV["blue"];
        selectable_bg_ = BG_INV["white"];
        disable_fg_ = FG_INV["black"];
        disable_bg_ = BG_INV["white"];
        disable_text_ = TEXT_INV["+bold"];

        default_bg_ = BG_INV["white"];
        home_footer_bg_ = BG_INV["black"];
        subtitle_fg_ = FG_INV["black"];
        subtitle_bg_ = BG_INV["white"];

        header_beg_ = 0;
        header_end_ = 6;

        value_beg_ = 36;
        des_beg_ = 70;

        footer_beg_ = height_ - 5;
        footer_end_ = height_;

        scroll_down_char_ = 'v';
        scroll_up_char_ = '^';

        header_regex_top_ = new std::regex("-{10}");
        header_regex_mid_ = new std::regex(" +([\\S ]+[\\S]) +");
        header_regex_bottom_ = new std::regex("-{10}");
        footer_regex_top_ = new std::regex("^\\/-*\\\\$");
        footer_regex_mid_ = new std::regex("^\\| *([\\S ]+[\\S]) *\\|$");
        footer_regex_bottom_ = new std::regex("^\\\\-*\\/$");
        popup_regex_top_ = new std::regex("\\/-+(\\^?)-+\\\\");
        popup_regex_mid_ = new std::regex("\\|(.*)\\|");
        popup_regex_bottom_ = new std::regex("\\\\-+(v?)-+\\/");
    }
    else
    {
        width_ = 80;
        height_ = 25;

        highlight_fg_ = FG_INV["white"];
        highlight_bg_ = BG_INV["black"];

        highlight_popup_fg_ = FG_INV["white"];
        highlight_popup_bg_ = BG_INV["cyan"];

        page_fg_ = FG_INV["red"];
        page_bg_ = BG_INV["white"];

        selectable_fg_ = FG_INV["black"]; //
        selectable_bg_ = BG_INV["white"];
        disable_fg_ = FG_INV["black"];
        disable_bg_ = BG_INV["white"];
        disable_text_ = TEXT_INV["+bold"];

        default_bg_ = BG_INV["white"];
        home_footer_bg_ = BG_INV["black"];
        subtitle_fg_ = FG_INV["blue"];
        subtitle_bg_ = BG_INV["white"];

        header_beg_ = 0;
        header_end_ = 6;

        value_beg_ = 30;
        des_beg_ = 57;

        footer_beg_ = height_ - 5;
        footer_end_ = height_;

        scroll_down_char_ = 'v';
        scroll_up_char_ = '^';

        header_regex_top_ = new std::regex("^\\/-*\\\\$");
        header_regex_mid_ = new std::regex("^\\| *([\\S ]+[\\S]) *\\|$");
        header_regex_bottom_ = new std::regex("^\\\\-*\\/$");
        footer_regex_top_ = new std::regex("^\\/-*\\\\$");
        footer_regex_mid_ = new std::regex("^\\| *([\\S ]+[\\S]) *\\|$");
        footer_regex_bottom_ = new std::regex("^\\\\-+(.*)-+\\/$");
        popup_regex_top_ = new std::regex("\\/-+(\\^?)-+\\\\");
        popup_regex_mid_ = new std::regex("\\|(.*)\\|");
        popup_regex_bottom_ = new std::regex("\\\\-+(v?)-+\\/");
    }
}

void Vt100ScreenParser::InitCharMatrix()
{
    char_matrix_.clear();
    char_matrix_.resize(height_);
    for (int i = 0; i < height_; i++)
    {
        char_matrix_[i].clear();
        char_matrix_[i].resize(width_);
    }
}

void Vt100ScreenParser::InitScreenInfo()
{
    screen_info_.clear();
    screen_info_.resize(height_);
}

void Vt100ScreenParser::CleanScreenData()
{
    InitCharMatrix();
    InitScreenInfo();
}

std::string Vt100ScreenParser::CharReplace(std::string str)
{
    // replace unreadable character to readale VT100 terminal format
    std::unordered_map<char, char> replace_map = {
        {'\xbf', '\\'},
        {'\xd9', '/'},
        {'\xc0', '\\'},
        {'\xb3', '|'},
        {'\xc4', '-'},
        {'\xda', '/'},
        {'\x10', '>'},
        {'\x18', '^'},
        {'\x19', 'v'}};
    int len = str.length();
    for (int i = 0; i < len; i++)
    {
        if (replace_map.count(str[i]))
        {
            str[i] = replace_map[str[i]];
        }
    }
    return str;
}

void Vt100ScreenParser::Feed(std::string input)
{
    input = CharReplace(input);
    std::vector<Vt100Cmd> vt100cmds = debug_screen_.SerialOutputSplit(input);
    buff_.insert(buff_.end(), vt100cmds.begin(), vt100cmds.end());
    if (!buff_.empty())
    {
        ParseScreen();
    }
}

void Vt100ScreenParser::ParseScreen()
{
    int beg = -1;
    int row = -1;
    for (auto event = buff_.begin(); event != buff_.end(); event++)
    {
        std::string e_name_s = event->description_;
        std::vector<std::string> e_args_l = event->params_;

        if (e_name_s == "display_attributes")
        {
            if (e_args_l.size() != 1)
            {
                cout << "select_graphic_rendition param num not 1" << endl;
                continue;
            }
            int cur = atoi(e_args_l[0].c_str());
            if (cur == 0)
            {
                cur_fg_ = FG_DEFAULT;
                cur_bg_ = BG_DEFAULT;
                cur_text_attribute_ = TEXT_DEFAULT;
            }
            else if (FG.count(cur))
            {
                cur_fg_ = cur;
            }
            else if (BG.count(cur))
            {
                cur_bg_ = cur;
            }
            else if (TEXT.count(cur))
            {
                cur_text_attribute_ = cur;
            }
            else
            {
                cout << "display attribute not found: " << cur << endl;
            }
        }

        else if (e_name_s == "cursor_position_start")
        {
            if (e_args_l.size() != 2)
            {
                cout << "cursor_position_start param num not 2" << endl;
                continue;
            }
            row = String2Num(e_args_l[0], height_) - 1; // convert start from 1 to 0
            beg = String2Num(e_args_l[1], width_) - 1;
        }

        else if (e_name_s == "draw")
        {
            if (e_args_l.size() != 1)
            {
                cout << "draw param number error" << endl;
                continue;
            }
            if (beg == -1 || row == -1)
            {
                continue;
            }
            std::string content = "";
            int len = e_args_l[0].length();
            for (int i = 0; i < len; i++)
            {
                if (e_args_l[0][i] != '\n')
                {
                    content.push_back(e_args_l[0][i]);
                }
            }

            if (content.empty())
            {
                continue;
            }
            if (content.find("EC Command") != -1)
            {
                content = strsplit(content, "EC Command")[0];
            }
            ScreenItem item(content, beg, beg + content.length(), cur_fg_, cur_bg_, cur_text_attribute_);
            InsertScreenInfo(row, item);
            beg += content.length();
        }

        else
        {
            cout << "function " << e_name_s << " not support" << endl;
            continue;
        }
    }
    buff_.clear();
    MergeScreenInfo();
}

void Vt100ScreenParser::InsertScreenInfo(int row, ScreenItem new_item)
{
    if (row >= height_)
    {
        cout << "row number : " << row + 1 << " > screen height : " << height_ << ", please check configuration" << endl;
        return;
    }
    for (size_t i = 0; i < new_item.content_.length() && (i + new_item.beg_ < width_); i++)
    {
        char_matrix_[row][i + new_item.beg_].content_ = (char)new_item.content_[i];
        char_matrix_[row][i + new_item.beg_].fg_color_ = new_item.fg_color_;
        char_matrix_[row][i + new_item.beg_].bg_color_ = new_item.bg_color_;
        char_matrix_[row][i + new_item.beg_].text_atr_ = new_item.text_atr_;
    }
}

void Vt100ScreenParser::MergeScreenInfo()
{
    /*
    *  """
        merge the _char_matrix to _screen_info, for following process
        """
    */

    for (int i = 0; i < height_; i++)
    {
        screen_info_[i].clear();
        ScreenItem item0(std::string(1, char_matrix_[i][0].content_),
                         0,
                         1,
                         char_matrix_[i][0].fg_color_,
                         char_matrix_[i][0].bg_color_,
                         char_matrix_[i][0].text_atr_);

        for (int j = 1; j < width_; j++)
        {
            if (item0.fg_color_ == char_matrix_[i][j].fg_color_ &&
                item0.bg_color_ == char_matrix_[i][j].bg_color_ &&
                item0.text_atr_ == char_matrix_[i][j].text_atr_)
            {
                item0.content_.push_back(char_matrix_[i][j].content_);
                item0.end_ = j + 1;
            }
            else
            {
                screen_info_[i].push_back(item0);
                ScreenItem new_item0(std::string(1, char_matrix_[i][j].content_),
                                     j,
                                     j + 1,
                                     char_matrix_[i][j].fg_color_,
                                     char_matrix_[i][j].bg_color_,
                                     char_matrix_[i][j].text_atr_);
                item0 = new_item0;
            }
        }
        screen_info_[i].push_back(item0);
    }
}

ScreenStruct *Vt100ScreenParser::GetScreenColored()
{
    for (int i = 0; i < height_; i++)
    {
        for (int j = 0; j < width_; j++)
        {
            if (!FG.count(char_matrix_[i][j].fg_color_) ||
                !BG.count(char_matrix_[i][j].bg_color_) ||
                !TEXT.count(char_matrix_[i][j].text_atr_))
            {
                return NULL;
            }

            screen.data[i][j][0][0] = char_matrix_[i][j].content_;

            Strcpy(screen.data[i][j][1], FG[char_matrix_[i][j].fg_color_], sizeof(screen.data[i][j][1]) / sizeof(char));
            Strcpy(screen.data[i][j][2], BG[char_matrix_[i][j].bg_color_], sizeof(screen.data[i][j][2]) / sizeof(char));
            Strcpy(screen.data[i][j][3], TEXT[char_matrix_[i][j].text_atr_], sizeof(screen.data[i][j][3]) / sizeof(char));
        }
    }
    screen.width = width_;
    screen.heigh = height_;
    return &screen;
}

void SelectPage::Print()
{
    cout << "----------SelectPage-----------" << endl;
    cout << "titles: " << titles << endl;
    cout << "description: " << description << endl;
    cout << "entries_count: " << entries_count << endl;
    for (int i = 0; i < entries_count; i++)
        cout << "entries " << i << "th: " << entries[i][0] << " " << entries[i][1] << " " << entries[i][2] << endl;
    cout << "highlight_idx: " << highlight_idx << endl;

    cout << "is_scrollable_up: " << is_scrollable_up << endl;
    cout << "is_scrollable_down: " << is_scrollable_down << endl;
    cout << "is_dialog_box: " << is_dialog_box << endl;
    cout << "is_popup: " << is_popup << endl;
}

SelectPage *Vt100ScreenParser::GetSelectablePage()
{
    Page page = get_whole_page_info(true, true);
    // cout << "completed get wholepage" << endl;
    select_page.highlight_idx = page.highlight_idx;
    select_page.is_dialog_box = page.is_dialog_box;
    select_page.is_popup = page.is_popup;
    select_page.is_scrollable_down = page.is_scrollable_down;
    select_page.is_scrollable_up = page.is_scrollable_up;
    Strcpy(select_page.description, page.description, sizeof(select_page.description) / sizeof(char));
    Strcpy(select_page.titles, page.titles, sizeof(select_page.titles) / sizeof(char));
    select_page.entries_count = min(page.entries.size(), sizeof(select_page.entries) / sizeof(select_page.entries[0]));

    for (int i = 0; i < select_page.entries_count; i++)
    {
        Strcpy(select_page.entries[i][0], page.entries[i].key, sizeof(select_page.entries[i][0]) / sizeof(char));
        Strcpy(select_page.entries[i][1], page.entries[i].value, sizeof(select_page.entries[i][1]) / sizeof(char));
        Strcpy(select_page.entries[i][2], std::to_string(page.entries[i].type), sizeof(select_page.entries[i][2]) / sizeof(char));
    }
    // cout << "return getselect page" <<endl;
    // select_page.Print();
    return &select_page;
}

Vt100ScreenParser::Page Vt100ScreenParser::get_whole_page_info(bool selectable_only, bool kv_sep)
{
    Page page_ret;
    InitPageDict(page_ret);
    if (!check_screen_available())
    {
        cout << "no screen data, return empty dict" << endl;
        return page_ret;
    }
    InitHeader(page_ret);
    InitFooter();
    InitWorkspace();
    if (!popup_parse(page_ret))
    {
        non_popup_parse(page_ret, selectable_only, kv_sep);
    }

    return page_ret;
}

void Vt100ScreenParser::InitPageDict(Vt100ScreenParser::Page &page)
{
    page.entries.clear();
    page.titles = "";
    page.description = "";
    page.highlight_idx = -1;
    page.is_scrollable_up = false;
    page.is_scrollable_down = false;
    page.is_dialog_box = false;
    page.is_popup = false;
}

bool Vt100ScreenParser::check_screen_available()
{
    for (auto it = screen_info_.begin(); it != screen_info_.end(); it++)
    {
        if (it->empty())
            return false;
    }
    return true;
}

std::string strip(std::string str)
{
    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

std::string rstrip(std::string str)
{
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

std::string toupper(std::string str)
{
    int len = str.length();
    for (int i = 0; i < len; i++)
    {
        str[i] = toupper(str[i]);
    }
    return str;
}

void Vt100ScreenParser::GetCompleteKvThruRows(string *key, string *value, string *desc, int *idx, bool is_disable)
{
    string content = GetRowContent(*idx);
    *key = strip(content.substr(0, value_beg_));
    *value = strip(content.substr(value_beg_, des_beg_ - value_beg_));
    *desc = strip(content.substr(des_beg_, content.length() - des_beg_));
    string existence = "";
    int free_space_for_key = value_beg_ - rstrip(content.substr(0, value_beg_)).length() - 3;
    free_space_for_key = free_space_for_key >= 0 ? free_space_for_key : 0;
    int free_space_for_val = des_beg_ - value_beg_ - (*value).length() - 3;
    free_space_for_val = free_space_for_val >= 0 ? free_space_for_val : 0;
    (*idx) += 1;

    while ((*idx) < workspace_end_ - 1)
    {
        content = GetRowContent(*idx);
        string _key = strip(content.substr(0, value_beg_));
        string _value = strip(content.substr(value_beg_, des_beg_ - value_beg_));
        string _desc = strip(content.substr(des_beg_, content.length() - des_beg_));
        // menu is not supposed to take 2 row, for now
        if (_key.length() >= 2 && _key.substr(0, 2) == "> ")
            break;
        // is_disable is false, break at disable text or subtitle
        else if (!is_disable &&
                 (CheckExistDisable(*idx, 0, des_beg_) ||
                  CheckExistSubtitle(*idx, 0, value_beg_)))
            break;
        // is_disable is true, break at selectable text
        else if (is_disable && !CheckExistDisable(*idx, 0, des_beg_))
            break;
        // no _keyand no _value
        else if (_key.empty() && value->empty())
            break;
        // has _key
        else if (!_key.empty())
        {
            if (existence != "" && existence.find("key") == -1)
            {
                cout << "independent value(" << _value << ") following independent key" << endl;
                break;
            }
            string first = strip(_key).find(" ") == -1 ? strip(_key) : strip(_key).substr(0, strip(_key).find(" "));
            if (first.length() > free_space_for_key)
            {
                free_space_for_key = value_beg_ - rstrip(content.substr(0, value_beg_)).length() - 3;
                free_space_for_key = free_space_for_key >= 0 ? free_space_for_key : 0;
            }
            else
                break;

            // has _value
            if (!_value.empty())
            {
                if (existence == "key")
                    break;
                string first = strip(_value).find(" ") == -1 ? strip(_value) : strip(_value).substr(0, strip(_value).find(" "));
                if ((((*value)[0] == '[') && ((*value).find(']') == -1)) || (((*value)[0] == '<') && ((*value).find('>') == -1)) || first.length() > free_space_for_val)
                {
                    free_space_for_val = des_beg_ - value_beg_ - _value.length() - 3;
                    free_space_for_val = free_space_for_val >= 0 ? free_space_for_val : 0;
                }
                else
                    break;

                existence = "key_value";
                (*value) += " " + _value;
            }
            // no _value
            else
            {
                existence = "key";
            }
            (*key) += " " + _key;
        }
        else
        {
            // no _key, but has _value
            if (existence.empty() || (existence.find("value") != -1))
                existence = "value";
            // independent key following independent value
            else
                break;
            (*value) += " " + _value;
        }

        // concatenate description
        if (!strip(_desc).empty())
        {
            (*desc) += " " + _desc;
        }
        (*idx) += 1;
    }
    (*idx) -= 1;
}

void Vt100ScreenParser::GetCompleteHighlightKeyThruRows(string *key, string *desc, int *idx)
{
    string content = GetRowContent(*idx);
    *key = content.substr(0, des_beg_);
    string cur_key = *key;
    *desc = content.substr(des_beg_, content.length() - des_beg_);
    (*idx) += 1;
    // skip the scroll down char
    while ((*idx) < workspace_end_ - 1)
    {
        if (CheckExistHighlight(*idx, 0, des_beg_, false))
        {
            string next_content = GetRowContent(*idx);
            string next_key = next_content.substr(0, des_beg_);
            if (rstrip(cur_key).length() == des_beg_)
            {
                *key = strip(*key) + strip(next_key);
            }
            else
            {
                *key = strip(*key) + " " + strip(next_key);
            }
            cur_key = next_key;
            (*idx) += 1;
        }
        else
        {
            break;
        }
    }
    (*idx) -= 1;
}

char *Vt100ScreenParser::GetValueByKey(std::string key)
{
    std::string values = "";
    Page page = get_whole_page_info(false, true);
    if (page.is_popup)
    {
        Strcpy(temp_value, values, 1000);
        return temp_value;
    }

    for (auto it = page.entries.begin(); it != page.entries.end(); it++)
    {
        std::string key2 = it->key;
        std::string value = it->value;
        if (toupper(strip(key2)).find(toupper(key)) != -1)
            values += (strip(value) + ";");
    }
    Strcpy(temp_value, values, 1000);
    return temp_value;
}

bool Vt100ScreenParser::popup_parse(Vt100ScreenParser::Page &page)
{
    int popup_beg = 0;
    int popup_end = 0;
    int col_beg = 0;
    int col_end = 0;
    vector<Entry> entries;

    for (int it = workspace_beg_; it < workspace_end_; it++)
    {
        std::string content = GetRowContent(it);
        std::smatch result;
        bool match = regex_search(content, result, *popup_regex_top_);
        if (match)
        {
            popup_beg = it;
            string::const_iterator beg_iter = result[0].first;
            for (col_beg = 0; beg_iter != content.begin(); beg_iter--, col_beg++)
                ;
            string::const_iterator end_iter = result[0].second;
            for (col_end = 0; end_iter != content.begin(); end_iter--, col_end++)
                ;
            std::string scroll = result[1];
            page.is_scrollable_up = ((scroll[0] == scroll_up_char_) ? true : false);
        }
    }

    if (popup_beg == 0)
        return false;

    for (int it = popup_beg + 1; it < workspace_end_; it++)
    {
        std::string content = GetRowContent(it);
        std::smatch result;
        bool mid_match = regex_search(content, result, *popup_regex_mid_);
        if (mid_match)
        {
            std::string txt = content.substr(col_beg + 1, col_end - 2 - col_beg);
            if (CheckExistHighlightPopup(it, col_beg, col_end))
                page.highlight_idx = entries.size();
            if ((CheckExistRowFgBgText(it, -1, highlight_bg_, -1, col_beg, col_end, false)) || (CheckExistRowFgBgText(it, -1, default_bg_, -1, col_beg, col_end, false)))
            {
                entries.push_back({txt, "", EntryType_INPUT_BOX});
            }
            else
            {
                entries.push_back({txt, "", EntryType_SELECTABLE_TXT});
            }
        }
        else
        {
            bool end_match = regex_search(content, result, *popup_regex_bottom_);
            if (end_match)
            {
                popup_end = it + 1;
                std::string scroll = result[1];
                page.is_scrollable_down = ((scroll[0] == scroll_down_char_) ? true : false);
                break;
            }
            else
            {
                cout << "popup menu mid break, ignore" << endl;
            }
        }
    }

    if ((!entries.empty()) && (page.highlight_idx == -1))
    {
        page.is_dialog_box = true;
        for (auto it = entries.begin(); it != entries.end(); it++)
        {
            if (it->type == EntryType_SELECTABLE_TXT)
            {
                it->type = EntryType_DISABLE_TXT;
            }
        }
    }

    if (popup_end == 0)
        cout << "popup begin, but end not found, ignore" << endl;
    else
        page.entries = entries;

    page.is_popup = true;
    return true;
}

void Vt100ScreenParser::non_popup_parse(Vt100ScreenParser::Page &page, bool selectable_only, bool kv_sep)
{
    page.is_scrollable_up = IsScrollableUp();
    page.is_scrollable_down = IsScrollableDown();
    vector<Entry> entries;
    int entry_beg = workspace_beg_ + 1;
    int entry_end = workspace_end_ - 1;
    int idx = entry_beg;
    int highlight_idx = -1;
    int highlight_first = 0;
    std::string desc_full = "";

    while (idx < entry_end)
    {
        int entry_type = EntryType_UNKNOWN;
        std::string content = GetRowContent(idx);
        std::string key = content.substr(0, value_beg_);
        std::string value = content.substr(value_beg_, des_beg_ - value_beg_);
        std::string desc = content.substr(des_beg_, content.size() - des_beg_);
        bool is_highlighted = false;
        if (CheckExistHighlight(idx, 0, des_beg_, false))
        {
            is_highlighted = true;
            if (highlight_idx == -1)
            {
                highlight_idx = entries.size();
                highlight_first = true;
            }
            else
                highlight_first = false;
        }
        std::string kv = key + value;

        if (CheckExistDisable(idx, 0, des_beg_, true))
        {
            // key, value, desc, idx = self._get_complete_kv_thru_rows(idx, is_disable=True)
            bool is_disable = true;
            GetCompleteKvThruRows(&key, &value, &desc, &idx, is_disable);
            entry_type = EntryType_DISABLE_TXT;
        }
        else if (CheckExistSubtitle(idx, 0, value_beg_, true))
        {
            entry_type = EntryType_SUBTITLE;
        }
        else if (strip(key).length() >= 2 && strip(key).substr(0, 2) == "> ")
        {
            entry_type = EntryType_MENU;
            key = kv;
            value = "";
            if (highlight_idx == entries.size())
            {
                // key, desc, idx = self._get_complete_highlight_key_thru_rows(idx)

                GetCompleteHighlightKeyThruRows(&key, &desc, &idx);
            }
        }
        else if (strip(value).length() >= 1 && strip(value).substr(0, 1) == "<")
        {
            // key, value, desc, idx = self._get_complete_kv_thru_rows(idx)
            bool is_disable = false;
            GetCompleteKvThruRows(&key, &value, &desc, &idx, is_disable);

            kv = key + value;
            entry_type = EntryType_DROP_DOWN;
        }
        else if (strip(value).length() >= 1 && strip(value).substr(0, 1) == "[")
        {
            // key, value, desc, idx = self._get_complete_kv_thru_rows(idx)
            bool is_disable = false;
            GetCompleteKvThruRows(&key, &value, &desc, &idx, is_disable);
            kv = key + value;
            if (strip(value) == "[ ]")
                entry_type = EntryType_CHECKBOX_UNCHECKED;
            else if (strip(value) == "[X]")
                entry_type = EntryType_CHECKBOX_CHECKED;
            else
                entry_type = EntryType_INPUT_BOX;
        }
        else if (CheckExistRowFgBgText(idx, selectable_fg_, selectable_bg_, -1, 0, value_beg_, true) || !strip(kv).empty())
        {
            if (!strip(key).empty() && (!strip(value).empty()))
            {
                if (key.substr(key.size() - 2, 2) == "  ")
                {
                    // key, value, desc, idx = self._get_complete_kv_thru_rows(idx)
                    bool is_disable = false;
                    GetCompleteKvThruRows(&key, &value, &desc, &idx, is_disable);
                }
                else
                {
                    key = kv;
                    value = "";
                    if (highlight_idx == entries.size())
                    {
                        // key, desc, idx = self._get_complete_highlight_key_thru_rows(idx)
                        GetCompleteHighlightKeyThruRows(&key, &desc, &idx);
                    }
                }
            }
            entry_type = EntryType_SELECTABLE_TXT;
        }
        if (entry_type != EntryType_UNKNOWN)
        {
            if ((entry_type >= 1 && entry_type <= 6) || !selectable_only)
            {
                if (kv_sep)
                {
                    if (is_highlighted)
                    {
                        if (highlight_first)
                            entries.push_back({strip(key), strip(value), entry_type});
                        else
                        {
                            Entry last_higlight_entry;
                            if (entries.size() > 0)
                            {
                                last_higlight_entry = *(entries.end() - 1);
                                *(entries.end() - 1) = {last_higlight_entry.key + strip(key), last_higlight_entry.value + strip(value), last_higlight_entry.type};
                            }
                        }
                    }
                    else
                        entries.push_back({strip(key), strip(value), entry_type});
                }
                else
                {
                    if (is_highlighted && !highlight_first)
                    {
                        Entry last_higlight_entry;
                        if (entries.size() > 0)
                        {
                            last_higlight_entry = *(entries.end() - 1);
                            *(entries.end() - 1) = {last_higlight_entry.key + strip(kv), last_higlight_entry.value, last_higlight_entry.type};
                        }
                    }
                    else
                        entries.push_back({strip(kv), "", entry_type});
                }
            }
        }
        if (!strip(desc).empty())
            desc_full += desc;
        idx += 1;
    }

    page.description = desc_full;
    page.entries = entries;
    page.highlight_idx = highlight_idx;
}

SelectPage::SelectPage()
{
    Clear();
}

void SelectPage::Clear()
{
    entries_count = 0;
    int len = sizeof(description) / sizeof(description[0]);
    for (int i = 0; i < len; i++)
        description[i] = '\0';
    len = sizeof(titles) / sizeof(titles[0]);
    for (int i = 0; i < len; i++)
        titles[i] = '\0';
    int len1 = sizeof(entries) / sizeof(entries[0]);
    int len2 = sizeof(entries[0]) / sizeof(entries[0][0]);
    int len3 = sizeof(entries[0][0]) / sizeof(entries[0][0][0]);
    for (int i = 0; i < len1; i++)
    {
        for (int j = 0; j < len2; j++)
        {
            for (int k = 0; k < len3; k++)
            {
                entries[i][j][k] = '\0';
            }
        }
    }

    highlight_idx = -1;
    is_scrollable_up = false;
    is_scrollable_down = false;
    is_dialog_box = false;
    is_popup = false;
}

DLLEXPORT void Init(char *platform)
{
    if (vt100_screen_parser != NULL)
    {
        delete vt100_screen_parser;
        vt100_screen_parser = NULL;
    }
    vt100_screen_parser = new Vt100ScreenParser(platform);
}

DLLEXPORT void CleanScreenData()
{
    if (vt100_screen_parser == NULL)
    {
        cout << "Error: Need init" << endl;
        return;
    }
    vt100_screen_parser->CleanScreenData();
}

DLLEXPORT void Feed(char *str1)
{
    if (vt100_screen_parser == NULL)
    {
        cout << "Error: Need init" << endl;
        return;
    }
    vt100_screen_parser->Feed(str1);
}

DLLEXPORT char *GetValueByKey(char *str1)
{
    if (vt100_screen_parser == NULL)
    {
        cout << "Error: Need init" << endl;
        return NULL;
    }
    return vt100_screen_parser->GetValueByKey(str1);
}

ScreenStruct::ScreenStruct()
{
    heigh = 0;
    width = 0;
    int len1 = sizeof(data) / sizeof(data[0]);
    int len2 = sizeof(data[0]) / sizeof(data[0][0]);
    int len3 = sizeof(data[0][0]) / sizeof(data[0][0][0]);
    int len4 = sizeof(data[0][0][0]) / sizeof(data[0][0][0][0]);
    for (int row = 0; row < len1; row++)
    {
        for (int col = 0; col < len2; col++)
        {
            for (int i = 0; i < len3; i++)
            {
                for (int j = 0; j < len4; j++)
                {
                    data[row][col][i][j] = '\0';
                }
            }
        }
    }
}

void WholePage::Clear()
{
    heigh = 0;
    width = 0;
    int len1 = sizeof(data) / sizeof(data[0]);
    int len2 = sizeof(data[0]) / sizeof(data[0][0]);
    for (int row = 0; row < len1; row++)
    {
        for (int col = 0; col < len2; col++)
        {
            data[row][col] = '\0';
        }
    }
}

WholePage::WholePage()
{
    Clear();
}

DLLEXPORT ScreenStruct GetScreenColored()
{
    if (vt100_screen_parser == NULL)
    {
        cout << "Error: Need init" << endl;
        return ScreenStruct();
    }
    return *vt100_screen_parser->GetScreenColored();
}

DLLEXPORT SelectPage GetSelectPage()
{
    // cout << "at get select page" << endl;
    if (vt100_screen_parser == NULL)
    {
        cout << "Error: vt100_screen_parser need init" << endl;
        select_page.Clear();
        return select_page;
    }
    SelectPage *select_page_ptr = vt100_screen_parser->GetSelectablePage();
    return *select_page_ptr;
}

DLLEXPORT WholePage GetWholePage()
{
    if (vt100_screen_parser == NULL)
    {
        cout << "Error: vt100_screen_parser need init" << endl;
        whole_page.Clear();
        return whole_page;
    }
    vector<string> whole = vt100_screen_parser->GetWholePage();
    whole_page.heigh = vt100_screen_parser->GetHeight();
    whole_page.width = vt100_screen_parser->GetWidth();
    for (int i = 0; i < whole_page.heigh; i++)
    {
        int w = whole_page.width < whole[i].size() ? whole_page.width : whole[i].size();
        // cout << whole[i] << endl;
        for (int j = 0; j < w; j++)
        {
            whole_page.data[i][j] = whole[i][j];
        }
    }
    return whole_page;
}

DLLEXPORT bool CheckVt100Draw(char *data)
{
    // cout << "this is check vt100 draw" << endl;
    DebugScreen debug_screen = DebugScreen();
    std::vector<Vt100Cmd> debug_info = debug_screen.SerialOutputSplit(data);
    for (auto it = debug_info.begin(); it != debug_info.end(); it++)
    {
        if ((it->description_ == "draw") && !(it->params_.size() == 1 && it->params_[0] == ""))
            return true;
    }
    return false;
}

DLLEXPORT char *ParseEdkShell(char *input, bool remove_ec_logs)
{
    /*
        Function Name       : parse_edk_shell()
        Parameters          : inputs
        Functionality       : parse the output of EDK/EFI shell serial output
                              return string
        Function Invoked    : DebugScreen.serial_output_split()
        Return Value        : str
    */
    vector<Vt100Cmd> events;
    string ret_str = "";
    DebugScreen debug_screen = DebugScreen(true);
    string esc_parse = ParseWithoutEsc(input, events);
    events = debug_screen.SerialOutputSplit(esc_parse, false, remove_ec_logs);

    for (auto it = events.begin(); it != events.end(); it++)
    {
        Vt100Cmd event = *it;
        if (event.description_ == "draw")
        {
            if (event.params_.size() == 1)
                ret_str += event.params_[0];
        }
    }
    if (edk_shell_string != NULL)
    {
        delete[] edk_shell_string;
        edk_shell_string = NULL;
    }

    edk_shell_string = new char[ret_str.length() + 1];
    Strcpy(edk_shell_string, ret_str, ret_str.length() + 1);
    return edk_shell_string;
}
