#include "stdafx.h"
#include "ttd.h"

#include "command.h"
#include "gfx.h"
#include "map.h"
#include "order.h"
#include "saveload.h"
#include "station.h"
#include "tile.h"
#include "town.h"
#include "waypoint.h"
#include "table/sprites.h"
#include "table/strings.h"
#include "table/track_land.h"

enum {
	/* Max waypoints: 64000 (8 * 8000) */
	WAYPOINT_POOL_BLOCK_SIZE_BITS = 3,       /* In bits, so (1 << 3) == 8 */
	WAYPOINT_POOL_MAX_BLOCKS      = 8000,

	MAX_WAYPOINTS_PER_TOWN        = 64,
};

/**
 * Called if a new block is added to the waypoint-pool
 */
static void WaypointPoolNewBlock(uint start_item)
{
	Waypoint *wp;

	FOR_ALL_WAYPOINTS_FROM(wp, start_item)
		wp->index = start_item++;
}

/* Initialize the town-pool */
MemoryPool _waypoint_pool = { "Waypoints", WAYPOINT_POOL_MAX_BLOCKS, WAYPOINT_POOL_BLOCK_SIZE_BITS, sizeof(Waypoint), &WaypointPoolNewBlock, 0, 0, NULL };

/* Create a new waypoint */
Waypoint *AllocateWaypoint(void)
{
	Waypoint *wp;

	FOR_ALL_WAYPOINTS(wp) {
		if (wp->xy == 0) {
			uint index = wp->index;

			memset(wp, 0, sizeof(Waypoint));
			wp->index = index;

			return wp;
		}
	}

	/* Check if we can add a block to the pool */
	if (AddBlockToPool(&_waypoint_pool))
		return AllocateWaypoint();

	return NULL;
}

/* Fetch a waypoint by tile */
Waypoint *GetWaypointByTile(TileIndex tile)
{
	Waypoint *wp;

	FOR_ALL_WAYPOINTS(wp) {
		if (wp->xy == tile)
			return wp;
	}

	return NULL;
}

/* Update the sign for the waypoint */
void UpdateWaypointSign(Waypoint *wp)
{
	Point pt = RemapCoords2(TileX(wp->xy) * 16, TileY(wp->xy) * 16);
	SetDParam(0, wp->index);
	UpdateViewportSignPos(&wp->sign, pt.x, pt.y - 0x20, STR_WAYPOINT_VIEWPORT);
}

/* Redraw the sign of a waypoint */
void RedrawWaypointSign(Waypoint *wp)
{
	MarkAllViewportsDirty(
		wp->sign.left - 6,
		wp->sign.top,
		wp->sign.left + (wp->sign.width_1 << 2) + 12,
		wp->sign.top + 48);
}

/* Update all signs */
void UpdateAllWaypointSigns(void)
{
	Waypoint *wp;

	FOR_ALL_WAYPOINTS(wp) {
		if (wp->xy)
			UpdateWaypointSign(wp);
	}
}

/* Set the default name for a waypoint */
void MakeDefaultWaypointName(Waypoint *wp)
{
	Waypoint *local_wp;
	bool used_waypoint[MAX_WAYPOINTS_PER_TOWN];
	int i;

	wp->town_index = ClosestTownFromTile(wp->xy, (uint)-1)->index;

	memset(used_waypoint, 0, sizeof(used_waypoint));

	/* Find an unused waypoint number belonging to this town */
	FOR_ALL_WAYPOINTS(local_wp) {
		if (wp == local_wp)
			continue;

		if (local_wp->xy && local_wp->string == STR_NULL && local_wp->town_index == wp->town_index)
			used_waypoint[local_wp->town_cn] = true;
	}

	/* Find an empty spot */
	for (i = 0; used_waypoint[i] && i < MAX_WAYPOINTS_PER_TOWN; i++) {}

	wp->string = STR_NULL;
	wp->town_cn = i;
}

/* Find a deleted waypoint close to a tile. */
static Waypoint *FindDeletedWaypointCloseTo(uint tile)
{
	Waypoint *wp, *best = NULL;
	uint thres = 8, cur_dist;

	FOR_ALL_WAYPOINTS(wp) {
		if (wp->deleted && wp->xy) {
			cur_dist = DistanceManhattan(tile, wp->xy);
			if (cur_dist < thres) {
				thres = cur_dist;
				best = wp;
			}
		}
	}

	return best;
}

