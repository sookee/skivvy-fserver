/*
 *  Created on: 25 July 2015
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2015 SooKee oaskivvy@gmail.com                     |
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

#include <zip.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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

bool FServerIrcBotPlugin::have_txt(const str& data_dir, const str& name)
{
	str txtname = name + ".txt";
	bug_var(txtname);

	if(std::ifstream(txtname).is_open())
		return true;

	log("I: Text file not found recreating: " << txtname);

	// create file
	str_set exts;
	{
		auto extvec = bot.get_vec("fserver.data.ext");
		for(auto&& ext: extvec)
		{
			lower(ext);
			bug_var(ext);
		}
		exts.insert(extvec.begin(), extvec.end());
	}

	std::ofstream ofs(txtname);
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

	if(std::ifstream(txtname).is_open())
		return true;

	log("E:: failed to create text index: " << txtname);

	return false;
}

bool FServerIrcBotPlugin::have_zip(const str& data_dir, const str& name)
{
	str zipname = name + ".zip";
	bug_var(zipname);

	if(std::ifstream(zipname).is_open())
		return true;

	// recreate

	if(!have_txt(data_dir, name))
		return false;

	// zip up txt
	int error = 0;
	zip* zf = zip_open(zipname.c_str(), ZIP_CREATE, &error);
	if(!zf)
	{
		log("E: unable to create zip file: " << zipname);
		log(" : code: " << error);
		log(" : " << zip_strerror(zf));
		return false;
	}

	str txtname = name + ".txt";
	zip_source* zs = zip_source_file(zf, txtname.c_str(), 0, 0);

	if(!zs)
	{
		log("E: unable to create zip source: " << txtname);
		log(" : " << zip_strerror(zf));
		zip_close(zf);
		return false;
	}

	auto pos = txtname.find_last_of('/');
	if(pos != str::npos)
	{
		txtname = txtname.substr(pos + 1);
	}

	if(zip_file_add(zf, txtname.c_str(), zs, ZIP_FL_OVERWRITE) == -1)
	{
		log("E: unable to add file: " << txtname << " to " << zipname);
		log(" : " << zip_strerror(zf));
		zip_source_free(zs);
		zip_close(zf);
		return false;
	}

	if(zip_close(zf))
	{
		log("E: unable to create zip file: " << zipname);
		log(" : " << zip_strerror(zf));
		return false;
	}

	return true;
}

std::uint32_t dcc_get_my_address()
{
	hostent* dns_query;
	std::uint32_t addr = 0;

   dns_query = gethostbyname("localhost");

   if (dns_query != NULL &&
	   dns_query->h_length == 4 &&
	   dns_query->h_addr_list[0] != NULL)
	{
		/*we're offered at least one IPv4 address: we take the first*/
		addr = *((std::uint32_t*) dns_query->h_addr_list[0]);
	}

	return addr;
}

str get_filename_from_pathname(const str& pathname)
{
	auto pos = pathname.find_last_of('/');

	if(pos == pathname.size() - 1)
		throw std::runtime_error("bad pathname: " + pathname);
	else if(pos != str::npos)
		return pathname.substr(pos + 1);
	return pathname;
}

// network code

void* get_in_addr(sockaddr* sa)
{
	if(sa->sa_family == AF_INET)
		return &(((sockaddr_in*) sa)->sin_addr);
	return &(((sockaddr_in6*) sa)->sin6_addr);
}

str get_addr(sockaddr_storage& ss)
{
	char ip[INET6_ADDRSTRLEN];
	return inet_ntop(ss.ss_family, get_in_addr((sockaddr*) &ss), ip, INET6_ADDRSTRLEN);
}

str FServerIrcBotPlugin::ntoa(uint32 ip) const
{
	in_addr ip_addr;
	ip_addr.s_addr = ip;
	return {inet_ntoa(ip_addr)};
}

