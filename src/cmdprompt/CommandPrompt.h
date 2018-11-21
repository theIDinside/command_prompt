//
// Created by cx on 2018-11-18.
//
#pragma once
#include <vector>
#include <string>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <memory>
#include <zconf.h>
#include <functional>
#include <iostream>

enum KeyCode {
    CTRL_A = 1,
    CTRL_B = 2,
    CTRL_C = 3,
    CTRL_D = 4,
    CTRL_E = 5,
    CTRL_F = 6,
    CTRL_H = 8,
    TAB = 9,
    CTRL_K = 11,
    CTRL_L = 12,
    ENTER = 13,
    CTRL_N = 14,
    CTRL_P = 16,
    CTRL_T = 20,
    CTRL_U = 21,
    CTRL_W = 23,
    ESC = 27,
    BACKSPACE = 127,
};

enum Error: int {
    Err = -1,
    OK = 0
};
using Validator = std::function<bool(std::string)>;
using Completer = std::function<std::optional<std::string>(std::string)>;

/*  This can be used as a completion callback, instead of using a lambda. I've removed the use of this struct for now though. If you use a lambda with register_completion_cb(...);
 *  you need to capture variables like for example commands, an index value and the lambda needs to be mutable, in order to store state, from call to call. Example exists in main.cpp*/
struct CompletionCallback {
    std::string current;
    std::vector<std::string> all_commands;
    std::vector<std::string> results;
    std::optional<std::string> m_result;
    std::size_t current_index = 0;
    CompletionCallback() = default;
    explicit CompletionCallback(std::vector<std::string> commands) : current{""}, all_commands(std::move(commands)), results{}, m_result{} {}
    std::optional<std::string> operator()(std::string s);
    bool has_value() {
        return m_result.has_value();
    }
};

class CommandPrompt {
    static constexpr const char* HORIZONTAL_CURSOR_POS = "\x1b[6n";
    static constexpr const char* STEPUP = "";                               // not yet in use
    static constexpr const char* STEPDOWN = "";                             // not yet in use
    static constexpr const char* CR = "";                                   // not yet in use
    static constexpr const char* SAVE_CURSOR_POS = "\x1b[s";                // not yet in use
    static constexpr const char* RESTORE_CURSOR_POS = "\x1b[u";             // not yet in use
    static constexpr const char* CLEAR_REST_OF_LINE = "\x1b[0K";
    static constexpr const char* MOVE_FORWARD_N = "\x1b[NC";
    static constexpr const char* MOVE_BACKWARD_N = "\x1b[ND";
    static constexpr const char* GOTO_COLUMN_N = "\x1b[NG";
    static constexpr const char* ERROR_COLOR = "\x1b[38;5;9m"; // use case: "\x1b[38;5;9m some message colored red\x1b[m"
    static struct termios* original_term_settings;
    static struct winsize* ws;
public:
    using usize = std::size_t;
    CommandPrompt() = delete;
    explicit CommandPrompt(std::string&& prompt);
    ~CommandPrompt() {
        disable_rawmode();
    }
    void clear_line();                      // not yet in use
    void clear_input_from(usize n=0);       // not yet in use
    std::optional<std::string> get_input();
    void register_validator(Validator&& v);
    void register_commands(const std::vector<std::string>& v);
    void register_completion_cb(Completer&& cb);

    std::vector<std::string> get_history();
    void print_data(const std::string& data);
    void print_data(const std::vector<std::string>& data);
    template <typename ...Args>
    void print_error(Args&&... msg)  {
        const char delim = ' ';
        // holy s**t you gotta love variadic templates in c++17 and up! Simple as ABC, easy as 123.
        std::cerr << '\r' << ERROR_COLOR << ((msg + delim) + ...) << "\x1b[m" << '\r' << std::endl;
    }
    void disable_rawmode();
    std::optional<std::string> get_last_input();
    std::optional<std::string> get_error_input();
private: /*         private functions       */
    bool read_input();
    Error set_rawmode();
    void display_prompt();
    auto get_columns();                     // not yet in use
    void step_n_forward(usize n = 1);
    void step_n_backward(usize n = 1);
    void goto_column(usize n);
    usize get_cursor_pos();
    usize get_data_index();
    void write_debug_file();

private: /*         private members       */
    const std::string m_prompt;
    const usize m_prompt_len;
    std::string m_buffer;
    std::vector<std::string> m_commands;
    std::optional<std::string> m_error{};
    /* for when trying to auto complete with tab*/
    // history
    std::vector<std::string> history;
    bool m_raw_mode_set;
    std::optional<std::string> m_completion_result{};
    Validator m_validator;
    Completer m_completion_cb;
};
