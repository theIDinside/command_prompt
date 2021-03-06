//
// Created by cx on 2018-11-18.
//
#pragma once

#include "heavy.h"

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

template <typename CommandType>
using ParamaterCompleter = std::function<std::optional<std::string>(CommandType, std::string)>;

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
    static struct termios* g_original_term_settings;
    static struct winsize* g_ws;
public:
    using usize = std::size_t;
    CommandPrompt() = delete;
    CommandPrompt(const CommandPrompt& prompt) : CommandPrompt(std::string{prompt.m_prompt}, prompt.m_save_history) {
        m_history.emplace_back(" ");
    }
    explicit CommandPrompt(std::string&& prompt, bool keep_history);
    ~CommandPrompt() {
        disable_rawmode();
        if(m_save_history)
            write_history_to_file();
        delete g_original_term_settings;
        delete g_ws;
    }
    std::optional<std::string> get_input();
    void register_validator(Validator&& v);
    void register_commands(const std::vector<std::string>& v);
    void register_completion_cb(Completer&& cb);
    void register_parameter_completer_cb(Completer && cb);
    void set_save_history(std::string history_file_path);
    void unset_save_history() {
        m_save_history = false;
    }
    std::vector<std::string> get_history();
    template <typename ...Args>
    void print_data(Args&&... data) {
        const char delimiter = ' ';
        std::cout << '\r';
        (std::cout << ... << data)  << "\r" << std::endl;
    }
    void print_data(const std::vector<std::string>& data);
    void load_history(std::string history_file);
    template <typename ...Args>
    void print_error(Args&&... msg)  {
        const char delim = ' ';
        // holy s**t you gotta love variadic templates in c++17 and up! Simple as ABC, easy as 123.
        std::cerr << '\r' << ERROR_COLOR;
        (std::cerr << ... << (msg + delim)) << "\x1b[39m" << '\r' << std::endl;
    }
    void disable_rawmode();
    std::optional<std::string> get_last_input();
    std::optional<std::string> get_error_input();

    std::optional<std::string> get_history_prev();
    std::optional<std::string> get_history_next();

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
    void write_history_to_file();
    void auto_fill();
private: /*         private members       */
    const std::string m_prompt;
    const usize m_prompt_len;
    std::string m_buffer;
    std::vector<std::string> m_commands;
    std::optional<std::string> m_error;
    /* for when trying to auto complete with tab*/
    // history
    std::vector<std::string> m_history;
    std::optional<std::vector<std::string>::reverse_iterator> m_history_item;

    std::string history_file_path;
    bool m_raw_mode_set;
    bool m_save_history;
    std::optional<std::string> m_completion_result;
    Validator m_validator;
    Completer m_completion_cb;
    Completer m_parameter_completer_cb;
};
