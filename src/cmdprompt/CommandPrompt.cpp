//
// Created by cx on 2018-11-18.
//

/*
 *
 * Custom command prompt is not implemented yet.
 *
 */


#include "CommandPrompt.h"

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios* CommandPrompt::g_original_term_settings = new struct termios;
struct winsize* CommandPrompt::g_ws = new struct winsize;

CommandPrompt::CommandPrompt(std::string &&prompt, bool keep_history=false) :
                        m_prompt(prompt), m_prompt_len(prompt.size()),
                        m_buffer{}, m_error{}, m_history{}, m_history_item{},
                        m_raw_mode_set(false), m_save_history(keep_history), m_completion_result{}

{
    set_rawmode();
    std::string s{m_prompt};
    m_buffer.reserve(40); // arbitrary reserved size.
    write(STDOUT_FILENO, s.c_str(), s.size());
    if(keep_history)
        set_save_history("./history.log");
}

Error CommandPrompt::set_rawmode() {
    if(tcgetattr(STDIN_FILENO, CommandPrompt::g_original_term_settings) != Error::Err);
    struct termios raw_mode = *(CommandPrompt::g_original_term_settings);
    /*
     * We are using boolean operations of AND and flipping the bits of provided values. So ~ICRNL means, DON'T use new line or carriage return
     * since we are AND'ing those flipped bits with the raw_mode.c_iflag value.
     */
    raw_mode.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
    raw_mode.c_oflag &= ~(OPOST); // disables post processing
    raw_mode.c_cflag |= CS8;
    raw_mode.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* 1 byte and no timer, means we return a read byte immediately. And since we are only using 8-bit chars this won't be a problem. */
    raw_mode.c_cc[VMIN] = 1;
    raw_mode.c_cc[VTIME] = 0;
    // next we need to set the terminal, with the provided "config struct" that we been setting:
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_mode);

    this->m_raw_mode_set = true;
    return Error::OK;
}



void CommandPrompt::disable_rawmode() {
    if(m_raw_mode_set) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, g_original_term_settings);
    }
}

auto CommandPrompt::get_columns() {
    ioctl(1, TIOCGWINSZ, g_ws);
    return g_ws->ws_col;
}

void CommandPrompt::step_n_forward(CommandPrompt::usize n)  { // n is default 1 step
    auto current_pos = get_data_index();
    if(current_pos < m_buffer.size()) {
        std::string s{CommandPrompt::MOVE_FORWARD_N};
        s.replace(2, 1, std::to_string(n));
        if(write(STDOUT_FILENO, s.c_str(), 4) != -1) {}
    }
}

void CommandPrompt::step_n_backward(CommandPrompt::usize n) {
    auto current_pos = get_data_index();
    if(current_pos > 0) {
        std::string s{CommandPrompt::MOVE_BACKWARD_N};
        s.replace(2, 1, std::to_string(n));
        if(write(STDOUT_FILENO, s.c_str(), 4) != -1) {}
    }
}
void CommandPrompt::goto_column(CommandPrompt::usize n) {
    std::string s{GOTO_COLUMN_N};
    s.replace(2, 1, std::to_string(n));
    if(write(STDOUT_FILENO, s.c_str(), 4) != -1) {}
}

void CommandPrompt::register_validator(Validator&& v) {
    this->m_validator = std::move(v);
}

std::optional<std::string> CommandPrompt::get_input() {
    std::string result;
    display_prompt();
    while(read_input()) {}
    if(m_validator(m_buffer)) {
        if(m_buffer.empty())
        {
            return {};
        }
        m_history.push_back(m_buffer);
        std::copy(m_buffer.begin(), m_buffer.end(), std::back_inserter(result));
        m_buffer.clear();
        return result;
    } else {
        m_error = m_buffer;
        m_buffer.clear();
        return {};
    }
}

void CommandPrompt::auto_fill() {
    std::string write_buffer{GOTO_COLUMN_N};
    write_buffer.replace(2, 1, std::to_string(m_prompt_len+1));
    write_buffer.append(CLEAR_REST_OF_LINE);
    auto output = m_completion_result.value_or(m_buffer);
    write_buffer.append(output);
    write_buffer.append(std::string{GOTO_COLUMN_N}.replace(2, 1, std::to_string(m_prompt_len+1+output.size())));
    write(STDOUT_FILENO, write_buffer.c_str(), write_buffer.size());
}

