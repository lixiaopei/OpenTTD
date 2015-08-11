/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map.cpp Base functions related to the map and distances on them. */

#include "stdafx.h"
#include "debug.h"
#include "core/alloc_func.hpp"
#include "water_map.h"
#include "string_func.h"

#include "safeguards.h"

#if defined(_MSC_VER)
/* Why the hell is that not in all MSVC headers?? */
extern "C" _CRTIMP void __cdecl _assert(void *, void *, unsigned);
#endif
/*
https://wiki.openttd.org/MapRewriteDesign
Summary of the biggest changes

Okay, so I'll list here the main changes, point by point. Some of these are already coded, some are still in progress.
Removed the unlabeled _map arrays and replaced them by a single, well defined and named struct.
Removed all the integer literals used everywhere for the map by enums.
Added an additional height level between every current height level (not a feature yet)
Separated the surface of the tile from the stuff that is build upon it, in the struct they are stored in seperate elements.
Added support for layered tiles. Apart from the normal ground tiles, additional tiles can be stacked above (or under) existing tiles. This way, bridges are for example seperate tiles from the ground underneath, for maximum flexibility.
Added internal support for highway-style supports (not a feature yet)
What is this multilayer stuff?

Frames of reference

In order not to cause any more confusion, I'll sum up the reference systems we should be using:
Screen Reference System (SRS)
Pixels on the screen. X runs across, Y down, defined in the struct "ScreenCoord"
Map Reference System (MRS)
Counts in units from the origin of the map and stores only X and Y coordinates. X runs from upper right to lower left. This is defined in PointXY.
Full Reference System (FRS)
Like the Map Reference System, but it stores a Z coordinate as well. The matching struct is a "Point"
Map Tile System (MTS)
Stores the X and Y position of a Tile, in units of one Tile; this should be stored in TStackXY. Again X runs from upper right to lower left.
Tile Reference System (TRS)
Counts in units from the nothern corner of the Tile. X from upper right to lower left, Y perpendicular, Z in the vertical axis. Stored as TileSubCoords.
What does a tile look like now?

Instead of having a lot of seperate unlabeled arrays which contain miscellaneous data, we will now store information about a tile in a struct appropriately named "Tile". The exact contents are to be found in the code (where?), but we will discuss the main lines here.
A Tile will contain the following data:
The owner of the tile. Players are indexed from 0 upwards, other special owners have constants defined.
The height and slope of the tile. See below for what these values mean.
A pointer to the next tile in the stack. The exact ordering of these tiles is as of yet undefined, has anybody got some ideas about this? Maybe these pointers should be in order of height? Maybe the should loop?
The surface type of the tile. Think of things like "bridge", "ground", "support", "tunnel entrance", etc. This is basically anything that is the surface of the tile and/or below.
The build type of the tile. Think of things like "tracks", "road", "buildings", "stations", "fields", "trees", etc. This is basically anything that is built on the surface of the tile.
Information about the surface of the tile. This is stored as a union, with one element for each surface type. Which element is used in the tile is determined by the surface type of the tile.
Information about the build on the tile. This a union, just like the information about the surface.
There is also some stuff that isn't stored on a tile:
The x and y coordinates of a tile. With the old map, when you wanted to pass a tile to a function, you just passed the index, which was essentially a condensed form of the x and y coordinates. With the index, information about the tile could be accessed. With the new map this won't work, since we must also pass the height of the tile. Unfortuanetely we can't just pass around x, y and z coordinates, since there can be multiple tiles at every z coordinate. So we will just pass around Tile pointers, right? Well, another problem is that if you just pass a pointer, you can't really determine the neighbouring tiles, since we don't store (x,y) in a Tile. So, we will have to pass around those (x,y) everywhere. So we will just build a struct TileRef than, which replaces the old TileIndex. (Note: this struct has not yet been defined anywhere).
Information about what can be built on, above and under a tile. Some people have thought about storing this explicitely in the tile, but this information can always be implicetely determined from the tile. Because different tile and surface and build types allow for different things, we will define several type-dependent functions for that. (see below).
How do we store heights and slopes then?

In short, we will store one altitude for each tile, and 4 heights, one for each corner. To get the actual altitude of a corner, you take the tile altitude, multiply it by 4, and add the corner height to that. Simple as that :-)
Some people will notice that altitude is 4 bits, so we could multiply it by 16 (or 8) before adding. The big problem with that is that then we can't have tile that has corner altitudes of (15, 15, 17, 17) for example. I can't really explain why, just draw out the example and I hope you'll see.
Of course you should NEVER perform this calculation (or acces these values) directly, use the various helper functions (tile.h) for that.
Definitions about a tile heights
Two tiles in the same tile stack should never intersect. We also say that two tiles will never have identical corners. This means that for two givens tiles, some corner in one tile is higher than the corresponding corner in the other tile. To prevent intersection, all other corners in the first tile must be higher than the corresponding corners on the second tile.
If tiles don't intersect, we can easily see which tile is higher than the other. Formally, we can say that a tile is higher than another tile, if any of it's corners is higher than the corresponding corner on the other tile.
Now we have defined a unique ordering between two tiles, we can see that this ordering will work for more than two tiles. In other words, if A > B, and B > C, then A > C, which is also an important property.
If anyone likes, I (Matthijs) could write up formal proof for all of this, but I don't think that's necessary?
What should I use to manipulate tiles?

Most people will be inclined to start using the _map array, with the various tile variables that are used everywhere as index. You shouldn't.
This is because of 2 reasons. If everyone is just using the _map array, we can't change shit about how things are stored internally, without changing all the code again. Because of this, we will define functions to access the map. These functions can then be changed internally later on if that would be more efficient or better. This means that when using these functions, pay close attention to the comments above the function declaration and don't assume things.
For example, it is most likely that the _map array will contain the ground tile and that all the tiles above and below the ground tile are stored in the linked list formed by Tile.next. But, you still need to use the GetGroundTile function to get to the ground tile, and GetTileAbove and GetTileBelow to get to the other tiles, for the exact order might change in the future.
The second reason, or actually the second problem with dumbly converting, is that you can't just use the tile variable anymore. Because of the multilayer approach, you can't just address a tile by an index anymore. So, for addressing tiles, you will generally just use a Tile*. The biggest problem is that you can't determine an (x,y) from a Tile*, so if needed, you will pass a TileRef around. A TileRef just wraps a Tile* together with the TileXY. A tileXY is a reference to an entire stack of tiles on a given (x,y) coordinate. Lastly there is a TileXYN, but that is only used for sending tiles over the network.
Notice that you can always translate a TileRef to a Tile* or a TileXY, but not the other way around!
So, what functions should you use then? Not everyone have been defined yet, but you should definately take a look at map.h for the functions that acces tiles of the map, and at tile.h for functions that query individual tiles. There should be functions for a lot of common things, such as looking up the track type of a tile, etc. Check these files often, new functions will be introduced!
When working with these tile things, try to use consistent variable namings: "t" for a Tile*, "tr" for a TileRef*, "txy" for a TileXY*.
Variables, Variables, Variables

It's nice to know how to store tiles, but it is even nicer to know how to store references to parts of the map.
Within our openttd world, we can define three different concepts.
Description	Referencable by
The map	This is the collection of all tiles.	Not referencable, since we only have one map for now.
Tile Stacks	The map is divided into squares using a grid. Each square has a x and y cooridnate. All the tiles in one square that have the same x and y coordinates, are called a tile stack. In other words, all tiles that are stacked on top of each other are a stack.	
TStack ts
This is a pointer to an entire stack (cannot be converted to a TileXY)	
TStackXY tsxy
This is the (x,y) location of a tile stack (can be easily converted to a TileStack)
Tiles	A tile is a essentially an element of the tile stack. It is a square on a given x and y coordinate (iow, in a given tile stack) with a given height and slope.	
Tile* t
This is a pointer to a single tile (cannot be converted to a TileRef)	
TileRef tr
This is essentially the location of a single tile. It stores the (x,y) and height information of a tile (can be easily converted to a Tile*)
Besides these referencing methods, there is also TileXYN, which is the network compatible variant of TileRef. You should not use it outside the network code.
Miscelaneous other changes

We changed the INLINE macro to inline on non C99 compatible platforms. So, if you define an inline function, just write "inline" instead of "INLINE". If you see INLINE anywhere, replace it by inline.
We added an index into the _depots array for depot tiles. Currently the translation from a tile to a depot index is made by looping all depots until the right one is found. This is of course braindead. You should instead use DEREF_DEPOT(t->b.depot.index).
*/
uint _map_log_x;     ///< 2^_map_log_x == _map_size_x
uint _map_log_y;     ///< 2^_map_log_y == _map_size_y
uint _map_size_x;    ///< Size of the map along the X
uint _map_size_y;    ///< Size of the map along the Y
uint _map_size;      ///< The number of tiles on the map
uint _map_tile_mask; ///< _map_size - 1 (to mask the mapsize)

