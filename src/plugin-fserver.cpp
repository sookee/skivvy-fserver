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

bool FServerIrcBotPlugin::have_txt(const str& data_dir, const str& name)
{
	str txtname = name + ".txt";
	bug_var(txtname);

	if(std::ifstream(txtname).is_open())
		return true;

	log("INFO: Text file not found recreating: " << txtname);

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

	log("ERROR: failed to create text index: " << txtname);

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
		log("ERROR: unable to create zip file: " << zipname);
		log("     : code: " << error);
		log("     : " << zip_strerror(zf));
		return false;
	}

	str txtname = name + ".txt";
	zip_source* zs = zip_source_file(zf, txtname.c_str(), 0, 0);

	if(!zs)
	{
		log("ERROR: unable to create zip source: " << txtname);
		log("     : " << zip_strerror(zf));
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
		log("ERROR: unable to add file: " << txtname << " to " << zipname);
		log("     : " << zip_strerror(zf));
		zip_source_free(zs);
		zip_close(zf);
		return false;
	}

	if(zip_close(zf))
	{
		log("ERROR: unable to create zip file: " << zipname);
		log("     : " << zip_strerror(zf));
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

void FServerIrcBotPlugin::dcc_server(uint32 ip, uint32 port, uint32 fid)
{
	bug_func();
	bug_var(ip);
	bug_var(port);
	bug_var(fid);

	entry e;
	{
		lock_guard lock(entry_mtx);
		auto found = entries.find(fid);
		if(found == entries.end())
		{
			log("ERROR: file id not found: " << fid);
			return;
		}
		e = std::move(found->second);
		entries.erase(found);
	}

//	sockaddr_in serv_addr;
//
//	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//
//	if(sockfd < 0)
//	{
//		log("ERROR: " << strerror(errno));
//		return;
//	}
//
//	bzero((char*) &serv_addr, sizeof(serv_addr));
//
//	serv_addr.sin_family = AF_INET;
//	serv_addr.sin_addr.s_addr = ip;
//	serv_addr.sin_port = htons(port);
//
//	if(connect(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
//	{
//		log("ERROR: " << strerror(errno));
//		return;
//	}
//
//	int len;
//	char buf[1024];
//
//	std::ifstream ifs(e.pathname, std::ios::binary);
//
//	if(!ifs)
//	{
//		log("ERROR: opening file to send: " << e.pathname);
//		return;
//	}
//
//	while(ifs.read(buf, sizeof(buf) && ifs.gcount()))
//	{
//		if((len = write(sockfd, buf, ifs.gcount())) < 0)
//		{
//			log("ERROR: " << strerror(errno));
//			return;
//		}
//	}
//
//	close(sockfd);

	str host = ntoa(ip);

	netstream ns;
	ns.open(host, std::to_string(port));

	if(!ns)
	{
		log("ERROR: opening connection to: " << host << ":" << port);
		return;
	}

	std::ifstream ifs(e.pathname, std::ios::binary);

	if(!ifs)
	{
		log("ERROR: opening file to send: " << e.pathname);
		return;
	}

	ns << ifs.rdbuf();// << std::flush;
//	ns.close();
}

//void FServerIrcBotPlugin::dcc_server(const str& pathname)
//{
//	uint16_t portno = 0;
//
//	sockaddr_in serv_addr;
//	sockaddr_in cli_addr;
//
//	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//
//	if(sockfd < 0)
//	{
//		log("ERROR: " << strerror(errno));
//		return;
//	}
//
//	bzero((char*) &serv_addr, sizeof(serv_addr));
//
//	serv_addr.sin_family = AF_INET;
//	serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
//	serv_addr.sin_port = htons(portno);
//
//	if(bind(sockfd, (sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
//	{
//		log("ERROR: " << strerror(errno));
//		return;
//	}
//
//	if(listen(sockfd, 5))
//	{
//		log("ERROR: " << strerror(errno));
//		return;
//	}
//
//	socklen_t clilen = sizeof(cli_addr);
//
//	int newsockfd = accept(sockfd, (sockaddr*) &cli_addr, &clilen);
//
//	close(sockfd);
//
//	if(newsockfd < 0)
//	{
//		log("ERROR: " << strerror(errno));
//		return;
//	}
//
//	int len;
//	char buf[1024];
//
//	std::ifstream ifs(pathname, std::ios::binary);
//
//	while(ifs.read(buf, sizeof(buf) && ifs.gcount()))
//	{
//		if((len = write(newsockfd, buf, ifs.gcount())) < 0)
//		{
//			log("ERROR: " << strerror(errno));
//			return;
//		}
//	}
//
//	close(newsockfd);
//}
//FServerIrcBotPlugin::uint32 FServerIrcBotPlugin::id = 0;

bool FServerIrcBotPlugin::files(const message& msg)
{
	BUG_COMMAND(msg);
	bot.fc_reply(msg, "This is a test.");
	// make zip file
	// ommo-ebooks-YYYY-MM-DD.txt
	str data_dir = bot.get("fserver.data.dir", bot.get_data_folder());
	bug_var(data_dir);
	str fname = data_dir;
	fname += "/" + bot.get("fserver.index.file.prefix", bot.nick + "-files");
	fname += "-" + todays_date();

	if(!have_zip(data_dir, fname))
		return false;

	{
		auto id = next_id();
		lock_guard lock(entry_mtx);
		entries[id].pathname = fname + ".zip";
		q.push_back(id);
		soss oss;
		// Request Accepted • List Has Been Placed In The Priority Queue At Position 1 • OmeNServE v2.60 •
		oss << "Request Accepted • Priority Queue Position";
		oss << " " << q.size();
		oss << " • " << bot.nick << "'s " << get_name() << " " << get_version();
		oss << " •";
		bot.fc_reply_pm_notice(msg, oss.str());
	}

	// TODO: move this to queue processing

	decltype(q)::value_type fid = 0;

	{
		lock_guard lock(entry_mtx);
		if(q.empty())
			return true;
		fid = q.front();
		q.pop_front();
	}

	fname += ".zip";
	bug_var(fname);
	str zipname = get_filename_from_pathname(fname);
	bug_var(zipname);

	auto ip = INADDR_LOOPBACK;//htonl(INADDR_LOOPBACK);//dcc_get_my_address();
	decltype(htonl(0)) port = 0;

//	decltype(stat::st_size) fsize = 0;

	struct stat s;
	if(stat(fname.c_str(), &s))
	{
		log("ERROR: can not stat zip file: " << fname);
	}

	str address = "127.0.0.1";

	// "DCC SEND %s %u %d %u %d"
	// , dcc->file
	// , cc->addr // the 32bit IP number, host byte order
	// , dcc->port
	// , dcc->size
	// , dcc->pasvid

	soss oss;
	oss << "DCC Send " << zipname << " (" << address << ")";
	bot.fc_reply_pm_notice(msg, oss.str());

	oss.str("");
	oss << '\001';
	oss << "DCC SEND";
	oss << ' ' << zipname;
	oss << ' ' << ip; // the 32bit IP number, host byte order
	oss << ' ' << port;
	oss << ' ' << s.st_size;
	oss << ' ' << fid; // sequence id?
	oss << '\001';
	bot.fc_reply_pm(msg, oss.str());

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
		  "files" // trigger
		, "@ammo" // default alias
		, "<no help found>" // default help
		, [&](const message& msg){ files(msg); }
	);
	add
	(
		  "serve"
		, "!ammo"
		, "<no help found>"
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
		log("ERROR: Failed to parse DCC SEND: " << dcc);
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
		dcc_server(ntohl(ip), port, fid);
	}
	catch(...)
	{
		log("ERROR: bad DCC SEND format: " << text);
		return;
	}

}

}} // skivvy::ircbot