/* Convert existing rail to waypoint */
int32 CmdBuildTrainWaypoint(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	TileIndex tile = TILE_FROM_XY(x,y);
	Waypoint *wp;
	uint tileh;
	uint dir;

	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);

	if (!IsTileType(tile, MP_RAILWAY) || ((dir = 0, _map5[tile] != 1) && (dir = 1, _map5[tile] != 2)))
		return_cmd_error(STR_1005_NO_SUITABLE_RAILROAD_TRACK);

	if (!CheckTileOwnership(tile))
		return CMD_ERROR;

	if (!EnsureNoVehicle(tile)) return CMD_ERROR;

	tileh = GetTileSlope(tile, NULL);
	if (tileh != 0) {
		if (!_patches.build_on_slopes || tileh & 0x10 || !(tileh & (0x3 << dir)) || !(tileh & ~(0x3 << dir)))
			return_cmd_error(STR_0007_FLAT_LAND_REQUIRED);
	}

	/* Check if there is an already existing, deleted, waypoint close to us that we can reuse. */
	wp = FindDeletedWaypointCloseTo(tile);
	if (wp == NULL) {
		wp = AllocateWaypoint();
		if (wp == NULL)
			return CMD_ERROR;

		wp->town_index = 0;
		wp->string = STR_NULL;
		wp->town_cn = 0;
	}

	if (flags & DC_EXEC) {
		ModifyTile(tile, MP_MAP5, RAIL_TYPE_WAYPOINT | dir);
		if (--p1 & 0x100) { // waypoint type 0 uses default graphics
			// custom graphics
			_map3_lo[tile] |= 16;
			_map3_hi[tile] = p1 & 0xff;
		}

		wp->deleted = 0;
		wp->xy = tile;
		wp->build_date = _date;

		if (wp->town_index == STR_NULL)
			MakeDefaultWaypointName(wp);

		UpdateWaypointSign(wp);
		RedrawWaypointSign(wp);
	}

	return _price.build_train_depot;
}

/* Internal handler to delete a waypoint */
static void DoDeleteWaypoint(Waypoint *wp)
{
	Order order;

	wp->xy = 0;

	order.type = OT_GOTO_WAYPOINT;
	order.station = wp->index;
	DeleteDestinationFromVehicleOrder(order);

	if (wp->string != STR_NULL)
		DeleteName(wp->string);

	RedrawWaypointSign(wp);
}

/* Daily loop for waypoints */
void WaypointsDailyLoop(void)
{
	Waypoint *wp;

	/* Check if we need to delete a waypoint */
	FOR_ALL_WAYPOINTS(wp) {
		if (wp->deleted && !--wp->deleted) {
			DoDeleteWaypoint(wp);
		}
	}
}

/* Remove a waypoint */
int32 RemoveTrainWaypoint(uint tile, uint32 flags, bool justremove)
{
	Waypoint *wp;

	/* Make sure it's a waypoint */
	if (!IsTileType(tile, MP_RAILWAY) || !IsRailWaypoint(_map5[tile]))
		return CMD_ERROR;

	if (!CheckTileOwnership(tile) && !(_current_player == OWNER_WATER))
		return CMD_ERROR;

	if (!EnsureNoVehicle(tile))
		return CMD_ERROR;

	if (flags & DC_EXEC) {
		int direction = _map5[tile] & RAIL_WAYPOINT_TRACK_MASK;

		wp = GetWaypointByTile(tile);

		wp->deleted = 30; // let it live for this many days before we do the actual deletion.
		RedrawWaypointSign(wp);

		if (justremove) {
			ModifyTile(tile, MP_MAP5, 1<<direction);
			_map3_lo[tile] &= ~16;
			_map3_hi[tile] = 0;
		} else {
			DoClearSquare(tile);
			SetSignalsOnBothDir(tile, direction);
		}
	}

	return _price.remove_train_depot;
}

/* Command call to remove a waypoint */
int32 CmdRemoveTrainWaypoint(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	uint tile = TILE_FROM_XY(x,y);
	SET_EXPENSES_TYPE(EXPENSES_CONSTRUCTION);
	return RemoveTrainWaypoint(tile, flags, true);
}

/* Rename a waypoint
 * p1 = id of waypoint */
int32 CmdRenameWaypoint(int x, int y, uint32 flags, uint32 p1, uint32 p2)
{
	Waypoint *wp;
	StringID str;

	if (_decode_parameters[0] != 0) {
		str = AllocateNameUnique((const char*)_decode_parameters, 0);
		if (str == 0)
			return CMD_ERROR;

		if (flags & DC_EXEC) {
			wp = GetWaypoint(p1);
			if (wp->string != STR_NULL)
				DeleteName(wp->string);

			wp->string = str;
			wp->town_cn = 0;

			UpdateWaypointSign(wp);
			MarkWholeScreenDirty();
		} else {
			DeleteName(str);
		}
	}	else {
		if (flags & DC_EXEC) {
			wp = GetWaypoint(p1);
			if (wp->string != STR_NULL)
				DeleteName(wp->string);

			MakeDefaultWaypointName(wp);
			UpdateWaypointSign(wp);
			MarkWholeScreenDirty();
		}
	}
	return 0;
}