Tile *_m = NULL;          ///< Tiles of the map
TileExtended *_me = NULL; ///< Extended Tiles of the map


/**
 * (Re)allocates a map with the given dimension
 * @param size_x the width of the map along the NE/SW edge
 * @param size_y the 'height' of the map along the SE/NW edge
 */
void AllocateMap(uint size_x, uint size_y)
{
	/* Make sure that the map size is within the limits and that
	 * size of both axes is a power of 2. */
	if (!IsInsideMM(size_x, MIN_MAP_SIZE, MAX_MAP_SIZE + 1) ||
			!IsInsideMM(size_y, MIN_MAP_SIZE, MAX_MAP_SIZE + 1) ||
			(size_x & (size_x - 1)) != 0 ||
			(size_y & (size_y - 1)) != 0) {
		error("Invalid map size");
	}

	DEBUG(map, 1, "Allocating map of size %dx%d", size_x, size_y);

	_map_log_x = FindFirstBit(size_x);
	_map_log_y = FindFirstBit(size_y);
	_map_size_x = size_x;
	_map_size_y = size_y;
	_map_size = size_x * size_y;
	_map_tile_mask = _map_size - 1;

	free(_m);
	free(_me);

	_m = CallocT<Tile>(_map_size);
	_me = CallocT<TileExtended>(_map_size);
}


