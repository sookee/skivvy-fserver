

# /*-----------------------------------------------------------------.
# | Copyright (C) 2015 SooKee oaskivvy@gmail.com                     |
# '------------------------------------------------------------------'
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
# 
# http://www.gnu.org/licenses/gpl-2.0.html
# 
# '-----------------------------------------------------------------*/

AC_INIT([skivvy-fserver],[0.1],[oaskivvy@gmail.com],[skivvy-fserver])
AC_CONFIG_AUX_DIR([aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/plugin-fserver.cpp])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.10 foreign])
AM_SILENT_RULES
AM_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_PROG_CXX
AC_PROG_INSTALL
AC_LANG_CPLUSPLUS

AX_CXX_COMPILE_STDCXX_14([noext],[mandatory])

PKG_CHECK_MODULES([SOOKEE], [libsookee])
PKG_CHECK_MODULES([SKIVVY], [libskivvy])
PKG_CHECK_MODULES([LIBZIP], [libzip])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
