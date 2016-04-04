/*
 *  Created on: 4 April 2016
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2016 SooKee oaskivvy@gmail.com                     |
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

#include <skivvy/plugin-fserver.h>

#include <fstream>
#include <iomanip>

#include <zip.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sookee/types/basic.h>
#include <sookee/cal.h>
#include <sookee/scoper.h>
#include <sookee/rnd.h>
#include <skivvy/utils.h>
#include <skivvy/logrep.h>

using namespace skivvy;
using namespace sookee;
using namespace sookee::types;
using namespace skivvy::utils;
using namespace sookee::utils;

using namespace skivvy::ircbot;

std::mutex mtx;
rnd::PRNG_32U prng;

void push_back_test(synchronized_queue<int>& q, const int_vec& v)
{
	for(auto i = 0U; i < 10000; ++i)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(prng.get(0, 10)));
		int n;
		{
//			lock_guard lock(mtx);
			n = i % v.size();
		}
		q.push_back(v[n]);
	}
}

siz pop_front_test(synchronized_queue<int>& q)
{
	siz total = 0;
	for(int v; q.pop_front(v);)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(prng.get(0, 10)));
		total += v;
	}
	return total;
}

void push_back_the_pop_front_from_test(synchronized_queue<int>& q1, synchronized_queue<int>& q2)
{
	while(q2.push_back_the_pop_front_from(q1))
		std::this_thread::sleep_for(std::chrono::milliseconds(prng.get(0, 10)));
}

int main(int, char const* const argv[])
{
	bug_fun();

	siz N = 10;
	bool test_push_back = false;
	bool test_pop_front = false;
	bool test_back_to_front = false;

	for(auto arg = argv + 1; *arg; ++arg)
	{
		if(!std::strcmp(*arg, "--test-push"))
			test_push_back = true;
		else if(!std::strcmp(*arg, "--test-pull"))
			test_pop_front = true;
		else if(!std::strcmp(*arg, "--test-b2f"))
			test_back_to_front = true;
		else if(!std::strcmp(*arg, "--test-all"))
		{
			test_push_back = true;
			test_pop_front = true;
			test_back_to_front = true;
		}
		else if(!std::strcmp(*arg, "-n"))
		{
			if(!*++arg)
			{
				err("E: -n requires a numeric parameter");
				return EXIT_FAILURE;
			}
			N = std::atoll(*arg);
		}
	}

	int_vec v;
	for(auto i = 0U; i < 1000; ++i)
		v.push_back(prng.get(0, 999));

	// push_back test

	siz prev_sum = 0;

	if(test_push_back)
	{
		for(auto times = 0U; times < N; ++times)
		{
			synchronized_queue<int> q;

			std::vector<std::thread> threads;
			for(auto i = 0U; i < 4; ++i)
				threads.emplace_back(push_back_test, std::ref(q), std::ref(v));

			for(auto&& t: threads)
				t.join();

			int i;
			siz sum = 0;
			while(q.pop_front(i))
				sum += i;

			std::cout << "checksum " << (times + 1 < 10?"0":"") << (times + 1) << ": " << sum << '\n';

			if(prev_sum && prev_sum != sum)
				std::cout << "checksum error, previous sum was: " << prev_sum << '\n';

			prev_sum = sum;
		}
	}
	// pop_front test

	if(test_pop_front)
	{
		for(auto times = 0U; times < N; ++times)
		{
			synchronized_queue<int> q;

			for(auto i: v)
				q.push_back(i);

			std::vector<std::future<siz>> threads;
			for(auto i = 0U; i < 4; ++i)
				threads.emplace_back(std::async(std::launch::async, pop_front_test, std::ref(q)));

			siz sum = 0;
			for(auto&& t: threads)
			{
				siz s = t.get();
				std::cout << "s: " << s << '\n';
				sum += s;
			}

			std::cout << "checksum " << (times + 1 < 10?"0":"") << (times + 1) << ": " << sum << '\n';

			if(prev_sum && prev_sum != sum)
				std::cout << "checksum error, previous sum was: " << prev_sum << '\n';

			prev_sum = sum;
		}
	}

	if(test_back_to_front)
	{
//		push_back_the_pop_front_from
		for(auto times = 0U; times < N; ++times)
		{
			synchronized_queue<int> q1;
			synchronized_queue<int> q2;

			for(auto i: v)
				q1.push_back(i);

			std::vector<std::thread> threads;
			for(auto i = 0U; i < 4; ++i)
				threads.emplace_back(push_back_the_pop_front_from_test, std::ref(q1), std::ref(q2));

			for(auto&& t: threads)
				t.join();

			int_vec v1 = v;
			int_vec v2;

			int i;
			siz sum = 0;
			while(q2.pop_front(i))
			{
				v2.push_back(i);
				sum += i;
			}

			std::sort(v1.begin(), v1.end());
			std::sort(v2.begin(), v2.end());

			if(v1 != v2)
			{
				std::cout << "contents error, final vector different from original" << '\n';
				bug_cnt(v);
				bug_cnt(v1);
				bug_cnt(v2);
			}

			std::cout << "checksum " << (times + 1 < 10?"0":"") << (times + 1) << ": " << sum << '\n';

			if(prev_sum && prev_sum != sum)
				std::cout << "checksum error, previous sum was: " << prev_sum << '\n';

			prev_sum = sum;
		}
	}
}
