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
#include <sookee/cal.h>
#include <skivvy/utils.h>
#include <skivvy/logrep.h>

#include <fstream>
#include <iomanip>

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
: ManagedIrcBotPlugin(bot)
, store(bot.getf(STORE_FILE, STORE_FILE_DEFAULT))
{
}

FServerIrcBotPlugin::~FServerIrcBotPlugin() {}

str todays_date()
{
	str y = std::to_string(cal::get_year());
	str m = std::to_string(cal::get_month() + 1);
	str d = std::to_string(cal::get_date());
	return y + '-' + (m.size()<2?"0":"") + m + '-' + (d.size()<2?"0":"") + d;
}

class CTCP
{
	bool is_l_char(char c)
	{
		static const str nl = "\000\012\015";
		return nl.find(c) == str::npos;
	}
	const char M_QUOTE = '\020';
	const char M_DELIM = '\001';
	const str Q_NULL = M_QUOTE + "0";
	const str Q_NL = M_QUOTE + "n";
	const str Q_CR = M_QUOTE + "r";
	const str Q_QUOTE = str() + M_QUOTE + M_QUOTE;

	str m_quote(const str& s)
	{
		str u;

		for(char c: s)
		{
			if(c == '\0')
				u += Q_NULL;
			else if(c == '\n')
				u += Q_NL;
			else if(c == '\r')
				u += Q_CR;
			else if(c == M_QUOTE)
				u += Q_QUOTE;
			else
				u += c;
		}

		return u;
	}

	str un_m_quote(const str& s)
	{
		str u;

		bool quote = false;
		for(char c: s)
		{
			if(quote)
			{
				if(c == '0')
					c = '\0';
				else if(c == 'n')
					c = '\n';
				else if(c == 'r')
					c = '\r';
				else if(c != M_QUOTE)
					log("ERROR: CTCP bad quote");
				quote = false;
			}
			else if(c == M_QUOTE)
			{
				quote = true;
				continue;
			}

			u += c;
		}

		return u;
	}

public:

//	parse_line()
};

bool FServerIrcBotPlugin::files(const message& msg)
{
	BUG_COMMAND(msg);
	bot.fc_reply(msg, "This is a test.");
	// make zip file
	// ommo-ebooks-YYYY-MM-DD.txt
	str data_dir = bot.get("fserver.data.dir", bot.get_data_folder());
	bug_var(data_dir);
	str fname = data_dir;
	fname += "/" + bot.get("fserver.index.file.prefix", "ommo-ebooks");
	fname += "-" + todays_date();
	fname += ".txt";

	bug_var(fname);

	std::ifstream ifs;
	ifs.open(fname);
	if(!ifs)
	{
		// create file
		str_set exts;
		{
			auto extvec = bot.get_vec("fserver.data.ext");
			for(auto& ext: extvec)
			{
				lower(ext);
				bug_var(ext);
			}
			exts.insert(extvec.begin(), extvec.end());
		}
		std::ofstream ofs(fname);
		for(auto& f: ios::ls(data_dir, ios::ftype::reg))
		{
			bug_var(f);
			auto pos = f.find_last_of('.');
			bug_var(pos);
			if(pos == str::npos || pos == f.size() - 1)
				continue;
			if(!exts.count(lower_copy(f.substr(pos + 1))))
				continue;
			bug("adding: " << f);
			struct stat s;
			if(!stat((data_dir + '/' + f).c_str(), &s))
			{
				bug_var(s.st_size);
				// !pondering filename.rar  ::INFO:: 120.0KB
				str units = "B";
				float fsize = s.st_size;
				if(s.st_size / (1000 * 1000 * 1000))
				{
					units = "GB";
					fsize /= (1000 * 1000 * 1000);
				}
				else if(s.st_size / (1000 * 1000))
				{
					units = "MB";
					fsize /= (1000 * 1000);
				}
				else if(s.st_size / 1000)
				{
					units = "KB";
					fsize /= 1024;
				}
				bug_var(units);
				bug_var(fsize);
				ofs << std::fixed;
				ofs << std::setprecision(0);
				if(units != "B")
					ofs << std::setprecision(1);
				ofs << "!ommo " << f << " ::INFO:: " << fsize << units << '\n';
			}
		}
		ofs.close();
		ifs.clear();
		ifs.open(fname);
		if(!ifs)
		{
			log("ERROR: failed to create index");
			return true;
		}
	}

	//irc->ctcp(msg.reply_to(), ""); // send file
	return true;
}

bool FServerIrcBotPlugin::serve(const message& msg)
{
	BUG_COMMAND(msg);
	bot.fc_reply(msg, "This is a test.");
	return true;
}

// INTERFACE: BasicIrcBotPlugin

str_vec FServerIrcBotPlugin::api(unsigned call, const str_vec& args)
{
	(void) call;
	(void) args;
	return {};
}

bool FServerIrcBotPlugin::initialize()
{
	bug_func();
	add
	(
		  "files"
		, "@ammo"
		, "<no help found>"
//		  bot.get("fserver.trigger.word.files", "@ommo")
//		, bot.get("fserver.trigger.help.files", "<no help found>")
		, [&](const message& msg){ files(msg); }
	);
	add
	(
		  "serve"
		, "!ammo"
		, "<no help found>"
//		  bot.get("fserver.trigger.word.serve", "!ommo")
//		, bot.get("fserver.trigger.help.serve", "<no help found>")
		, [&](const message& msg){ serve(msg); }
	);
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
	(void) msg;
}

}} // skivvy::ircbot

