#include <iostream>
#include "../src/cmdprompt/CommandPrompt.h"
#include <algorithm>

int main() {

    std::vector<std::string> commands{"print", "add", "sub", "perf", "quit", "break", "list", "step", "stepn", "halt", "registers"};
    std::sort(commands.begin(), commands.end());
    std::cout << "Registered commands: " << '\n';
    for(const auto& c : commands) {
        std::cout << c << '\n';
    }
    std::vector<std::string> history{};
    {
        using Strvec = std::vector<std::string>;
        using Result = std::optional<std::string>;
        using String = std::string;
        auto save_history = false;
        /* either put a scope around the CommandPrompt object, or call cmdprompt.disable_rawmode(), to restore terminal.
           Destructor of CommandPrompt calls disable_rawmode(). It's a matter of taste how you do it. */
        CommandPrompt cmdprompt{"test> ", save_history};
        cmdprompt.register_validator([cms = commands](const auto &input) {
            return std::any_of(cms.begin(), cms.end(), [&](auto cmd) { return cmd == input; });
        });
        cmdprompt.register_commands(commands);
        // cmdprompt.load_history("./history.log"); // THIS MUST ONLY BE CALLED AFTER A VALIDATOR HAS BEEN REGISTERED, SO THAT NO NON-VALID COMMANDS GET LOADED FROM HISTORY FILE
        cmdprompt.register_completion_cb([cms = commands, res=Strvec{}, idx = 0, cur=String{""}, result = Result{}](String str) mutable -> std::optional<std::string> {
            if(cur == str) {
                // continue scrolling through commands
                // using current_index
                if(idx < res.size()) {
                    result = res[idx];
                    idx++;
                    return result;
                } else {
                    result = {};
                    idx = 0;
                    return result;
                }
            } else {
                cur = str;
                idx = 0;
                res.clear();
                // copies all res that begin with str, from cms vector to res vector, so these can be scrolled through.
                auto b = cms.cbegin();
                auto e = cms.cend();
                std::copy_if(b, e, std::back_inserter(res), [&](auto s) { return std::equal(str.begin(), str.end(), s.begin()); });
                if(!res.empty() && idx < res.size()) {
                    result = res[idx];
                    idx++;
                    return result;
                } else {
                    idx = 0;
                    result = {};
                    return result;
                }
            }
        });
        auto running = true;
        while (running) {
            auto s = cmdprompt.get_input().value_or("unknown");
            if (s == "unknown") {
                cmdprompt.print_error(std::string{"Unknown command"}, cmdprompt.get_error_input().value_or("<couldn't catch erroneous input>"));
            } else if (s == "quit") {
                running = false;
            }
        }
        history = cmdprompt.get_history();
    }
    std::cout << "Printing history" << '\n';
    for(const auto& c : history) {
        std::cout << c << '\n';
    }
    return 0;
}