void FServerIrcBotPlugin::end_future(entry& e)
{
//	bug_func();
//	bug_var(fid);
//
//	if(!send_q.erase(fid))
//		log("E: erasing from send queue(missing) [" << fid << "]");
//
	if(e.fut.valid())
	{
		auto status = e.fut.wait_for(std::chrono::seconds(3));
		if(status == std::future_status::ready)
			e.fut.get();
		else
		{
			log("E: waiting for transfer thread to die [" << e.fid << "]");
		}
	}
}

void FServerIrcBotPlugin::process_queues()
{
	while(!done.load())
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		// tidy queues
//		decltype(entries.begin()) found;
		{
			lock_guard lock(send_q.get_mtx());

			for(auto e = send_q.get_data().begin(); e != send_q.get_data().end();)
			{
				bug_var(e->fid);
				bug_var(e->size);
				bug_var(e->sent);
				bug_var(e->error);

				if(e->error)
				{
					log("I: send error: [" << e->fid << "] " << e->userhost << " " << e->pathname);
					end_future(*e);
					e = send_q.get_data().erase(e);
				}
				else if(e->size && e->size == e->sent)
				{
					log("I: file complete: [" << e->fid << "] " << e->userhost << " " << e->pathname);
					end_future(*e);
					e = send_q.get_data().erase(e);
				}
				else
					++e;
			}
		}

		// check send_q for spaces & fill from wait_q

//		decltype(wait_q)::value_type fid = 0;
//		decltype(entries.begin()) found;

		if(send_q.size() < send_q.capacity()
		&& send_q.push_back_the_pop_front_from(wait_q))
		{
			lock_guard lock(send_q.get_mtx());

			str pathname = send_q.back().pathname;
			bug_var(pathname);
			str filename = get_filename_from_pathname(pathname);
			bug_var(filename);
			str quote;
			if(filename.find(' ') != str::npos)
				quote = "\"";

			auto ip = INADDR_LOOPBACK;//htonl(INADDR_LOOPBACK);//dcc_get_my_address();
			decltype(htonl(0)) port = 0;

			struct stat s;
			if(stat(pathname.c_str(), &s))
			{
				log("E: can not stat zip file: " << pathname);
			}

			str address = "127.0.0.1";

			soss oss;
			oss << "DCC Send " << quote << filename << quote << " (" << address << ")";
			bot.fc_reply_pm_notice(send_q.back().msg, oss.str());

			oss.str("");
			oss << '\001';
			oss << "DCC SEND";
			oss << ' ' << quote << filename << quote;
			oss << ' ' << ip; // the 32bit IP number, host byte order
			oss << ' ' << port;
			oss << ' ' << s.st_size;
			oss << ' ' << send_q.back().fid; // sequence id?
			oss << '\001';
			bot.fc_reply_pm(send_q.back().msg, oss.str());
		}
	}
}

void FServerIrcBotPlugin::dcc_send_file_by_id(uint32 ip, uint32 port, uint32 fid)
{
	bug_func();
	bug_var(ip);
	bug_var(port);
	bug_var(fid);

	netstream ns;
	std::ifstream ifs;

	auto found = send_q.get_data().end();

	str userhost;

	{
		lock_guard lock(send_q.get_mtx());

		for(auto e = send_q.get_data().begin(); e != send_q.get_data().end(); ++e)
		{
			if(e->fid != fid)
				continue;

			found = e;
			break;
		}

		if(found == send_q.get_data().end())
		{
			log("E: file id not found: " << fid);
			return;
		}

		userhost = found->userhost;

		str host = ntoa(ip);
		bug_var(host);
		bug_var(found->fid);
		bug_var(found->pathname);
		bug_var(found->userhost);

		ns.open(host, std::to_string(port));

		if(!ns)
		{
			log("E: opening connection to: " << host << ":" << port);
			return;
		}

		ifs.open(found->pathname, std::ios::binary|std::ios::ate);

		if(!ifs)
		{
			log("E: opening file to send: " << found->pathname);
			return;
		}

		found->size = ifs.tellg();
		ifs.seekg(0);
		found->sent = ifs.tellg();

		bug_var((bool)ifs);
		ns << ifs.rdbuf();
		found->sent = ifs.tellg();
	}


//	char buf[1024];
//
//	while(ifs.read(buf, sizeof(buf)) && ifs.gcount())
//	{
//		bug_var(ifs.gcount());
//		if(!ns.write(buf, ifs.gcount()))
//		{
//			log("E: writing to client failed: " << fid);
//			lock_guard lock(entries_mtx);
//			if((found = entries.find(fid)) != entries.end())
//				found->second.error = true;
//			return;
//		}
//
//		lock_guard lock(entries_mtx);
//		if((found = entries.find(fid)) == entries.end())
//		{
//			log("E: file id disappeared (should never happen): " << fid);
//			return;
//		}
//		found->second.sent = ifs.tellg();
//	}

	log("I: File sent to " << userhost);

//	ns << ifs.rdbuf();
}