bool CommandPrompt::read_input() {
    char sequence[3]; // CTRL + A for example, is a 3 byte char seq
    char ch;
    auto chars_read = read(STDIN_FILENO, &ch, 1);
    if (chars_read <= 0) return true; // continue reading if nothing was read.
    if (ch == KeyCode::TAB) {
        m_completion_result = m_completion_cb(m_buffer);
        auto_fill();
        return true;
    }
    switch (ch) {
        case CTRL_KEY('w'):
        case CTRL_KEY('h'): { // CTRL+Backspace
            if(auto pos = m_buffer.find_last_of(' '); pos != std::string::npos) {
                m_buffer.erase(pos);
            } else {
                m_buffer.clear();
            }
            auto_fill();
            break;
        }
        case KeyCode::ENTER:
            std::cout << std::endl;
            m_buffer = m_completion_result.value_or(m_buffer);
            m_completion_result = {};
            goto_column(0);
            return false;
        case KeyCode::ESC:
            if (read(STDIN_FILENO, sequence, 1) == -1) break;
            if (read(STDIN_FILENO, sequence + 1, 1) == -1) break;
            if (sequence[0] == '[') {
                if (sequence[1] >= '0' && sequence[1] <= '9') {

                } else {
                    switch (sequence[1]) {
                        case 'A': // todo: scroll up through history list.-
                            m_completion_result = get_history_prev();
                            auto_fill();
                            break;
                        case 'B': // todo: scroll down through history list.-
                            m_completion_result = get_history_next();
                            break;
                        case 'C': // right
                            m_buffer = m_completion_result.value_or(m_buffer);
                            m_completion_result = {};
                            step_n_forward();
                            break;
                        case 'D': // left
                            step_n_backward();
                            break;
                        case 'H': // home key
                        {
                            goto_column(m_prompt_len + 1);
                            break;
                        }
                        case 'F': // end key
                            goto_column(m_prompt_len + 1 + m_buffer.size());
                            break;
                        default:
                            break;
                    }
                }
            } else if (sequence[0] == '0') {
                switch (sequence[1]) {
                    case 'H': /* home */
                        goto_column(m_prompt_len + 1);
                        break;
                    case 'F': /* end */
                        goto_column(m_prompt_len + 1 + m_buffer.size());
                        break;
                    case 'W':
                    {
                        if(auto pos = m_buffer.find_last_of(' '); pos != std::string::npos) {
                            if(sequence[1] == KeyCode::BACKSPACE) {
                                m_buffer.erase(pos);
                                auto_fill();
                            } else {
                                goto_column(m_prompt_len + 1);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            } break;
        case KeyCode::BACKSPACE: {
            auto current_position = get_data_index();
            auto cursorpos = get_cursor_pos();
            m_buffer = m_completion_result.value_or(m_buffer);
            m_completion_result = {};
            if (current_position >= m_buffer.size() && !m_buffer.empty() && cursorpos > m_prompt_len + 1) {
                m_buffer.pop_back();
                std::string write_buffer{GOTO_COLUMN_N};
                write_buffer.replace(2, 1, std::to_string(cursorpos - 1));
                write_buffer.append(CLEAR_REST_OF_LINE);
                write(STDOUT_FILENO, write_buffer.c_str(), write_buffer.size());
            } else if (!m_buffer.empty() && current_position < m_buffer.size() + 1 && current_position != 0 &&
                       cursorpos > m_prompt_len + 1) {
                m_buffer.erase(current_position, 1);
                std::string write_buffer{GOTO_COLUMN_N};
                write_buffer.replace(2, 1, std::to_string(m_prompt_len + 1));
                write_buffer.append(CLEAR_REST_OF_LINE);
                write_buffer.append(m_buffer);
                write_buffer.append(std::string{GOTO_COLUMN_N}.replace(2, 1, std::to_string(cursorpos - 1)));
                write(STDOUT_FILENO, write_buffer.c_str(), write_buffer.size());
            }
            return true;
        } default:
            // enter character into character buffer m_buffer
            m_buffer = m_completion_result.value_or(m_buffer);
            m_completion_result = {};
            auto current_pos = get_data_index();
            if (ch == KeyCode::TAB) break;
            if (current_pos == m_buffer.size()) {
                m_buffer.push_back(ch);
                write(STDOUT_FILENO, &ch, 1);
            } else if (current_pos < m_buffer.size()) {
                auto cursor = get_cursor_pos();
                std::string write_buffer{GOTO_COLUMN_N};
                write_buffer.replace(2, 1, std::to_string(m_prompt_len + 1));
                m_buffer.insert(current_pos, 1, ch);
                write_buffer.append(m_buffer);
                write_buffer.append(std::string{GOTO_COLUMN_N}.replace(2, 1, std::to_string(cursor + 1)));
                write(STDOUT_FILENO, write_buffer.c_str(), write_buffer.size());
            }
            return true;
        }
        // finally, if we haven't entered a control sequence, push the character onto m_buffer;
    return true;
}

void CommandPrompt::register_commands(const std::vector<std::string> &v) {
    for(const auto &cmd : v) {
        m_commands.emplace_back(cmd);
    }
}

void CommandPrompt::display_prompt() {
    std::string s{GOTO_COLUMN_N};
    s.replace(2, 1, std::to_string(0));
    s.append(CLEAR_REST_OF_LINE);
    s.append(m_prompt);
    write(STDOUT_FILENO, s.c_str(), s.size());
}

std::vector<std::string> CommandPrompt::get_history() {
    return m_history;
}

void CommandPrompt::print_data(const std::vector<std::string> &data) {
    for(const auto& line : data) {
        print_data(line);
    }
}

CommandPrompt::usize CommandPrompt::get_cursor_pos() {
    char cursor_data[12];
    std::fill_n(cursor_data, 12, '\0');
    write(STDOUT_FILENO, HORIZONTAL_CURSOR_POS, 4);
    for(auto& c : cursor_data) {
        if(read(STDIN_FILENO, &c, 1) != 1) break;
        if(c == 'R') break;
    }
    if(cursor_data[0] == KeyCode::ESC && cursor_data[1] == '[') {
        std::string s{};
        std::copy_if(cursor_data+2, cursor_data+12, std::back_inserter(s), [&](const auto c) {
            return c != '\0';
        });
        s = s.substr(s.find_first_of(';')+1, s.find_first_of('R'));
        usize pos = 0;
        std::stringstream ss{s};
        ss >> pos;
        return pos;
    }
    return m_prompt_len+1;
}
CommandPrompt::usize CommandPrompt::get_data_index() {
     return get_cursor_pos()-(m_prompt_len+1);
}
void CommandPrompt::write_debug_file() {
    std::ofstream debug{"./debug.txt", std::ios::app};
    char cursor_data[12];
    std::fill_n(cursor_data, 12, '\0');
    write(STDOUT_FILENO, HORIZONTAL_CURSOR_POS, 4);

    for(auto& c : cursor_data) {
        if(read(STDIN_FILENO, &c, 1) != 1) break;
        if(c == 'R') break;
    }
    if(cursor_data[0] == KeyCode::ESC && cursor_data[1] == '[') {
        std::string s{};
        std::copy_if(cursor_data+2, cursor_data+12, std::back_inserter(s), [&](const auto c) {
            return c != '\0';
        });
        s = s.substr(s.find_first_of(';')+1, s.find_first_of('R'));
        debug << "Cursor position: " << s << " data index: " << get_data_index() << '\n';
    }
    debug.close();
}

void CommandPrompt::register_completion_cb(Completer&& c) {
    this->m_completion_cb = std::move(c);
}


void CommandPrompt::register_parameter_completer_cb(Completer &&cb) {
    this->m_parameter_completer_cb = std::move(cb);
}

std::optional<std::string> CommandPrompt::get_last_input() {
    return (m_history.empty() ? std::optional<std::string>{} : m_history[m_history.size()-1]);
}

std::optional<std::string> CommandPrompt::get_error_input() {
    return this->m_error;
}

void CommandPrompt::set_save_history(std::string history_file_path) {
    this->history_file_path = std::move(history_file_path);
    this->m_save_history = true;
}

void CommandPrompt::load_history(std::string history_file_path) {
    this->history_file_path = history_file_path;
    std::fstream history_file{};
    history_file.open(history_file_path.c_str(), std::ios::in);
    std::string line;
    auto _hist = std::vector<std::string>{};
    while(std::getline(history_file, line)) {
        if(m_validator(line))
            _hist.emplace_back(line);
    }
    // only keep 200 records of commands entered
    auto i = _hist.begin();
    auto e = _hist.end();
    if(_hist.size() > 200) {
        auto a = _hist.size() - 200;
        for(auto j = 0; j < a; ++j){++i;};
    }
    std::copy(i, _hist.end(), m_history.begin());
}

void CommandPrompt::write_history_to_file() {
    std::ofstream f{history_file_path.c_str()};
    std::vector<std::string> fv{};
    if(m_history.size() > 200) {
        auto i = m_history.begin();
        auto a = m_history.size() - 200;
        for(auto j = 0; j < a; ++j){++i;};
        std::copy(i, m_history.end(), std::back_inserter(fv));
    } else {
        std::copy(m_history.begin(), m_history.end(), std::back_inserter(fv));
    }
    for(const auto& h : fv) {
        f << h << '\n';
    }
    f << "------------------" << '\n';
    f.close();
}

std::optional<std::string> CommandPrompt::get_history_prev() {
    if(m_history.empty())
    {
        m_history_item = {};
        return {};
    }
    else {
        m_history_item = m_history_item.value_or(m_history.rbegin());
        if(m_history_item.value() != m_history.rend()) {
            auto res = std::optional<std::string>{*m_history_item.value()};
            (m_history_item.value())++;
            return res;
        } else {
            m_history_item = {};
            return {};
        }
    }
}

std::optional<std::string> CommandPrompt::get_history_next() {
    if(m_history.empty())
    {
        m_history_item = {};
        return {};
    }
    else {
        m_history_item = m_history_item.value_or(m_history.rbegin());
        if(m_history_item.value() != m_history.rbegin()) { // if we are at the latest history item, "next" should produce nothing.
            auto res = std::optional<std::string>{*m_history_item.value()};
            (m_history_item.value())--;
            return res;
        } else {
            m_history_item = {};
            return {};
        }
    }
}