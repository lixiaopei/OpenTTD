/* $Id$ */

/** @file newgrf_station.c Functions for dealing with station classes and custom stations. */

#include "stdafx.h"
#include "openttd.h"
#include "debug.h"
#include "sprite.h"
#include "station.h"
#include "newgrf_station.h"

static StationClass station_classes[STAT_CLASS_MAX];

/**
 * Reset station classes to their default state.
 * This includes initialising the Default and Waypoint classes with an empty
 * entry, for standard stations and waypoints.
 */
void ResetStationClasses(void)
{
	StationClassID i;
	for (i = 0; i < STAT_CLASS_MAX; i++) {
		station_classes[i].id = 0;

		free(station_classes[i].name);
		station_classes[i].name = NULL;

		station_classes[i].stations = 0;

		free(station_classes[i].spec);
		station_classes[i].spec = NULL;
	}

	// Set up initial data
	station_classes[0].id = 'DFLT';
	station_classes[0].name = strdup("Default");
	station_classes[0].stations = 1;
	station_classes[0].spec = malloc(sizeof(*station_classes[0].spec));
	station_classes[0].spec[0] = NULL;

	station_classes[1].id = 'WAYP';
	station_classes[1].name = strdup("Waypoints");
	station_classes[1].stations = 1;
	station_classes[1].spec = malloc(sizeof(*station_classes[1].spec));
	station_classes[1].spec[0] = NULL;
}

/**
 * Allocate a station class for the given class id.
 * @param classid A 32 bit value identifying the class.
 * @return Index into station_classes of allocated class.
 */
StationClassID AllocateStationClass(uint32 class)
{
	StationClassID i;

	for (i = 0; i < STAT_CLASS_MAX; i++) {
		if (station_classes[i].id == class) {
			// ClassID is already allocated, so reuse it.
			return i;
		} else if (station_classes[i].id == 0) {
			// This class is empty, so allocate it to the ClassID.
			station_classes[i].id = class;
			return i;
		}
	}

	DEBUG(grf, 2)("StationClassAllocate: Already allocated %d classes, using default.", STAT_CLASS_MAX);
	return STAT_CLASS_DFLT;
}

/**
 * Return the number of stations for the given station class.
 * @param sclass Index of the station class.
 * @return Number of stations in the class.
 */
uint GetNumCustomStations(StationClassID sclass)
{
	assert(sclass < STAT_CLASS_MAX);
	return station_classes[sclass].stations;
}

/**
 * Tie a station spec to its station class.
 * @param spec The station spec.
 */
void SetCustomStation(StationSpec *spec)
{
	StationClass *station_class;
	int i;

	assert(spec->sclass < STAT_CLASS_MAX);
	station_class = &station_classes[spec->sclass];

	i = station_class->stations++;
	station_class->spec = realloc(station_class->spec, station_class->stations * sizeof(*station_class->spec));

	station_class->spec[i] = spec;
}

/**
 * Retrieve a station spec from a class.
 * @param sclass Index of the station class.
 * @param station The station index with the class.
 * @return The station spec.
 */
const StationSpec *GetCustomStation(StationClassID sclass, uint station)
{
	assert(sclass < STAT_CLASS_MAX);
	if (station < station_classes[sclass].stations)
		return station_classes[sclass].spec[station];

	// If the custom station isn't defined any more, then the GRF file
	// probably was not loaded.
	return NULL;
}

static const RealSpriteGroup *ResolveStationSpriteGroup(const SpriteGroup *spg, const Station *st)
{
	switch (spg->type) {
		case SGT_REAL:
			return &spg->g.real;

		case SGT_DETERMINISTIC: {
			const DeterministicSpriteGroup *dsg = &spg->g.determ;
			SpriteGroup *target;
			int value = -1;

			if ((dsg->variable >> 6) == 0) {
				/* General property */
				value = GetDeterministicSpriteValue(dsg->variable);
			} else {
				if (st == NULL) {
					/* We are in a build dialog of something,
					 * and we are checking for something undefined.
					 * That means we should get the first target
					 * (NOT the default one). */
					if (dsg->num_ranges > 0) {
						target = dsg->ranges[0].group;
					} else {
						target = dsg->default_group;
					}
					return ResolveStationSpriteGroup(target, NULL);
				}

				/* Station-specific property. */
				if (dsg->var_scope == VSG_SCOPE_PARENT) {
					/* TODO: Town structure. */

				} else /* VSG_SELF */ {
					if (dsg->variable == 0x40 || dsg->variable == 0x41) {
						/* FIXME: This is ad hoc only
						 * for waypoints. */
						value = 0x01010000;
					} else {
						/* TODO: Only small fraction done. */
						// TTDPatch runs on little-endian arch;
						// Variable is 0x70 + offset in the TTD's station structure
						switch (dsg->variable - 0x70) {
							case 0x80: value = st->facilities;             break;
							case 0x81: value = st->airport_type;           break;
							case 0x82: value = st->truck_stops->status;    break;
							case 0x83: value = st->bus_stops->status;      break;
							case 0x86: value = st->airport_flags & 0xFFFF; break;
							case 0x87: value = st->airport_flags & 0xFF;   break;
							case 0x8A: value = st->build_date;             break;
						}
					}
				}
			}

			target = value != -1 ? EvalDeterministicSpriteGroup(dsg, value) : dsg->default_group;
			return ResolveStationSpriteGroup(target, st);
		}

		default:
		case SGT_RANDOMIZED:
			DEBUG(grf, 6)("I don't know how to handle random spritegroups yet!");
			return NULL;
	}
}

uint32 GetCustomStationRelocation(const StationSpec *spec, const Station *st, byte ctype)
{
	const RealSpriteGroup *rsg = ResolveStationSpriteGroup(spec->spritegroup[ctype], st);

	if (rsg->sprites_per_set != 0) {
		if (rsg->loading_count != 0) return rsg->loading[0]->g.result.result;
		if (rsg->loaded_count != 0) return rsg->loaded[0]->g.result.result;
	}

	DEBUG(grf, 6)("Custom station 0x%08x::0x%02x has no sprites associated.",
		spec->grfid, spec->localidx);
	/* This is what gets subscribed of dtss->image in newgrf.c,
	 * so it's probably kinda "default offset". Try to use it as
	 * emergency measure. */
	return 0;
}
