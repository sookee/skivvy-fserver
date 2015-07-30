#pragma once
#ifndef SKIVVY_IRCBOT_FSERVER_H
#define SKIVVY_IRCBOT_FSERVER_H
/*
 *  Created on: 25 July 2015
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oaskivvy@gmail.com                     |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

#include <skivvy/ircbot.h>

#include <sookee/types/basic.h>
#include <sookee/socketstream.h>

#include <skivvy/store.h>

namespace skivvy { namespace ircbot {

using namespace sookee::net;
using namespace sookee::types;
using namespace skivvy::utils;

const str LOGI = "I: ";
const str LOGW = "W: ";
const str LOGE = "E: ";
const str LOGX = "X: ";

template<typename ValueType>
class synchronized_queue
{
public:
	using queue_type = std::deque<ValueType>;
	using vector_type = std::vector<ValueType>;
	using size_type = typename queue_type::size_type;
	using value_type = typename queue_type::value_type;

private:
	std::mutex mtx;
//	unique_lock lock;
	size_type max_size = 0;
	queue_type data;
	std::condition_variable cv;
	bool done = false;

	friend void swap(synchronized_queue& q1, synchronized_queue& q2)
	{
		std::swap(q1.max_size, q2.max_size);
		std::swap(q1.data, q2.data);
		std::swap(q1.done, q2.done);
	}

public:
	synchronized_queue(size_type max_size = 0)
	: max_size(max_size)
	, done(true)
	{
	}

	synchronized_queue(synchronized_queue&& q)
	: synchronized_queue()
	{
		lock_guard lock(q.mtx);
		swap(*this, q);
	}

	synchronized_queue& operator=(synchronized_queue&& q)
	{
		swap(*this, q);
		return *this;
	}

	std::mutex& get_mtx() { return mtx; }
	queue_type& get_data() { return data; }

	synchronized_queue(const synchronized_queue& q) = delete;

	// unsynchronized
	value_type& front() { return data.front(); }
	value_type const& front() const { return data.front(); }
	value_type& back() { return data.back(); }
	value_type const& back() const { return data.back(); }

	size_type size() { lock_guard lock(mtx); return data.size(); }
	size_type capacity() { lock_guard lock(mtx); return max_size; }
	size_type empty() { lock_guard lock(mtx); return data.empty(); }

	vector_type get_vector()
	{
//		bug_func();
		std::unique_lock<std::mutex> lock(mtx);
		return {data.begin(), data.end()};
	}

	bool find(const value_type& v, size_type& pos)
	{
		bug_func();
		bug_var(v);
		std::unique_lock<std::mutex> lock(mtx);
		for(pos = 0; pos < data.size(); ++pos)
			if(data[pos] == v)
				return true;
		return false;
	}

	bool erase(const value_type& v)
	{
		bug_func();
		bug_var(v);
		std::unique_lock<std::mutex> lock(mtx);
		auto found = std::find(data.begin(), data.end(), v);
		if(found == data.end())
			return false;
		data.erase(found);
		return true;
	}

	bool push_back(ValueType v)
	{
		bug_func();
		std::unique_lock<std::mutex> lock(mtx);
		if(max_size && data.size() >= max_size)
			return false;
		data.push_back(std::move(v));
		lock.unlock();
		cv.notify_one();
		return true;
	}

	bool push_back_the_pop_front_from(synchronized_queue& q)
	{
//		bug_func();
		value_type v;
		if(!q.pop_front(v))
			return false;
		return push_back(std::move(v));
	}

	void open()
	{
		bug_func();
		std::unique_lock<std::mutex> lock(mtx);
		done = false;
	}

	void close()
	{
		bug_func();
		std::unique_lock<std::mutex> lock(mtx);
		done = true;
		lock.unlock();
		cv.notify_one();
	}

	bool active()
	{
		bug_func();
		std::unique_lock<std::mutex> lock(mtx);
		return !done;
	}

	bool pop_front(ValueType& v)
	{
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this]()
		{	return done || !data.empty();});
		if(data.empty())
			return false;
		v = std::move(data.front());
		data.pop_front();
		return true;
	}
};

class FServerIrcBotPlugin
: public ManagedIrcBotPlugin
, public IrcBotMonitor
{
private:

	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;
	using uint64 = std::uint64_t;

	static uint32 next_id()
	{
		static uint32 id = 0;
		static std::mutex mtx;
		lock_guard lock(mtx);
		return ++id;
	}

	BackupStore store;

	struct entry
	{
		message msg;
		str pathname;
		str userhost;
		uint32 fid = 0;
		uint64 size = 0; // 0 -> not started
		uint64 sent = 0; // size == sent -> done
		bool error = false;
		std::future<void> fut;

		friend void swap(entry& a, entry& b)
		{
			std::swap(a.msg, b.msg);
			std::swap(a.pathname, b.pathname);
			std::swap(a.userhost, b.userhost);
			std::swap(a.fid, b.fid);
			std::swap(a.size, b.size);
			std::swap(a.sent, b.sent);
			std::swap(a.error, b.error);
			std::swap(a.fut, b.fut);
		}

		entry() {};

		entry(entry&& e) noexcept
		: entry()
		{
			swap(*this, e);
		}

		entry& operator=(entry&& e)
		{
			swap(*this, e);
			return *this;
		}
	};

//	std::mutex entries_mtx;
//	std::map<uint32, entry> entries;

	uns get_userhost_entries(const str& userhost)
	{
		uns count = 0;

		lock_guard lock (wait_q.get_mtx());
		for(auto&& p: wait_q.get_data())
			if(p.userhost == userhost)
				++count;

		return count;
	}

	synchronized_queue<entry> wait_q {10};
	synchronized_queue<entry> send_q {2};

	std::atomic_bool done {false};
	std::future<void> process_queues_fut;
	void process_queues();
	void end_future(entry& e);

	bool have_zip(const str& data_dir, const str& name); // no .ext
	bool have_txt(const str& data_dir, const str& name); // no .ext

	void request_file(const message& msg, const str& filename);

	bool files(const message& msg);
	bool serve(const message& msg);

	str ntoa(uint32 ip) const;
//	void dcc_server(const str& pathname);
	void dcc_send_file_by_id(uint32 ip, uint32 port, uint32 fid);

public:
	FServerIrcBotPlugin(IrcBot& bot);
	virtual ~FServerIrcBotPlugin();

	// OPTIONAL INTERFACE: IrcBotPlugin
	str_vec api(unsigned call, const str_vec& args = {}) override;

	// INTERFACE: BasicIrcBotPlugin
	bool initialize() override;

	// INTERFACE: IrcBotPlugin
	str get_id() const override;
	str get_name() const override;
	str get_version() const override;
	void exit() override;

	// INTERFACE: IrcBotMonitor
	void event(const message& msg) override;
};

}} // skivvy::ircbot

#endif // SKIVVY_IRCBOT_FSERVER_H