#ifdef _DEBUG
TileIndex TileAdd(TileIndex tile, TileIndexDiff add,
	const char *exp, const char *file, int line)
{
	int dx;
	int dy;
	uint x;
	uint y;

	dx = add & MapMaxX();
	if (dx >= (int)MapSizeX() / 2) dx -= MapSizeX();
	dy = (add - dx) / (int)MapSizeX();

	x = TileX(tile) + dx;
	y = TileY(tile) + dy;

	if (x >= MapSizeX() || y >= MapSizeY()) {
		char buf[512];

		seprintf(buf, lastof(buf), "TILE_ADD(%s) when adding 0x%.4X and 0x%.4X failed",
			exp, tile, add);
#if !defined(_MSC_VER) || defined(WINCE)
		fprintf(stderr, "%s:%d %s\n", file, line, buf);
#else
		_assert(buf, (char*)file, line);
#endif
	}

	assert(TileXY(x, y) == TILE_MASK(tile + add));

	return TileXY(x, y);
}
#endif

/**
 * This function checks if we add addx/addy to tile, if we
 * do wrap around the edges. For example, tile = (10,2) and
 * addx = +3 and addy = -4. This function will now return
 * INVALID_TILE, because the y is wrapped. This is needed in
 * for example, farmland. When the tile is not wrapped,
 * the result will be tile + TileDiffXY(addx, addy)
 *
 * @param tile the 'starting' point of the adding
 * @param addx the amount of tiles in the X direction to add
 * @param addy the amount of tiles in the Y direction to add
 * @return translated tile, or INVALID_TILE when it would've wrapped.
 */
TileIndex TileAddWrap(TileIndex tile, int addx, int addy)
{
	uint x = TileX(tile) + addx;
	uint y = TileY(tile) + addy;

	/* Disallow void tiles at the north border. */
	if ((x == 0 || y == 0) && _settings_game.construction.freeform_edges) return INVALID_TILE;

	/* Are we about to wrap? */
	if (x >= MapMaxX() || y >= MapMaxY()) return INVALID_TILE;

	return TileXY(x, y);
}

/** 'Lookup table' for tile offsets given a DiagDirection */
extern const TileIndexDiffC _tileoffs_by_diagdir[] = {
	{-1,  0}, ///< DIAGDIR_NE
	{ 0,  1}, ///< DIAGDIR_SE
	{ 1,  0}, ///< DIAGDIR_SW
	{ 0, -1}  ///< DIAGDIR_NW
};

/** 'Lookup table' for tile offsets given a Direction */
extern const TileIndexDiffC _tileoffs_by_dir[] = {
	{-1, -1}, ///< DIR_N
	{-1,  0}, ///< DIR_NE
	{-1,  1}, ///< DIR_E
	{ 0,  1}, ///< DIR_SE
	{ 1,  1}, ///< DIR_S
	{ 1,  0}, ///< DIR_SW
	{ 1, -1}, ///< DIR_W
	{ 0, -1}  ///< DIR_NW
};

