/* $Id$ */

/** @file articulated_vehicles.cpp Implementation of articulated vehicles. */

#include "stdafx.h"
#include "train.h"
#include "roadveh.h"
#include "aircraft.h"
#include "newgrf_engine.h"
#include "vehicle_func.h"

static const uint MAX_ARTICULATED_PARTS = 100; ///< Maximum of articulated parts per vehicle, i.e. when to abort calling the articulated vehicle callback.

uint CountArticulatedParts(EngineID engine_type, bool purchase_window)
{
	if (!HasBit(EngInfo(engine_type)->callbackmask, CBM_VEHICLE_ARTIC_ENGINE)) return 0;

	/* If we can't allocate a vehicle now, we can't allocate it in the command
	 * either, so it doesn't matter how many articulated parts there are. */
	if (!Vehicle::CanAllocateItem()) return 0;

	Vehicle *v = NULL;;
	if (!purchase_window) {
		v = new InvalidVehicle();
		v->engine_type = engine_type;
	}

	uint i;
	for (i = 1; i < MAX_ARTICULATED_PARTS; i++) {
		uint16 callback = GetVehicleCallback(CBID_VEHICLE_ARTIC_ENGINE, i, 0, engine_type, v);
		if (callback == CALLBACK_FAILED || GB(callback, 0, 8) == 0xFF) break;
	}

	delete v;

	return i - 1;
}


/**
 * Returns the default (non-refitted) capacity of a specific EngineID.
 * @param engine the EngineID of iterest
 * @param type the type of the engine
 * @param cargo_type returns the default cargo type, if needed
 * @return capacity
 */
static inline uint16 GetVehicleDefaultCapacity(EngineID engine, VehicleType type, CargoID *cargo_type)
{
	const Engine *e = GetEngine(engine);
	CargoID cargo = (e->CanCarryCargo() ? e->GetDefaultCargoType() : (CargoID)CT_INVALID);
	if (cargo_type != NULL) *cargo_type = cargo;
	if (cargo == CT_INVALID) return 0;
	switch (type) {
		case VEH_TRAIN:
			return GetEngineProperty(engine, 0x14, e->u.rail.capacity) + (e->u.rail.railveh_type == RAILVEH_MULTIHEAD ? e->u.rail.capacity : 0);

		case VEH_ROAD:
			return GetEngineProperty(engine, 0x0F, e->u.road.capacity);

		case VEH_SHIP:
			return GetEngineProperty(engine, 0x0D, e->u.ship.capacity);

		case VEH_AIRCRAFT:
			return AircraftDefaultCargoCapacity(cargo, &e->u.air);

		default: NOT_REACHED();
	}

}

/**
 * Returns all cargos a vehicle can carry.
 * @param engine the EngineID of iterest
 * @param type the type of the engine
 * @param include_initial_cargo_type if true the default cargo type of the vehicle is included; if false only the refit_mask
 * @return bit set of CargoIDs
 */
static inline uint32 GetAvailableVehicleCargoTypes(EngineID engine, VehicleType type, bool include_initial_cargo_type)
{
	uint32 cargos = 0;
	CargoID initial_cargo_type;

	if (GetVehicleDefaultCapacity(engine, type, &initial_cargo_type) > 0) {
		if (type != VEH_SHIP || ShipVehInfo(engine)->refittable) {
			const EngineInfo *ei = EngInfo(engine);
			cargos = ei->refit_mask;
		}
		if (include_initial_cargo_type && initial_cargo_type < NUM_CARGO) SetBit(cargos, initial_cargo_type);
	}

	return cargos;
}

uint16 *GetCapacityOfArticulatedParts(EngineID engine, VehicleType type)
{
	static uint16 capacity[NUM_CARGO];
	memset(capacity, 0, sizeof(capacity));

	CargoID cargo_type;
	uint16 cargo_capacity = GetVehicleDefaultCapacity(engine, type, &cargo_type);
	if (cargo_type < NUM_CARGO) capacity[cargo_type] = cargo_capacity;

	if (type != VEH_TRAIN && type != VEH_ROAD) return capacity;

	if (!HasBit(EngInfo(engine)->callbackmask, CBM_VEHICLE_ARTIC_ENGINE)) return capacity;

	for (uint i = 1; i < MAX_ARTICULATED_PARTS; i++) {
		uint16 callback = GetVehicleCallback(CBID_VEHICLE_ARTIC_ENGINE, i, 0, engine, NULL);
		if (callback == CALLBACK_FAILED || GB(callback, 0, 8) == 0xFF) break;

		EngineID artic_engine = GetNewEngineID(GetEngineGRF(engine), type, GB(callback, 0, 7));

		cargo_capacity = GetVehicleDefaultCapacity(artic_engine, type, &cargo_type);
		if (cargo_type < NUM_CARGO) capacity[cargo_type] += cargo_capacity;
	}

	return capacity;
}

