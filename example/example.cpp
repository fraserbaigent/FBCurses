#include "../src/console.hpp"

#include <mutex>
#include <condition_variable>

int main(int argc, char *argv[])
{
    using namespace ConsoleWriter;
    auto cnsl = ConsoleInterface::create();
    auto add_cmd = std::make_shared<Command>
	(
	 "Add a series of space separated numbers",
	 [cnsl = cnsl] (std::string const& args) {
	     double total = 0.0;
	     size_t s_pos = 0;
	     size_t pos = args.find(' ');
	     do {
		 try {
		     total += static_cast<double>
			 (std::stoi(args.substr(s_pos, pos)));
		 } catch ( ... ) {
		     total += 0.0;
		 }
		 s_pos = pos;
		 pos = args.find(' ', pos);
	     } while (pos != std::string::npos);
	     return "The numerical total is : " + std::to_string(total);
	 });
    
    cnsl->add_command("add", add_cmd);

    std::mutex mtx;
    std::condition_variable cv;


    // Let the user terminate the programme from the terminal
    // The terminal cleanup should be tied to the shutdown 

    auto shutdown_cmd = std::make_shared<Command>
	(
	 "Shut down the example programme",
	 [cnsl = cnsl, cv_ptr = &cv](std::string const& args) {
	     cnsl->shutdown();
	     cv_ptr->notify_all();
	     return ""; 
	 });
    cnsl->add_command("shutdown", shutdown_cmd);
    
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&]() { return cnsl->is_deletable();});

    std::cout <<"niciicescsdfs\n";
    // make sure console is dead before we shutdown
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    shutdown();
		
    return 0;
}
