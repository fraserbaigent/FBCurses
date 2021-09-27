#include "threaded_process.hpp"
#include "numerical.hpp"

#include <sstream>

ThreadedProcess::ThreadedProcess
(uint64_t const unique_id)
    : _unique_id(unique_id)
{
}

ThreadedProcess::~ThreadedProcess
() {
    for (const auto& callback : _dependent_callbacks) {
	callback(_unique_id);
    };
}

void
ThreadedProcess::add_dependency
(ThreadedProcess& other_process) {
    std::scoped_lock<std::mutex> lock(_dependencies_mutex);
    if (std::find(_dependencies.cbegin(),
		  _dependencies.cend(),
		  other_process.get_unique_id()) == _dependencies.cend()) {
	_dependencies.push_back(other_process.get_unique_id());
	other_process.add_dependent_callback( [&] (const uint64_t input) {
	    this->remove_dependency(input); });
    }
};

void
ThreadedProcess::remove_dependency
(const uint64_t process_id) {
    std::scoped_lock<std::mutex> lock(_dependencies_mutex);
    const auto it = std::find(_dependencies.cbegin(),
			      _dependencies.cend(),
			      process_id);
    if (it != _dependencies.cend()) {
	_dependencies.erase(it);
    };
};

bool ThreadedProcess::has_dependencies() const {
    std::scoped_lock<std::mutex> lock(_dependencies_mutex);
    return !_dependencies.empty();
};

bool
ThreadedProcess::is_deletable
() const {
    return _deletable && !has_dependencies();
};

void
ThreadedProcess::add_dependent_callback
(std::function<void (uint64_t)> &&callback) {
    std::scoped_lock<std::mutex> lock(_dependencies_mutex);
    _dependent_callbacks.push_back(std::move(callback));
};

bool
ThreadedProcess::can_exit_loop
(bool const control_bool) const {
    std::scoped_lock<std::mutex> lock(_dependencies_mutex);
    return !control_bool && _dependencies.empty();
};

void
ThreadedProcess::shutdown
() noexcept {
    _running = false;
};
