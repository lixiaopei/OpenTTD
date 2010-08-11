/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file airport_sl.cpp Code handling saving and loading airport ids */

#include "../stdafx.h"

#include "saveload.h"
#include "newgrf_sl.h"

static void Save_APID()
{
	Save_NewGRFMapping(_airport_mngr);
}

static void Load_APID()
{
	Load_NewGRFMapping(_airport_mngr);
}

extern const ChunkHandler _airport_chunk_handlers[] = {
	{ 'APID', Save_APID, Load_APID, NULL, NULL, CH_ARRAY | CH_LAST },
};
