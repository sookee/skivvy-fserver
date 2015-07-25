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

#include <skivvy/store.h>

namespace skivvy { namespace ircbot {

using namespace sookee::types;
using namespace skivvy::email;
using namespace skivvy::utils;

class FServerIrcBotPlugin
: public BasicIrcBotPlugin
, public IrcBotMonitor
{
private:

	BackupStore store;

	bool serve(const message& msg);

public:
	FServerIrcBotPlugin(IrcBot& bot);
	virtual ~FServerIrcBotPlugin();

	// Plugin API
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