/**
 * Gets the Manhattan distance between the two given tiles.
 * The Manhattan distance is the sum of the delta of both the
 * X and Y component.
 * Also known as L1-Norm
 * @param t0 the start tile
 * @param t1 the end tile
 * @return the distance
 */
uint DistanceManhattan(TileIndex t0, TileIndex t1)
{
	const uint dx = Delta(TileX(t0), TileX(t1));
	const uint dy = Delta(TileY(t0), TileY(t1));
	return dx + dy;
}


/**
 * Gets the 'Square' distance between the two given tiles.
 * The 'Square' distance is the square of the shortest (straight line)
 * distance between the two tiles.
 * Also known as euclidian- or L2-Norm squared.
 * @param t0 the start tile
 * @param t1 the end tile
 * @return the distance
 */
uint DistanceSquare(TileIndex t0, TileIndex t1)
{
	const int dx = TileX(t0) - TileX(t1);
	const int dy = TileY(t0) - TileY(t1);
	return dx * dx + dy * dy;
}


/**
 * Gets the biggest distance component (x or y) between the two given tiles.
 * Also known as L-Infinity-Norm.
 * @param t0 the start tile
 * @param t1 the end tile
 * @return the distance
 */
uint DistanceMax(TileIndex t0, TileIndex t1)
{
	const uint dx = Delta(TileX(t0), TileX(t1));
	const uint dy = Delta(TileY(t0), TileY(t1));
	return max(dx, dy);
}


/**
 * Gets the biggest distance component (x or y) between the two given tiles
 * plus the Manhattan distance, i.e. two times the biggest distance component
 * and once the smallest component.
 * @param t0 the start tile
 * @param t1 the end tile
 * @return the distance
 */
uint DistanceMaxPlusManhattan(TileIndex t0, TileIndex t1)
{
	const uint dx = Delta(TileX(t0), TileX(t1));
	const uint dy = Delta(TileY(t0), TileY(t1));
	return dx > dy ? 2 * dx + dy : 2 * dy + dx;
}

/**
 * Param the minimum distance to an edge
 * @param tile the tile to get the distance from
 * @return the distance from the edge in tiles
 */
uint DistanceFromEdge(TileIndex tile)
{
	const uint xl = TileX(tile);
	const uint yl = TileY(tile);
	const uint xh = MapSizeX() - 1 - xl;
	const uint yh = MapSizeY() - 1 - yl;
	const uint minl = min(xl, yl);
	const uint minh = min(xh, yh);
	return min(minl, minh);
}

/**
 * Gets the distance to the edge of the map in given direction.
 * @param tile the tile to get the distance from
 * @param dir the direction of interest
 * @return the distance from the edge in tiles
 */
uint DistanceFromEdgeDir(TileIndex tile, DiagDirection dir)
{
	switch (dir) {
		case DIAGDIR_NE: return             TileX(tile) - (_settings_game.construction.freeform_edges ? 1 : 0);
		case DIAGDIR_NW: return             TileY(tile) - (_settings_game.construction.freeform_edges ? 1 : 0);
		case DIAGDIR_SW: return MapMaxX() - TileX(tile) - 1;
		case DIAGDIR_SE: return MapMaxY() - TileY(tile) - 1;
		default: NOT_REACHED();
	}
}

/**
 * Function performing a search around a center tile and going outward, thus in circle.
 * Although it really is a square search...
 * Every tile will be tested by means of the callback function proc,
 * which will determine if yes or no the given tile meets criteria of search.
 * @param tile to start the search from. Upon completion, it will return the tile matching the search
 * @param size: number of tiles per side of the desired search area
 * @param proc: callback testing function pointer.
 * @param user_data to be passed to the callback function. Depends on the implementation
 * @return result of the search
 * @pre proc != NULL
 * @pre size > 0
 */
bool CircularTileSearch(TileIndex *tile, uint size, TestTileOnSearchProc proc, void *user_data)
{
	assert(proc != NULL);
	assert(size > 0);

	if (size % 2 == 1) {
		/* If the length of the side is uneven, the center has to be checked
		 * separately, as the pattern of uneven sides requires to go around the center */
		if (proc(*tile, user_data)) return true;

		/* If tile test is not successful, get one tile up,
		 * ready for a test in first circle around center tile */
		*tile = TILE_ADD(*tile, TileOffsByDir(DIR_N));
		return CircularTileSearch(tile, size / 2, 1, 1, proc, user_data);
	} else {
		return CircularTileSearch(tile, size / 2, 0, 0, proc, user_data);
	}
}

