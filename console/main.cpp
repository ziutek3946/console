#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <vector>

#include <elcc/completion.h>
#include <elcc/editor.h>
#include <elcc/history.h>

#include "Cfg_reader.h"

// dumb poll based event loop, for example purposes _only_
namespace dumb_ev {

struct watch {
	typedef std::function<void(short)> callback;

	watch()
		:
			fd(-1),
			events(0),
			cb(0)
	{ }

	watch(int fd_, short events_, callback const& cb_)
		:
			fd(fd_),
			events(events_),
			cb(cb_)
	{ }

	struct pollfd pollfd() const
	{
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = events;
		return pfd;
	}

	int fd;
	short events;
	callback cb;
};

class loop {
public:
	typedef std::map< int, watch > map_t;

	class poll_set {
	public:
		poll_set(map_t const& watches)
			:
				nfds_(watches.size()),
				pfds_((struct pollfd *) malloc(nfds_ * sizeof(struct pollfd)))
		{
			if (!pfds_) {
				throw std::bad_alloc();
			}

			map_t::const_iterator it(watches.begin());
			for (size_t i(0); it != watches.end(); ++it, ++i) {
				pfds_[i] = it->second.pollfd();
			}
		}

		~poll_set()
		{
			free(pfds_);
		}

		size_t nfds() const
		{ return nfds_; }

		struct pollfd *pfds() const
		{ return pfds_; }

		short revents(size_t i) const
		{
			if (i > nfds_) {
				throw std::runtime_error("out of range");
			}

			return pfds_[i].revents;
		}

		int fd(size_t i) const
		{
			if (i > nfds_) {
				throw std::runtime_error("out of range");
			}

			return pfds_[i].fd;
		}

	private:
		poll_set(poll_set const&);
		poll_set & operator=(poll_set const&);

		nfds_t nfds_;
		struct pollfd *pfds_;
	};

	loop() :
		wakeup_fd_(-1),
		watches_(),
		stop_flag_(true)
	{
		int fds[2];

		if (pipe(fds) == -1) {
			throw std::runtime_error("error in pipe()");
		}
		fcntl(fds[0], F_SETFL, FD_CLOEXEC|O_NONBLOCK);
		fcntl(fds[1], F_SETFL, FD_CLOEXEC|O_NONBLOCK);

		wakeup_fd_ = fds[1];
		add_watch(fds[0], POLLIN, std::bind(&loop::set_stop_flag, this));
	}


	void run()
	{
		stop_flag_ = false;

		do {
			poll_set pset(watches_);

			int ret(-1);
			do {
				ret = poll(pset.pfds(), pset.nfds(), -1);
			} while (ret == -1 && errno == EINTR);

			std::vector < std::function<void(void)> > cb;
			for (size_t i(0); i < pset.nfds(); ++i) {
				if (pset.revents(i)) {
					cb.push_back(std::bind(
						watches_[pset.fd(i)].cb, pset.revents(i)));
				}
			}

			std::vector < std::function<void(void)> >::iterator cb_it(cb.begin());
			for (; cb_it != cb.end(); ++cb_it) {
				(*cb_it)();
			}

		} while (!stop_flag_);
	}

	void add_watch(int fd, short events, std::function<void(short)> const& cb)
	{
		watches_[fd] = watch(fd, events, cb);
	}

	void remove_watch(int fd)
	{
		watches_.erase(fd);
	}

	void stop()
	{
		char x(42);
		if (write(wakeup_fd_, &x, 1) < 0) {
			throw std::runtime_error("error in write()");
		}
	}
private:
	void set_stop_flag()
	{
		stop_flag_ = true;
	}

	int wakeup_fd_;
	std::map< int, watch > watches_;
	bool stop_flag_;
};

class editor : public elcc::editor {
public:
	editor(std::string const& name, loop & loop)
		:
			elcc::editor(name, std::bind(&editor::toggle_watch, this,
				std::placeholders::_1, std::placeholders::_2)),
			loop_(loop)
	{
	}

	~editor() {}
private:
	void on_watch_event(short revents)
	{
		if (revents & POLLIN) {
			handle_io();
		}
	}

	void toggle_watch(int fd, bool on)
	{
		if (!on) {
			loop_.remove_watch(fd);
			loop_.stop();
			return;
		}

		loop_.add_watch(fd, POLLIN, std::bind(&editor::on_watch_event,
			this, std::placeholders::_1));
	}

	loop & loop_;
};

}

void on_line(elcc::editor & el, std::string const& line)
{
    std::vector<std::string> tokeny;
	el.history().enter(line);
	Cfg_reader cfg;
	cfg.translate_k2_k0(line);
}

std::string fancy_prompt()
{
	return "> ";
}

elcc::function_return eof_handler(dumb_ev::loop & loop)
{
	loop.stop();
	fprintf(stdout, "\n");
	return elcc::eof;
}

elcc::word_list completion_handler(const elcc::word_list& words, size_t idx)
{
    Cfg_reader cfg;

	std::vector<std::string> cmds;

	switch (idx) {
	case 0:
		return cfg.GetMacs();
		break;
	case 1:
		return cfg.GetCommandsForMac(words[idx - 1]);
		break;
	//case 2:
		//std::vector<std::string> ex;
		//cfg.tokenizacja("ala ma kota");
		//for(int i=0;i<ex.size();++i)
		//{
        //    std::cout<<ex[i]<<"   ";
		//}
	//	break;
	default:
		break;
	}

	return cmds;
}

int main()
{
	setlocale(LC_CTYPE, "");

	dumb_ev::loop loop;
	dumb_ev::editor el("elcc", loop);

	el.prompt_cb(&fancy_prompt);
	el.line_cb(std::bind(&on_line, std::ref(el), std::placeholders::_1));

	el.add_function("exit", "exit at eof", std::bind(&eof_handler, std::ref(loop)));
	el.bind("^D", "exit");

	el.bind_completer("^I", std::bind(&completion_handler, std::placeholders::_1, std::placeholders::_2));
	el.start();

	loop.run();
	return 0;
}
