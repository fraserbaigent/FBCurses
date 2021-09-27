#ifndef CLASS_CONSOLE
#define CLASS_CONSOLE

#define DEBUG 0

#include <thread>
#include <iostream>
#include <queue>
#include <string>
#include <mutex>
#include <memory>
#include <list>
#include <functional>
#include <unordered_map>

#include "threaded_process.hpp"

namespace ConsoleWriter {
    namespace ESC {
	static const std::string red = "\u001b[38;5;196m";
	static const std::string gre = "\u001b[38;5;82m";
	static const std::string yel = "\u001b[38;5;226m";
	static const std::string blu = "\u001b[38;5;21m";
	static const std::string mag = "\u001b[38;5;165m";
	static const std::string cya = "\u001b[38;5;51m";
	static const std::string whi = "\u001b[38;5;225m";
	static const std::string reset = "\033[0m";
    };

    struct Command {
	Command(std::string&& description,
		std::function<std::string(std::string const&)>&& callback) {
	    _description = std::move(description);
	    _callback = std::move(callback);
	};
	std::string _description;
	std::function<std::string(std::string const&)> _callback;
    };
    
    void timestamped_message(const std::string &message) noexcept;
    void error_message(const std::string &message) noexcept;
    std::string in_colour(const std::string &message,
			  const std::string &colour) noexcept;
    std::string timestamp(const bool padded) noexcept;

    void add_command(std::string&& command_string,
		     std::shared_ptr<const Command> const& command);
    void add_command(std::vector<std::string>&& command_strings,
		     std::shared_ptr<const Command> const& command);

    bool can_shutdown();
    void shutdown();
    void end_console_loop();
	
    class ConsoleInterface :
	public ThreadedProcess {
    public:
	class Message {
	public:
	    enum Colours {
		NORMAL,
		HIGHLIGHT,
		ERROR,
		TIMESTAMP,
		INPUT,
		COLOURS_COUNT
	    };

	    Message() = default;
	    void send_message(int const row,int const column) const;
	    void add_chunk(std::string const&msg, int const colour);
	    std::vector<int> _colour_pairs;
	    std::vector<std::string> _strs;
	};
    public:
	static std::shared_ptr<ConsoleInterface> create();
	ConsoleInterface();
	~ConsoleInterface();
	
	ConsoleInterface(ConsoleInterface const& other) noexcept = delete;
	ConsoleInterface& operator=
	(ConsoleInterface const& other) noexcept = delete;
	ConsoleInterface(ConsoleInterface&& other) noexcept = delete;
	ConsoleInterface& operator=
	(ConsoleInterface&& other) noexcept = delete;


	// PURE VIRTUAL FUNCTIONS
	std::string process_name() const noexcept override { return "Console"; }
	void start() override;
	// END OF PURE VIRTUAL FUNCTIONS

	
	void add_message(Message const& message);

	void run_console();

	void add_command(std::string&& command_string,
			 std::shared_ptr<const Command> const& command);
	void add_command(std::vector<std::string>&& command_strings,
			 std::shared_ptr<const Command> const& command);
    private:
	static const std::string _acceptable_characters;
	void print_line(int line, Message const& output,
			const bool save_msg = true);
	void send_next_message() noexcept;
	std::string current_buffer_string() noexcept;
	void check_for_input() noexcept;
	void send_shutdown_message() noexcept;

	void run_user_input();

	enum KeyPress : int {
	    DOWN = 258,
	    UP = 259,
	    LEFT = 260,
	    RIGHT = 261,
	    BACKSPACE = 263,
	    ENTER = 10,
	    ESCAPE = 27,
	};
	void handle_input(int const input);
	void remove_character();
	void move_index(int const direction);
	void execute_message();
	void add_character(char const input);
	void print_input_buffer();
	void print_separator();
	void adjust_lines();
	void handle_command(std::string const& command);
	void handle_command_result(std::string const& result);

	void add_default_commands();
	void save_message(Message && msg) noexcept;
    private:
	static constexpr size_t MAX_MSG_BUFFER { 100 };
	
	std::queue<Message> _message_queue;
	std::mutex _msg_lock;
	std::mutex _print_lock;
	std::mutex _commands_lock;

	// user entry
	std::unique_ptr<std::thread> _user_entry_thread;
	std::list<char> _input_buffer;
	std::vector<Message> _sent_messages;
	bool _terminal_running;
	size_t _current_index;

	// commands
	std::unordered_map<std::string,
			   std::shared_ptr<const Command>> _commands;
    };
};

#endif
