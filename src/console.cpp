#include "console.hpp"

#include <ctime>
#include <chrono>
#include <sstream>
#include <ncurses.h>
#include <iostream>
#include <cstring>

namespace ConsoleWriter {
    std::shared_ptr<ConsoleInterface> _console { nullptr };
    const std::string ConsoleInterface::_acceptable_characters =
	" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\""
	"£$%^&*()+-=_[]{}@:;'#~?/|.,<>\\";
};

void
ConsoleWriter::timestamped_message
(std::string const& message) noexcept {
    ConsoleInterface::Message msg;
    msg.add_chunk(timestamp(true), ConsoleInterface::Message::TIMESTAMP);
    msg.add_chunk(message, ConsoleInterface::Message::NORMAL);
#if DEBUG
    for (auto const& st : msg._strs) {
	std::cout << st;
    };
    std::cout<< "\n";
#else
    _console->add_message(std::move(msg));
#endif
};

void
ConsoleWriter::error_message
(std::string const& message) noexcept {
    ConsoleInterface::Message msg;
    msg.add_chunk(timestamp(true), ConsoleInterface::Message::TIMESTAMP);
    msg.add_chunk("[ERROR]", ConsoleInterface::Message::ERROR);
    msg.add_chunk(message, ConsoleInterface::Message::NORMAL);
#if DEBUG
    auto &s = msg._strs;
    for (auto const& st : s) {
	std::cout << st;
    };
    std::cout<< "\n";
#else
    _console->add_message(std::move(msg));
#endif
};

std::string ConsoleWriter::timestamp(const bool padded) noexcept {
    const time_t curr_time = time(NULL);
    char buff[32];
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&curr_time));
    std::string time_str(&(buff[0]), 20);
    time_str.erase(std::find(time_str.begin(), time_str.end(), '\0'),
	      time_str.end());
    std::stringstream ss;
    if (padded) {
    	ss <<  '[' << time_str << ']';
    } else {
	ss << time_str;
    };
    return ss.str();
};

bool ConsoleWriter::can_shutdown() {
    return !_console || _console->is_deletable();
};

void ConsoleWriter::shutdown() {
    _console = nullptr;
};

void ConsoleWriter::end_console_loop() {
    if (_console) {
	_console->shutdown();
    };
};

void
ConsoleWriter::add_command
(std::string &&command_string,
 std::shared_ptr<const Command> const& command) 
{
    if (_console) {
	_console->add_command(std::move(command_string),
			      command);
    };
};

void
ConsoleWriter::add_command
(std::vector<std::string> &&command_strings,
 std::shared_ptr<const Command> const& command)
{
    if (_console) {
	_console->add_command(std::move(command_strings),
			      command);
    };
};

// Terminal class

std::shared_ptr<ConsoleWriter::ConsoleInterface>
ConsoleWriter::ConsoleInterface::create() {
    ConsoleWriter::_console = std::make_shared<ConsoleInterface>();
    _console->_user_entry_thread =
	std::make_unique<std::thread>(std::bind
				      (&ConsoleInterface::run_user_input,
				       std::ref(*_console)));
    return ConsoleWriter::_console;
};

