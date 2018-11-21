#include <iostream>
#include "src/cmdprompt/CommandPrompt.h"
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
        /* either put a scope around the CommandPrompt object, or call cmdprompt.disable_rawmode(), to restore terminal.
           Destructor of CommandPrompt calls disable_rawmode(). It's a matter of taste how you do it. */
        CommandPrompt cmdprompt{"test> "};
        cmdprompt.register_validator([cms = commands](const auto &input) {
            return std::any_of(cms.begin(), cms.end(), [&](auto cmd) { return cmd == input; });
        });
        cmdprompt.register_commands(commands);
        cmdprompt.register_completion_cb([cms = commands, results=std::vector<std::string>{}, index = 0, current=std::string{""}, result = std::optional<std::string>{}](auto str) mutable -> std::optional<std::string> {
            if(current == str) {
                // continue scrolling through commands
                // using current_index
                if(index < results.size()) {
                    auto res = results[index];
                    index++;
                    return (result = res);
                } else {
                    index = 0;
                    return (result = {});
                }
            } else {
                current = str;
                index = 0;
                results.clear();
                // copies all results that begin with str, from cms vector to results vector, so these can be scrolled through.
                auto b = cms.cbegin();
                auto e = cms.cend();
                std::copy_if(b, e, std::back_inserter(results), [&](auto s) { return std::equal(str.begin(), str.end(), s.begin()); });
                if(!results.empty() && index < results.size()) {
                    auto res = results[index];
                    index++;
                    return (result = res);
                } else {
                    index = 0;
                    return (result = {});
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