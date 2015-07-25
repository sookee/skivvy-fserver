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

#include <skivvy/plugin-fserver.h>

#include <sookee/types/basic.h>

#include <skivvy/logrep.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(FServerIrcBotPlugin);
PLUGIN_INFO("fserver", "File Server", "0.1-beta");

using namespace skivvy;
using namespace sookee;
using namespace sookee::types;
using namespace skivvy::utils;
using namespace sookee::utils;

const str STORE_FILE = "fserver.store.file";
const str STORE_FILE_DEFAULT = "fserver-store.txt";

FServerIrcBotPlugin::FServerIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
, store(bot.getf(STORE_FILE, STORE_FILE_DEFAULT))
{
	smtp.mailfrom = "<noreply@sookee.dyndns.org>";
}

FServerIrcBotPlugin::~FServerIrcBotPlugin() {}

bool FServerIrcBotPlugin::serve(const message& msg)
{
	bot.fc_reply(msg, "This is a test.");
	return true;
}

// INTERFACE: BasicIrcBotPlugin

bool FServerIrcBotPlugin::initialize()
{
	bug_func();
	add
	({
		"!" + bot.nick
		, "!" + bot.nick + " - fserver parameter stuff"
		, [&](const message& msg){ serve(msg); }
	});
	bot.add_monitor(*this);
	return true;
}

// INTERFACE: IrcBotPlugin

std::string FServerIrcBotPlugin::get_id() const { return ID; }
std::string FServerIrcBotPlugin::get_name() const { return NAME; }
std::string FServerIrcBotPlugin::get_version() const { return VERSION; }

void FServerIrcBotPlugin::exit()
{
//	bug_func();
}

// INTERFACE: IrcBotMonitor
void FServerIrcBotPlugin::event(const message& msg)
{
}