/**
 * Generalized circular search allowing for rectangles and a hole.
 * Function performing a search around a center rectangle and going outward.
 * The center rectangle is left out from the search. To do a rectangular search
 * without a hole, set either h or w to zero.
 * Every tile will be tested by means of the callback function proc,
 * which will determine if yes or no the given tile meets criteria of search.
 * @param tile to start the search from. Upon completion, it will return the tile matching the search.
 *  This tile should be directly north of the hole (if any).
 * @param radius How many tiles to search outwards. Note: This is a radius and thus different
 *                from the size parameter of the other CircularTileSearch function, which is a diameter.
 * @param w the width of the inner rectangle
 * @param h the height of the inner rectangle
 * @param proc callback testing function pointer.
 * @param user_data to be passed to the callback function. Depends on the implementation
 * @return result of the search
 * @pre proc != NULL
 * @pre radius > 0
 */
bool CircularTileSearch(TileIndex *tile, uint radius, uint w, uint h, TestTileOnSearchProc proc, void *user_data)
{
	assert(proc != NULL);
	assert(radius > 0);

	uint x = TileX(*tile) + w + 1;
	uint y = TileY(*tile);

	const uint extent[DIAGDIR_END] = { w, h, w, h };

	for (uint n = 0; n < radius; n++) {
		for (DiagDirection dir = DIAGDIR_BEGIN; dir < DIAGDIR_END; dir++) {
			/* Is the tile within the map? */
			for (uint j = extent[dir] + n * 2 + 1; j != 0; j--) {
				if (x < MapSizeX() && y < MapSizeY()) {
					TileIndex t = TileXY(x, y);
					/* Is the callback successful? */
					if (proc(t, user_data)) {
						/* Stop the search */
						*tile = t;
						return true;
					}
				}

				/* Step to the next 'neighbour' in the circular line */
				x += _tileoffs_by_diagdir[dir].x;
				y += _tileoffs_by_diagdir[dir].y;
			}
		}
		/* Jump to next circle to test */
		x += _tileoffs_by_dir[DIR_W].x;
		y += _tileoffs_by_dir[DIR_W].y;
	}

	*tile = INVALID_TILE;
	return false;
}

/**
 * Finds the distance for the closest tile with water/land given a tile
 * @param tile  the tile to find the distance too
 * @param water whether to find water or land
 * @return distance to nearest water (max 0x7F) / land (max 0x1FF; 0x200 if there is no land)
 */
uint GetClosestWaterDistance(TileIndex tile, bool water)
{
	if (HasTileWaterGround(tile) == water) return 0;

	uint max_dist = water ? 0x7F : 0x200;

	int x = TileX(tile);
	int y = TileY(tile);

	uint max_x = MapMaxX();
	uint max_y = MapMaxY();
	uint min_xy = _settings_game.construction.freeform_edges ? 1 : 0;

	/* go in a 'spiral' with increasing manhattan distance in each iteration */
	for (uint dist = 1; dist < max_dist; dist++) {
		/* next 'diameter' */
		y--;

		/* going counter-clockwise around this square */
		for (DiagDirection dir = DIAGDIR_BEGIN; dir < DIAGDIR_END; dir++) {
			static const int8 ddx[DIAGDIR_END] = { -1,  1,  1, -1};
			static const int8 ddy[DIAGDIR_END] = {  1,  1, -1, -1};

			int dx = ddx[dir];
			int dy = ddy[dir];

			/* each side of this square has length 'dist' */
			for (uint a = 0; a < dist; a++) {
				/* MP_VOID tiles are not checked (interval is [min; max) for IsInsideMM())*/
				if (IsInsideMM(x, min_xy, max_x) && IsInsideMM(y, min_xy, max_y)) {
					TileIndex t = TileXY(x, y);
					if (HasTileWaterGround(t) == water) return dist;
				}
				x += dx;
				y += dy;
			}
		}
	}

	if (!water) {
		/* no land found - is this a water-only map? */
		for (TileIndex t = 0; t < MapSize(); t++) {
			if (!IsTileType(t, MP_VOID) && !IsTileType(t, MP_WATER)) return 0x1FF;
		}
	}

	return max_dist;
}