/* This hacks together some dummy one-shot Station structure for a waypoint. */
Station *ComposeWaypointStation(uint tile)
{
	Waypoint *wp = GetWaypointByTile(tile);
	static Station stat;

	stat.train_tile = stat.xy = wp->xy;
	stat.town = GetTown(wp->town_index);
	stat.string_id = wp->string == STR_NULL ? /* FIXME? */ 0 : wp->string;
	stat.build_date = wp->build_date;
	stat.class_id = 6;
	stat.stat_id = wp->stat_id;

	return &stat;
}

extern uint16 _custom_sprites_base;

/* Draw a waypoint */
void DrawWaypointSprite(int x, int y, int stat_id, int railtype)
{
	StationSpec *stat;
	uint32 relocation;
	DrawTileSprites *cust;
	DrawTileSeqStruct const *seq;
	uint32 ormod, img;

	ormod = SPRITE_PALETTE(PLAYER_SPRITE_COLOR(_local_player));

	x += 33;
	y += 17;

	/* draw default waypoint graphics of ID 0 */
	if (stat_id == 0) {
		const DrawTrackSeqStruct *dtss = _track_depot_layout_table[4];

		img = dtss++->image;
		if (img & 0x8000) img = (img & 0x7FFF) + railtype*TRACKTYPE_SPRITE_PITCH;
		DrawSprite(img, x, y);

		for (; dtss->image != 0; dtss++) {
			Point pt = RemapCoords(dtss->subcoord_x, dtss->subcoord_y, 0);
			img = dtss->image;
			if (img & 0x8000) img |= ormod;
			DrawSprite(img, x + pt.x, y + pt.y);
		}
		return;
	}

	stat = GetCustomStation(STAT_CLASS_WAYP, stat_id - 1);
	assert(stat);
	relocation = GetCustomStationRelocation(stat, NULL, 1);
	// emulate station tile - open with building
	// add 1 to get the other direction
	cust = &stat->renderdata[2];

	img = cust->ground_sprite;
	img += railtype * ((img < _custom_sprites_base) ? TRACKTYPE_SPRITE_PITCH : 1);

	if (img & 0x8000) img = (img & 0x7FFF);
	DrawSprite(img, x, y);

	foreach_draw_tile_seq(seq, cust->seq) {
		Point pt = RemapCoords(seq->delta_x, seq->delta_y, seq->delta_z);
		uint32 image = seq->image + relocation;

		DrawSprite((image&0x3FFF) | ormod, x + pt.x, y + pt.y);
	}
}

/* Fix savegames which stored waypoints in their old format */
void FixOldWaypoints(void)
{
	Waypoint *wp;

	/* Convert the old 'town_or_string', to 'string' / 'town' / 'town_cn' */
	FOR_ALL_WAYPOINTS(wp) {
		if (wp->xy == 0)
			continue;

		wp->town_index = ClosestTownFromTile(wp->xy, (uint)-1)->index;
		wp->town_cn = 0;
		if (wp->string & 0xC000) {
			wp->town_cn = wp->string & 0x3F;
			wp->string = STR_NULL;
		}
	}
}

void InitializeWaypoints(void)
{
	CleanPool(&_waypoint_pool);
	AddBlockToPool(&_waypoint_pool);
}

static const byte _waypoint_desc[] = {
	SLE_CONDVAR(Waypoint, xy, SLE_FILE_U16 | SLE_VAR_U32, 0, 5),
	SLE_CONDVAR(Waypoint, xy, SLE_UINT32, 6, 255),
	SLE_CONDVAR(Waypoint, town_index, SLE_UINT16, 12, 255),
	SLE_CONDVAR(Waypoint, town_cn, SLE_UINT8, 12, 255),
	SLE_VAR(Waypoint,string,		SLE_UINT16),
	SLE_VAR(Waypoint,deleted,						SLE_UINT8),

	SLE_CONDVAR(Waypoint, build_date, SLE_UINT16, 3, 255),
	SLE_CONDVAR(Waypoint, stat_id, SLE_UINT8, 3, 255),

	SLE_END()
};

static void Save_WAYP(void)
{
	Waypoint *wp;

	FOR_ALL_WAYPOINTS(wp) {
		if (wp->xy != 0) {
			SlSetArrayIndex(wp->index);
			SlObject(wp, _waypoint_desc);
		}
	}
}

static void Load_WAYP(void)
{
	int index;

	while ((index = SlIterateArray()) != -1) {
		Waypoint *wp;

		if (!AddBlockIfNeeded(&_waypoint_pool, index))
			error("Waypoints: failed loading savegame: too many waypoints");

		wp = GetWaypoint(index);
		SlObject(wp, _waypoint_desc);
	}
}

const ChunkHandler _waypoint_chunk_handlers[] = {
	{ 'CHKP', Save_WAYP, Load_WAYP, CH_ARRAY | CH_LAST},
};
