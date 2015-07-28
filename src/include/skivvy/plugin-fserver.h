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
		str pathname;
		std::future<void> fut;
	};

	std::mutex entry_mtx;
	std::map<uint32, entry> entries;
	std::deque<uint32> q;

	bool have_zip(const str& data_dir, const str& name); // no .ext
	bool have_txt(const str& data_dir, const str& name); // no .ext

	bool files(const message& msg);
	bool serve(const message& msg);

	str ntoa(uint32 ip) const;
//	void dcc_server(const str& pathname);
	void dcc_server(uint32 ip, uint32 port, uint32 fid);

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