void FServerIrcBotPlugin::request_file(const message& msg, const str& filename)
{
	log("I: Creating entry for: " << msg.get_userhost());
	auto fid = next_id();
	entry e;
	e.fid = fid;
	e.pathname = filename;
	e.userhost = msg.get_userhost();
	e.size = 0;
	e.sent = 0;
	e.msg = msg;

	log("I: Adding to queue: " << fid);

	soss oss;
	if(!wait_q.push_back(std::move(e)))
	{
		log("I: Wait queue full: " << fid);

		oss << "Request Denied • Priority Queue is FULL!";
		oss << " " << wait_q.size();
		oss << " • " << bot.nick << "'s " << get_name() << " " << get_version();
		oss << " •";
	}
	else
	{
		log("I: In wait queue: " << fid);
		// Request Accepted • List Has Been Placed In The Priority Queue At Position 1 • OmeNServE v2.60 •
		oss << "Request Accepted • Priority Queue Position";
		oss << " " << wait_q.size();
		oss << " • " << bot.nick << "'s " << get_name() << " " << get_version();
		oss << " •";
	}
	bot.fc_reply_pm_notice(msg, oss.str());
}

bool FServerIrcBotPlugin::files(const message& msg)
{
	BUG_COMMAND(msg);
	if(done.load())
	{
		bot.fc_reply_pm_notice(msg, "Declined, server shutting down");
		return true;
	}
	// make zip file
	// ommo-ebooks-YYYY-MM-DD.txt
	str data_dir = bot.get("fserver.data.dir", bot.get_data_folder());
	bug_var(data_dir);
	str fname = data_dir;
	fname += "/" + bot.get("fserver.index.file.prefix", bot.nick + "-files");
	fname += "-" + todays_date();

	if(!have_zip(data_dir, fname))
		return false;

	request_file(msg, fname + ".zip");

	return true;
}