/**
 * Checks whether any of the articulated parts is refittable
 * @param engine the first part
 * @return true if refittable
 */
bool IsArticulatedVehicleRefittable(EngineID engine)
{
	if (IsEngineRefittable(engine)) return true;

	const Engine *e = GetEngine(engine);
	if (e->type != VEH_TRAIN && e->type != VEH_ROAD) return false;

	if (!HasBit(e->info.callbackmask, CBM_VEHICLE_ARTIC_ENGINE)) return false;

	for (uint i = 1; i < MAX_ARTICULATED_PARTS; i++) {
		uint16 callback = GetVehicleCallback(CBID_VEHICLE_ARTIC_ENGINE, i, 0, engine, NULL);
		if (callback == CALLBACK_FAILED || GB(callback, 0, 8) == 0xFF) break;

		EngineID artic_engine = GetNewEngineID(GetEngineGRF(engine), e->type, GB(callback, 0, 7));
		if (IsEngineRefittable(artic_engine)) return true;
	}

	return false;
}

/**
 * Ors the refit_masks of all articulated parts.
 * @param engine the first part
 * @param type the vehicle type
 * @param include_initial_cargo_type if true the default cargo type of the vehicle is included; if false only the refit_mask
 * @return bit mask of CargoIDs which are a refit option for at least one articulated part
 */
uint32 GetUnionOfArticulatedRefitMasks(EngineID engine, VehicleType type, bool include_initial_cargo_type)
{
	uint32 cargos = GetAvailableVehicleCargoTypes(engine, type, include_initial_cargo_type);

	if (type != VEH_TRAIN && type != VEH_ROAD) return cargos;

	if (!HasBit(EngInfo(engine)->callbackmask, CBM_VEHICLE_ARTIC_ENGINE)) return cargos;

	for (uint i = 1; i < MAX_ARTICULATED_PARTS; i++) {
		uint16 callback = GetVehicleCallback(CBID_VEHICLE_ARTIC_ENGINE, i, 0, engine, NULL);
		if (callback == CALLBACK_FAILED || GB(callback, 0, 8) == 0xFF) break;

		EngineID artic_engine = GetNewEngineID(GetEngineGRF(engine), type, GB(callback, 0, 7));
		cargos |= GetAvailableVehicleCargoTypes(artic_engine, type, include_initial_cargo_type);
	}

	return cargos;
}

/**
 * Ands the refit_masks of all articulated parts.
 * @param engine the first part
 * @param type the vehicle type
 * @param include_initial_cargo_type if true the default cargo type of the vehicle is included; if false only the refit_mask
 * @return bit mask of CargoIDs which are a refit option for every articulated part (with default capacity > 0)
 */
uint32 GetIntersectionOfArticulatedRefitMasks(EngineID engine, VehicleType type, bool include_initial_cargo_type)
{
	uint32 cargos = UINT32_MAX;

	uint32 veh_cargos = GetAvailableVehicleCargoTypes(engine, type, include_initial_cargo_type);
	if (veh_cargos != 0) cargos &= veh_cargos;

	if (type != VEH_TRAIN && type != VEH_ROAD) return cargos;

	if (!HasBit(EngInfo(engine)->callbackmask, CBM_VEHICLE_ARTIC_ENGINE)) return cargos;

	for (uint i = 1; i < MAX_ARTICULATED_PARTS; i++) {
		uint16 callback = GetVehicleCallback(CBID_VEHICLE_ARTIC_ENGINE, i, 0, engine, NULL);
		if (callback == CALLBACK_FAILED || GB(callback, 0, 8) == 0xFF) break;

		EngineID artic_engine = GetNewEngineID(GetEngineGRF(engine), type, GB(callback, 0, 7));
		veh_cargos = GetAvailableVehicleCargoTypes(artic_engine, type, include_initial_cargo_type);
		if (veh_cargos != 0) cargos &= veh_cargos;
	}

	return cargos;
}


/**
 * Tests if all parts of an articulated vehicle are refitted to the same cargo.
 * Note: Vehicles not carrying anything are ignored
 * @param v the first vehicle in the chain
 * @param cargo_type returns the common CargoID if needed. (CT_INVALID if no part is carrying something or they are carrying different things)
 * @return true if some parts are carrying different cargos, false if all parts are carrying the same (nothing is also the same)
 */