ConsoleWriter::ConsoleInterface::ConsoleInterface() 
    : ThreadedProcess(0)
    , _current_index(0) {
    initscr();
    curs_set(0);
    start_color();
    init_pair(Message::HIGHLIGHT, COLOR_BLACK, COLOR_WHITE);
    init_pair(Message::NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(Message::INPUT, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(Message::TIMESTAMP, COLOR_CYAN, COLOR_BLACK);
    init_pair(Message::ERROR, COLOR_RED, COLOR_BLACK);
    keypad(stdscr, TRUE);
    noecho();
    _terminal_running = true;
    print_separator();
    print_input_buffer();
    add_default_commands();
    this->start();
};

ConsoleWriter::ConsoleInterface::~ConsoleInterface() {
    _terminal_running = false;
    _user_entry_thread->join();
    _thread->join();
    refresh();
    endwin();
    std::cout << "goodbye world.\n";
};

void ConsoleWriter::ConsoleInterface::start() {
    _thread = std::make_shared<std::thread>
	([&]( ) { this->run_console(); });
};

void ConsoleWriter::ConsoleInterface::add_message(Message const& message) {
    std::scoped_lock<std::mutex> local_mutex(_msg_lock);
    _message_queue.emplace(message);
};

void ConsoleWriter::ConsoleInterface::run_console() {
    _running = true;
    while (_running) {
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	while (!_message_queue.empty()) {
	    send_next_message();
	};
    };
    send_shutdown_message();
    _deletable = true;
};

std::string ConsoleWriter::ConsoleInterface::current_buffer_string() noexcept {
    std::stringstream ss;
    for (const auto &c : _input_buffer) {
	ss << c;
    };
    return ss.str();
};

void ConsoleWriter::ConsoleInterface::send_shutdown_message() noexcept {
    Message msg;
    msg.add_chunk(timestamp(true), Message::TIMESTAMP);
    msg.add_chunk("Console shut down.", Message::NORMAL);
    std::scoped_lock<std::mutex> lock(_msg_lock);
    print_line(_sent_messages.size(), std::move(msg));
}

void ConsoleWriter::ConsoleInterface::send_next_message() noexcept {
    if (_message_queue.empty())
	return;
    std::scoped_lock<std::mutex> lock(_msg_lock);
    print_line(_sent_messages.size(), std::move(_message_queue.front()));
    _message_queue.pop();
};

void ConsoleWriter::ConsoleInterface::run_user_input() {
    while (_terminal_running) {
	handle_input(static_cast<KeyPress>(getch()));
    };
};

void ConsoleWriter::ConsoleInterface::handle_input(int const input) {
    switch (input) {
    case KeyPress::LEFT:
    case KeyPress::RIGHT:
    case KeyPress::UP:
    case KeyPress::DOWN: {
	move_index(input);
	break;
    }
    case KeyPress::BACKSPACE: {
	remove_character();
	break;
    }
    case KeyPress::ENTER: {
	execute_message();
	break;
    }
    default: {
	add_character(static_cast<char>(input));
	break;
    };
    };
    print_input_buffer();
};

void ConsoleWriter::ConsoleInterface::remove_character() {
    if (_current_index == 0 || _input_buffer.empty() ||
	_current_index > _input_buffer.size()) {
	return;
    };
    _current_index--;
    auto it = _input_buffer.begin();
    std::advance(it, _current_index);
    _input_buffer.erase(it);
    print_input_buffer();
};

void ConsoleWriter::ConsoleInterface::execute_message() {
    std::stringstream ss;
    for (const auto &c : _input_buffer) {
	ss << c;
    };
    Message msg;
    msg.add_chunk(timestamp(true), Message::TIMESTAMP);
    msg.add_chunk(">", Message::INPUT);
    msg.add_chunk(ss.str(), Message::NORMAL);
    print_line(_sent_messages.size(), std::move(msg));
    handle_command(ss.str());
    _input_buffer.clear();
    _current_index = 0;
    print_input_buffer();
};

void ConsoleWriter::ConsoleInterface::add_character(char const input) {
    if (_acceptable_characters.find(input) == std::string::npos) {
	return;
    } else if (_current_index >= _input_buffer.size()) {
	_input_buffer.push_back(input);
    } else {
	auto it = _input_buffer.begin();
	std::advance(it, _current_index);
	_input_buffer.insert(it, input);
    };
    _current_index++;
};

void ConsoleWriter::ConsoleInterface::move_index(int const direction) {
    switch (direction) {
    case KeyPress::LEFT: {
	if (_current_index >= 1) {
	    _current_index--;
	};
	break;
    }
    case KeyPress::RIGHT: {
	if (_current_index < _input_buffer.size()) {
	    _current_index++;
	};
	break;
    }
    default: {
	break;
    };
    };
};

void
ConsoleWriter::ConsoleInterface::print_line
(int line, Message output, bool const save_msg) {
    if (line >= LINES - 3) {
	adjust_lines();
	line =  LINES - 3;
    };
    if (save_msg) {
	save_message(std::move(output));
    };
    std::scoped_lock<std::mutex> lock(_print_lock);
    move(line, 0);
    clrtoeol();
    output.send_message(line, 0);
};

void
ConsoleWriter::ConsoleInterface::adjust_lines
() {
    const size_t bottom_row = static_cast<size_t>(LINES) - 3;
    if (_sent_messages.size() >= bottom_row) {
    	for (size_t i = 1; i <= bottom_row; ++i) {
	    const auto index = _sent_messages.size() - i;
    	    print_line(bottom_row - i, _sent_messages[index], false);
    	};
    };
    print_separator();
};

void
ConsoleWriter::ConsoleInterface::print_input_buffer
() {
    std::scoped_lock<std::mutex> lock(_print_lock);
    size_t i = 0;
    move(LINES - 1, 0);
    clrtoeol();
    for (const auto &c : _input_buffer) {
	if (i == _current_index) {
	    attron(COLOR_PAIR(Message::HIGHLIGHT));
	    mvaddch(LINES - 1, i, c);
	    attroff(COLOR_PAIR(Message::HIGHLIGHT));
	} else {
	    mvaddch(LINES - 1, i, c);
	};
	++i;
    };
    if (_current_index == _input_buffer.size()) {
	attron(COLOR_PAIR(Message::HIGHLIGHT));
	    addch(' ');
	    attroff(COLOR_PAIR(Message::HIGHLIGHT));
    }
    refresh();
};

void ConsoleWriter::ConsoleInterface::print_separator() {
    for (int i = 0; i < COLS; ++i) {
	mvaddch(LINES - 2, i, '-');
    };
};

void
ConsoleWriter::ConsoleInterface::Message::send_message
(int const row, int const column) const {
    int index = column;
    for (size_t i = 0; i < _colour_pairs.size() && i < _strs.size(); ++i) {
	attron(COLOR_PAIR(_colour_pairs[i]));
	mvaddstr(row, index, _strs[i].c_str());
	attroff(COLOR_PAIR(_colour_pairs[i]));
	index += _strs[i].size() + 1;
    };
    refresh();
};

void
ConsoleWriter::ConsoleInterface::Message::add_chunk
(std::string msg, int const colour) {
    _strs.emplace_back(msg);
    if (colour >= COLOURS_COUNT || colour < NORMAL) {
	_colour_pairs.push_back(NORMAL);
    } else {
	_colour_pairs.push_back(colour);
    };
};

void
ConsoleWriter::ConsoleInterface::handle_command
(std::string const& command) {
    const auto pos = command.find(' ');
    const bool args = pos != std::string::npos;
    const std::string cmd = !args ? command :
	command.substr(0, pos);
    const std::string arg = !args ? "" :
	command.substr(pos + 1, command.size() - 1);
    const auto it = _commands.find(cmd);
    if (it == _commands.end()) {
	std::stringstream ss;
	ss << "Command \"" << cmd << "\" not found.";
	error_message(ss.str());		      
    } else {
	const std::string result = it->second->_callback(arg);
	handle_command_result(result);
    };
};

void
ConsoleWriter::ConsoleInterface::handle_command_result
(std::string const& result) {
    if (!result.empty()) {
	timestamped_message(result);
    };
    
};

void
ConsoleWriter::ConsoleInterface::add_command
(std::string&& command_string,
 std::shared_ptr<const Command> const& command) {
    std::scoped_lock<std::mutex> lock(_commands_lock);
    _commands[std::move(command_string)] = command;
};

void
ConsoleWriter::ConsoleInterface::add_command
(std::vector<std::string>&& command_strings,
  std::shared_ptr<const Command> const& command) {
    for (std::string& c : command_strings) {
	add_command(std::move(c), command);
    };
};

void
ConsoleWriter::ConsoleInterface::add_default_commands
() {
    // hard code default commands here
    auto commands = [&] (std::string const&) {
	std::stringstream ss;
	ss << "Commands: ";
	std::scoped_lock<std::mutex> lock(_commands_lock);
	size_t i = 1;
	for (auto const& cmd:_commands) {
	    ss << cmd.first;
	    if (i++ != _commands.size()) {
		ss << ", ";
	    };
	}
	return ss.str();
    };
    auto list_command = std::make_shared<ConsoleWriter::Command>
	("List all commands",
	 std::move(commands));
    add_command("commands",
		list_command);

    // help
    auto help = [&] (std::string const& cmd_arg) {
	if (cmd_arg.empty()) {
	    return std::string("Type \"help <command>\" for help with that comm"
			       "and.Type \"commands\" for a list of commands.");
	};
	
	auto cmd_it = _commands.find(cmd_arg);
	if (cmd_it == _commands.end()) {
	    std::stringstream ss;
	    ss << "Command \"" << cmd_arg <<
		"\" not found, type \"commands\" to list all commands.";
	    return ss.str();
	} else {
	    std::stringstream ss;
	    ss << cmd_arg << ": " << cmd_it->second->_description;
	    return ss.str();
	};
    };
    auto help_command = std::make_shared<Command>
	("Type \"help <command>\" for help with that command.",
	 std::move(help));
    add_command("help",
		help_command);    
};

void
ConsoleWriter::ConsoleInterface::save_message
(Message && output) noexcept {
    _sent_messages.emplace_back(std::move(output));
    if (_sent_messages.size() > MAX_MSG_BUFFER) {
	_sent_messages.erase(_sent_messages.begin());
    };
};