bool FServerIrcBotPlugin::serve(const message& msg)
{
	BUG_COMMAND(msg);
	//---------------------------------------------------
	// get_nickname()       : Lelly
	// get_user()           : ~lelly
	// get_host()           : wibble.wobble
	// get_userhost()       : ~lelly@wibble.wobble
	// get_nick()           : Lelly
	// get_chan()           : #ommo
	// get_user_cmd()       : !ommo
	// get_user_params()    : Test #1.zip
	//---------------------------------------------------
	if(done.load())
	{
		bot.fc_reply_pm_notice(msg, "Declined, server shutting down");
		return true;
	}
	bot.fc_reply(msg, "This is a test.");

	// !ommo Test #1.zip
	str data_dir = bot.get("fserver.data.dir", bot.get_data_folder());
	bug_var(data_dir);
	request_file(msg, data_dir + '/' + msg.get_user_params());

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

	log("I: Starting queue processing");
	process_queues_fut = std::async(std::launch::async, [&]{process_queues();});

	add
	(
		  "files" // trigger
		, "@ammo" // default alias
		, "<no help found>" // default help
		, [&](const message& msg){ files(msg); }
	);
	add
	(
		  "serve"
		, "!ammo"
		, "<filename> - download specific file."
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
	bug_func();

	// stop queue processing
	log("I: Stopping queue procssing");
	done.store(true);

	// wait for send queue to complete (with timeout)
	log("I: Waiting for send queue to complete");
	if(process_queues_fut.valid())
	{
		auto status = process_queues_fut.wait_for(std::chrono::seconds(30));
		if(status == std::future_status::ready)
			process_queues_fut.get();
		else
		{
			log("E: time-out for queue processing thread to die");
		}
	}

	// notify wait queue of termination
	log("I: Notifying wait queue of termination");
	lock_guard lock(wait_q.get_mtx());

//	uint32 fid;
//	decltype(entries.begin()) found;
//
//	while(!wait_q.empty())
//	{
//		if(wait_q.pop_front(fid))
//		{
//			if((found = entries.find(fid)) != entries.end())
//			{
//				bot.fc_reply_pm_notice(found->second.msg, "DCC removed from queue: server shutting down.");
//				entries.erase(found);
//			}
//		}
//	}
//
//	// wait for transfers to complete
//	log("I: Waiting for transfers to complete");
//
//	st_clk::time_point timeout = st_clk::now() + std::chrono::minutes(5);
//	uint32 count = 1;
//	while(count && st_clk::now() < timeout)
//	{
//		count = 0;
//		uint64 size = 0;
//		uint64 sent = 0;
//		for(auto&& p: entries)
//		{
//			if(!p.second.size || p.second.size == p.second.sent)
//				continue;
//
//			size += p.second.size;
//			sent += p.second.sent;
//			++count;
//		}
//		log("I: Transfers: [" << count << "] " << (sent * 100 / size) << "% complete");
//		std::this_thread::sleep_for(std::chrono::seconds(3));
//	}
//
//	if(count)
//	{
//		log("E: times out waiting for transfers disconnecting them:");
//		for(auto&& p: entries)
//			if(p.second.size && p.second.size != p.second.sent)
//				bot.fc_reply_pm_notice(found->second.msg, "DCC aborting transfer: server shutting down.");
//	}
//
//	entries.clear();
}

// INTERFACE: IrcBotMonitor
void FServerIrcBotPlugin::event(const message& msg)
{
	if(msg.command != "PRIVMSG")
		return;
	BUG_COMMAND(msg);
	// :Lelly!~lelly@wibble.wobble PRIVMSG oMMo :DCC SEND oMMo-files-2015-07-28.zip 1365602080 6660 229 1

//	str userhost = msg.get_userhost();
//	for(const str& s: ignores)
//		if(userhost.find(s) != str::npos)
//			return;

	str text = msg.get_trailing();
	trim(text, "\001");
	// DCC SEND oMMo-files-2015-07-28.zip <ip> <port> <fsize> <fid>
	// <fid> = id of offered file
	if(text.size() < 9)
		return; // malformed

	str dcc = text.substr(9);
	std::reverse(dcc.begin(), dcc.end());

	str s_fid;
	str s_fsize;
	str s_port;
	str s_ip;
	str fname;

	if(!sgl(siss(dcc) >> s_fid >> s_fsize >> s_port >> s_ip >> std::ws, fname))
	{
		log("E: Failed to parse DCC SEND: " << dcc);
		return;
	}

	std::reverse(s_fid.begin(), s_fid.end());
	std::reverse(s_fsize.begin(), s_fsize.end());
	std::reverse(s_port.begin(), s_port.end());
	std::reverse(s_ip.begin(), s_ip.end());
	std::reverse(fname.begin(), fname.end());

	try
	{
		uint32 fid = std::stol(s_fid);
//		uint64 fsize = std::stol(s_fsize);
		uint32 port = std::stol(s_port);
		uint32 ip = std::stol(s_ip);
		dcc_send_file_by_id(ntohl(ip), port, fid);
	}
	catch(...)
	{
		log("E: bad DCC SEND format: " << text);
		return;
	}

}

}} // skivvy::ircbot