bool IsArticulatedVehicleCarryingDifferentCargos(const Vehicle *v, CargoID *cargo_type)
{
	CargoID first_cargo = CT_INVALID;

	do {
		if (v->cargo_cap > 0 && v->cargo_type != CT_INVALID) {
			if (first_cargo == CT_INVALID) first_cargo = v->cargo_type;
			if (first_cargo != v->cargo_type) {
				if (cargo_type != NULL) *cargo_type = CT_INVALID;
				return true;
			}
		}

		switch (v->type) {
			case VEH_TRAIN:
				v = (EngineHasArticPart(v) ? GetNextArticPart(v) : NULL);
				break;

			case VEH_ROAD:
				v = (RoadVehHasArticPart(v) ? v->Next() : NULL);
				break;

			default:
				v = NULL;
				break;
		}
	} while (v != NULL);

	if (cargo_type != NULL) *cargo_type = first_cargo;
	return false;
}


void AddArticulatedParts(Vehicle **vl, VehicleType type)
{
	const Vehicle *v = vl[0];
	Vehicle *u = vl[0];

	if (!HasBit(EngInfo(v->engine_type)->callbackmask, CBM_VEHICLE_ARTIC_ENGINE)) return;

	for (uint i = 1; i < MAX_ARTICULATED_PARTS; i++) {
		uint16 callback = GetVehicleCallback(CBID_VEHICLE_ARTIC_ENGINE, i, 0, v->engine_type, v);
		if (callback == CALLBACK_FAILED || GB(callback, 0, 8) == 0xFF) return;

		/* Attempt to use pre-allocated vehicles until they run out. This can happen
		 * if the callback returns different values depending on the cargo type. */
		u->SetNext(vl[i]);
		if (u->Next() == NULL) return;

		Vehicle *previous = u;
		u = u->Next();

		EngineID engine_type = GetNewEngineID(GetEngineGRF(v->engine_type), type, GB(callback, 0, 7));
		bool flip_image = HasBit(callback, 7);

		/* get common values from first engine */
		u->direction = v->direction;
		u->owner = v->owner;
		u->tile = v->tile;
		u->x_pos = v->x_pos;
		u->y_pos = v->y_pos;
		u->z_pos = v->z_pos;
		u->build_year = v->build_year;
		u->vehstatus = v->vehstatus & ~VS_STOPPED;

		u->cargo_subtype = 0;
		u->max_speed = 0;
		u->max_age = 0;
		u->engine_type = engine_type;
		u->value = 0;
		u->subtype = 0;
		u->cur_image = 0xAC2;
		u->random_bits = VehicleRandomBits();

		const Engine *e_artic = GetEngine(engine_type);
		switch (type) {
			default: NOT_REACHED();

			case VEH_TRAIN:
				u = new (u) Train();
				previous->SetNext(u);
				u->u.rail.track = v->u.rail.track;
				u->u.rail.railtype = v->u.rail.railtype;
				u->u.rail.first_engine = v->engine_type;

				u->spritenum = e_artic->u.rail.image_index;
				if (e_artic->CanCarryCargo()) {
					u->cargo_type = e_artic->GetDefaultCargoType();
					u->cargo_cap = e_artic->u.rail.capacity;  // Callback 36 is called when the consist is finished
				} else {
					u->cargo_type = v->cargo_type; // Needed for livery selection
					u->cargo_cap = 0;
				}

				SetArticulatedPart(u);
				break;

			case VEH_ROAD:
				u = new (u) RoadVehicle();
				previous->SetNext(u);
				u->u.road.first_engine = v->engine_type;
				u->u.road.cached_veh_length = GetRoadVehLength(u);
				u->u.road.state = RVSB_IN_DEPOT;

				u->u.road.roadtype = v->u.road.roadtype;
				u->u.road.compatible_roadtypes = v->u.road.compatible_roadtypes;

				u->spritenum = e_artic->u.road.image_index;
				if (e_artic->CanCarryCargo()) {
					u->cargo_type = e_artic->GetDefaultCargoType();
					u->cargo_cap = e_artic->u.road.capacity;  // Callback 36 is called when the consist is finished
				} else {
					u->cargo_type = v->cargo_type; // Needed for livery selection
					u->cargo_cap = 0;
				}

				SetRoadVehArticPart(u);
				break;
		}

		if (flip_image) u->spritenum++;

		VehiclePositionChanged(u);
	}
}
