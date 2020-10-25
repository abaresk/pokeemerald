#include "global.h"
#include "boolean.h"
#include "ecs.h"
#include "circuit.h"

// My implementation will use a weak ECS system: it will allow some objects to
// store components. It will have an event and triggering mechanism.

// It would be easier to do ECS if we modeled each tile as a separate entity.
// Each wire is an entity, each node is an entity. In the future, you could 
// optimize this by storing each wire stretch, instead of each individaul wire.

/* 
*  Nodes should be able to add on components:
*  Components related to charging:
*    - Generator
*    - Conductor
*    - Trigger
*
*  Other components:
*    - On/Off - adds/removes components when pressed
*/

// PowerSupply currents
#define NORTH (1 << 0)
#define EAST  (1 << 1)
#define SOUTH (1 << 2)
#define WEST  (1 << 3)

#define OUT(x) (x << 4)

#define CURRENT_NORTH_IN  (NORTH)
#define CURRENT_EAST_IN   (EAST)
#define CURRENT_SOUTH_IN  (SOUTH)
#define CURRENT_WEST_IN   (WEST)
#define CURRENT_NORTH_OUT (OUT(NORTH))
#define CURRENT_EAST_OUT  (OUT(EAST))
#define CURRENT_SOUTH_OUT (OUT(SOUTH))
#define CURRENT_WEST_OUT  (OUT(WEST))

#define ALL_CURRENTS_IN  (CURRENT_NORTH_IN | CURRENT_EAST_IN | CURRENT_SOUTH_IN | CURRENT_WEST_IN)
#define ALL_CURRENTS_OUT (CURRENT_NORTH_OUT | CURRENT_EAST_OUT | CURRENT_SOUTH_OUT | CURRENT_WEST_OUT)
#define ALL_CURRENTS     (ALL_CURRENTS_IN | ALL_CURRENTS_OUT)

// All ports
#define PORT_NORTH (NORTH)
#define PORT_EAST  (EAST)
#define PORT_SOUTH (SOUTH)
#define PORT_WEST  (WEST)

#define ALL_PORTS (PORT_NORTH | PORT_EAST | PORT_SOUTH | PORT_WEST)

// Propagation should be fast. Should spread multiple spaces within a frame.
// Discharge should spread faster to prevent infinite loops.
#define CHARGE_TICKS    1
#define DISCHARGE_TICKS 2

// Fixed because each Conductor keeps track of the generator that created the
// impulse going through its ports
#define NUM_GENERATORS 8

/*
*  Components
*/
// TODO: Move this to an area for universal components
typedef struct {
    s16 x, y;
} Position;

// The direction an entity is moving.
typedef struct {
    u8 dir;
} Movement;

// Conductor - has ports where current can flow in and out
typedef struct {
    u8 ports;
} Conductor;

// Conductor can have PowerSupply, indexed by the Generator that is providing
// power.
typedef struct {
    u8[4] supplies;
} PowerSupply;

typedef struct {
    // Each generator is assigned a unique ID between 0-7. This identifies 
    // which generator a given impulse comes from.
    u8 id;
} Generator;

// Both fields are indexed by generatorId
typedef struct {
    u8 on;
    u8 generatorId;
} Impulse;


// Triggered when a Generator is turned on
// OnSet System: {Generator, Conductor, Position}
// Add an On Impulse to all neighboring Conductors
void GeneratorSpreadCharge(ecs_iter_t *it) {
    Generator *gen = ecs_column(it, Generator, 1);
    Conductor *conduct = ecs_column(it, Conductor, 2);
    Position *pos = ecs_column(it, Position, 3);

    for (s32 port = NORTH; port <= WEST; port <<= 1) {
        tryPassImpulse(*conduct, *pos, port);
    }
}

static ecs_entity_t* getConductorAtPos(Position pos) {
    ecs_entity_t *entList = EntitiesAtPos(pos);
    for (s32 i = 0; i < ARRAY_COUNT(entList); i++) {
        if(!ecs_get(entList[i], Conductor)) {
            return &entList[i];
        }
    }
    return NULL;
}

static bool8 connectedAtPort(ecs_entity_t e, u8 port, ecs_entity_t *adj) {
    Conductor *conduct = ecs_get(e, Conductor);
    Position *pos = ecs_get(e, Position);
    ecs_assert(conduct != NULL && pos != NULL);
    ecs_assert(conduct->ports & port);

    adj = getConductorAtPos(PosOffset(*pos, DIR(port), 1));
    Conductor *adjConduct = ecs_get(e, Conductor);
    if (adj == NULL || adjConduct ==  NULL) {
        return FALSE;
    }
    return PORT_OPPOSITE(port) & adjConduct->ports;
}

static bool8 hasPowerFromGenerator(PowerSupply power, u8 generatorId) { 
    for (u8 port = 0; port < 4; port++) {
        if (power->supplies[port] & generatorId) {
            return TRUE;
        }
    }
    return FALSE;
}

static void spreadChargeTick(ecs_entity_t e, u8 port, u8 generatorId) {
    Conductor *conduct = ecs_get(e, Conductor);
    PowerSupply *power = ecs_get(e, PowerSupply);
    Position *pos = ecs_get(e, Position);
    ecs_assert(power != NULL && conduct != NULL && pos != NULL);

    ecs_entity_t adj;
    if (!connectedAtPort(e, port, &adj)) { 
        return;
    }

    Conductor *adjConduct = ecs_get(adj, Conductor);
    PowerSupply *adjPower = ecs_get_mut(adj, PowerSupply);
    ecs_assert(adjConduct != NULL && adjPower != NULL);

    // Add power supply from generatorId, first checking for cycles.
    if (hasPowerFromGenerator(*adjPower, generatorId)) {
        return;
    }
    adjPower->supplies[PORT_OPPOSITE(port)] |= (1 << generatorId);
    // Shortcut - ideally we make a ticking system - this only spreads 1 per frame
    ecs_set(world, adj, Impulse, {.on = (1 << generatorId), .generatorID = (1 << generatorId)});
}







// Triggered when Movement is first added
// OnSet System: {Conductor, PowerSupply, Position, Movement}
void MoveConductor(ecs_iter_t *it) {
    Conductor *conduct = ecs_column(it, Conductor, 1);
    PowerSupply *power = ecs_column(it, PowerSupply, 2);
    Position *pos = ecs_column(it, Position, 3);
    Movement *move = ecs_column(it, Movement, 4);

    // Update PowerSupplies at current position
}

// Triggered when Movement is finished
// UnSet System: {Conductor, PowerSupply, Position, Movement}
void MoveConductor(ecs_iter_t *it) {
    Conductor *conduct = ecs_column(it, Conductor, 1);
    PowerSupply *power = ecs_column(it, PowerSupply, 2);
    Position *pos = ecs_column(it, Position, 3);

    // Updates PowerSupplies at new position
}


static void update


static bool8 hasPower(Conductor conduct, PowerSupply power) {
    return conduct & power;
}

// This is triggered whenever these components change for an entity: when the
// Condcutor is moved, rotated, or its power supply changes.
// OnSet System: {Conductor, PowerSupply, Position}
//
// Need to make this work for Position changes. Also if conductor moves onto space.
// This doesn't stop power from spreading back on itself. It should only move forwards
// through a circuit.
void SpreadCharge(ecs_iter_t *it) {
    Conductor *conduct = ecs_column(it, Conductor, 1);
    PowerSupply *power = ecs_column(it, PowerSupply, 2);
    Position *pos = ecs_column(it, Position, 3);
    PowerSupply *adjPower;
    bool8 hasPower;

    for (s32 i = 0; i < it->count; i++) {
        // For all neighbors, add PowerSupply on adjacent port. Remove if 
        // necessary from ex-neighbors.
        for (s32 port = PORT_NORTH; port <= PORT_WEST; port <<= 1) {
            // Alternatively, in each port, store the ID of the entity it's
            // connected to.
            ecs_entity_t *entList = EntitiesAtPos(PosOffset(pos, DIR(port), 1));

            // If current conductor is powered on and has an outgoing 
            // connection, add power supply to neighboring conductors. Otherwise, 
            // remove power supply.
            for (s32 i = 0; i < ARRAY_COUNT(entList); i++) {
                adjPower = ecs_get_mut(world, entList[i], PowerSupply);
                if (!adjPower || !ecs_get(world, entList[i], Conductor))
                    continue;

                hasPower = power->ports & conduct->ports;
                if (hasPower && conduct->ports & port) {
                    adjPower->ports |= PORT_OPPOSITE(port); // Make sure that if it doesn't change, it doesn't set off a system trigger
                } else {
                    adjPower->ports &= ~PORT_OPPOSITE(port);
                }
            }
        }
    }
}

// These aren't necessary - PowerSupply dictates whether a Conductor has power.
// // Does the piece have a charge going through it
// // {Charging, Charged, Discharging, Discharged} are mutually exclusive switch-case
// // components.
// // Alternate naming {PoweringOn, PoweredOn, PoweringOff, PoweredOff}
// typedef struct {} Charging;

// typedef struct {} Charged;

// typedef struct {} Discharging;

// typedef struct {} Discharged;

typedef struct {
    u8 timer;
} Timer;

// Generator - this piece is a source of current. This can be applied to
// batteries to make them become a source of current.
typedef struct {} Generator;

// System: {Conductor, Charging, Timer}
void ChargeConductor(ecs_iter_t *it) {
    Charging *c = ecs_column(it, Charging, 2);
    Timer *t = ecs_column(it, Timer, 3);

    for (s32 i = 0; i < it->count; i++) {
        if ((*t)++ < CHARGE_DELAY) {
            return;
        }

        // Switch to Charged component
        ecs_remove(world, e, Timer);
        ecs_add_entity(world, e, ECS_CASE | Charged);
    }
}

// System: {Conductor, Discharging, Timer}
void DischargeConductor(ecs_iter_t *it) {
    Discharging *c = ecs_column(it, Discharging, 2);
    Timer *t = ecs_column(it, Timer, 3);

    for (s32 i = 0; i < it->count; i++) {
        if ((*t)++ < DISCHARGE_DELAY) {
            return;
        }
        // Switch to Uncharged component
        ecs_remove(world, e, Timer);
        ecs_add_entity(world, e, ECS_CASE | Discharged);
    }
}

// Current spreads from entities with {Generator, Conductor} or 
// {Charge, Conductor} to any neighboring {Conductor} entities.

// OnSet System: {Charged, Conductor, Position}
void SpreadCharge_Charged(ecs_iter_t *it) {
    /* Get the two columns from the system signature */
    ECS_COLUMN(it, Conductor, conduct, 2);
    ECS_COLUMN(it, Position, pos, 3);

    for (s32 i = 0; i < it->count; i++) {
        spreadCharge(conduct[i], pos);
    }
}

// OnSet System: {Generator, Conductor, Position}
void SpreadCharge_Generator(ecs_iter_t *it) {
    /* Get the two columns from the system signature */
    ECS_COLUMN(it, Conductor, conduct, 2);
    ECS_COLUMN(it, Position, pos, 3);

    for (s32 i = 0; i < it->count; i++) {
        spreadCharge(conduct[i], pos);
    }
}

// Add Charging components onto all neighboring Conductors
static void spreadCharge(Conductor conduct, Position pos) {
    Conductor *adjConduct;
    u8 adjPort;

    for (s32 port = PORT_NORTH; port <= PORT_WEST; port <<= 1) {
        if (conduct.ports & port) {
            u8 dir = DIR(port); // Make this a macro

            // Alternatively, in each port, store the ID of the entity it's
            // connected to.
            ecs_entity_t *entList = EntitiesAtPos(PosOffset(pos, dir, 1));

            // Check whether there is a conductor adjacent to the port that can
            // be connected to. If the conductor is in {Charged, Charging}, do 
            // nothing. Set the conductor to Charging and mark the source of
            // its power supply.
            for (s32 i = 0; i < ARRAY_COUNT(entList); i++) {
                adjConduct = ecs_get(world, entList[i], Conductor);
                if (!adjConduct)
                    continue;
                
                adjPort = PORT_OPPOSITE(port);
                if (!(adjConduct->ports & adjPort))
                    continue;

                if (ecs_get(world, entList[i], Charged) ||  ecs_get(world, entList[i], Charging))
                    continue;

                ecs_add_entity(world, entList[i], ECS_CASE | Charging);
                ecs_set(world, entList[i], Timer, {0});
                ecs_set(world, entList[i], PowerSupply, {adjPort});
            }
        }
    }
}

// Think of how charges "un-spread" when power sources go out.
//
// Make {Discharged, Conductor} unspread to nearby conductors. When generator is
// removed an entity, unspread to nearby conductors. If a conductor is 
// {Charged, Charging} set to Discharging.
// 
// What if a conductor has a backup power source? Then it shouldn't be set to
// Discharging. Therefore, each Charged conductor should keep track of which
// ports it's receiving power from. Unspreading deactivates a conductor's power
// supply on the adjacent port. Set it to Discharging only if the conductor has
// no other power supplies.

// OnUnset System: {Charged, Conductor, Position}
void SpreadDischarge_Charged(ecs_iter_t *it) {
    ECS_COLUMN(it, Conductor, conduct, 2);
    ECS_COLUMN(it, Position, pos, 3);

    for (s32 i = 0; i < it->count; i++) {
        spreadDischarge(conduct[i]);
    }
}

// OnUnset System: {Generator, Conductor, Position}
void SpreadDischarge_Generator(ecs_iter_t *it) {
    ECS_COLUMN(it, Conductor, conduct, 2);
    ECS_COLUMN(it, Position, pos, 3);

    for (s32 i = 0; i < it->count; i++) {
        spreadDischarge(conduct[i]);
    }
}
// Add Discharging components onto all neighboring Conductors
static void spreadDischarge(Conductor conduct, Position pos) {
    Conductor *adjConduct;
    u8 adjPort;
    PowerSupply *supply;

    for (s32 port = PORT_NORTH; port <= PORT_WEST; port <<= 1) {
        if (conduct.ports & port) {
            ecs_entity_t *entList = EntitiesAtPos(PosOffset(pos, DIR(port), 1));

            // Check whether there is a conductor adjacent to the port that can
            // be connected to. If the conductor is in 
            // {Discharged, Discharging}, do nothing. Unmark the conductor's 
            // PowerSupply port adjacent to the current port. If the conductor
            // no longer has PowerSupply, set it to Discharging.
            for (s32 i = 0; i < ARRAY_COUNT(entList); i++) {
                adjConduct = ecs_get(world, entList[i], Conductor);
                if (!adjConduct)
                    continue;
                
                adjPort = PORT_OPPOSITE(port);
                if (!(adjConduct->ports & adjPort))
                    continue;

                if (ecs_get(world, entList[i], Discharged) || ecs_get(world, entList[i], Discharging))
                    continue;

                supply = ecs_get_mut(world, entList[i], PowerSupply);
                *supply &= ~adjPort;

                if (!(*supply & ALL_PORTS)) {
                    ecs_add_entity(world, entList[i], ECS_CASE | Discharging);
                    ecs_set(world, entList[i], Timer, {0});
                    ecs_remove(world, entList[i], PowerSupply);
                }
            }
        }
    }
}

// Think of how charges "un-spread" when the circuit gets broken, piece gets 
// added, or piece gets rotated.
// 
// Because SpreadCharge and SpreadDischarge are OnSet/OnUnset systems, whenever
// the Position changes, SpreadDischarge will be called on the old position,
// and SpreadCharge will be called on the new position.
//
// When a piece moves, its PowerSupply will have to update.
//
// Rotation:
// 
// When a piece rotates, its ports will need update.
//
// For rotation, SpreadCharge will be triggered when a conductor's ports are 
// changed. If the conductor is charged, should check that it is still connected
// to a power supply. If so, it stays Charged and it propagates the charge to
// all its ports, and tries to propagate a discharge to any non-ports. If it 
// no longer connected to power, it should try to propagate a discharge to all 
// its ports.
//
// When there's no charge going through the piece {Discharged, Discharging}, 
// once rotated, it may become connected to a power source.
// {Charging, Charged} means the piece holds a current that will spread to other
// pieces. 
//
// Propagation, checking {Charged} isn't enough. A new PowerSupply should 
// propagate throughout a conductor. E.g. two charged stretches become connected
// by a middle piece. The PowerSupply should be propagated throughout both 
// stretches.
//
// If the conductor is discharged, it should check that i
//

// Called externally - changes an entity's components, triggering a few systems.
// Alternatively, this could happen in a Rotation system. The caller would add a
// rotation component to the conductor specifying the direction. But this 
// introduces order dependency between the systems. 
bool8 RotateConductor(ecs_entity_t e, Rotate dir) {
    Conductor *c = ecs_get_mut(world, e, Conductor);
    if (c == NULL) {
        return FALSE;
    }
    if (dir == CLOCKWISE) {
        c->ports = rotateClockwise(c->ports);
    } else {
        c->ports = rotateCounterClockwise(c->ports);
    }
    return TRUE;
}


// Think about how certain objects should have behaviors triggered when the
// Charge component is set/unset (e.g., locked doors, gates, light bulb).

// Locked doors
// OnSet System: {Charged, SWITCH | Lock}
void SwitchLock(ecs_iter_t *it) {
    // Get the case state of Lock (either {Locked, Unlocked}), and invert it.
}

// Think about interactions with other systems/components (e.g. Generator is
// subscribed to On/Off switch).




// Helper funcs
static u8 rotr(u8 bits, u8 n, u8 r) {
    return (bits >> r) | (bits << (n - r));
}

static u8 rotateClockwise(u8 ports) {
    return rotr(ports, 4, 1);
}

static u8 rotateCounterClockwise(u8 ports) {
    return rotr(ports, 4, 3);
}


// Stored in ROM.
typedef struct
{
    // Open connections which can attach to a wire or another node.
    u8 ports;
    Component[MAX_COMPONENTS] comps;
} CircuitNode;

/*
*  How would you initialize a circuit?
*
*  Placing this here for now, but this is not a part of the circuit module.
*  This is will be called from the game.
*/
static void initCircuit(GymWorld *world, NodeMap *map) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(map); i++) {
        CircuitAddNode(world, map[i]);
    }
}

// State of circuit, stored in RAM.
typedef struct {

} CircuitWorld;

// Add a node to the circuit
void Circuit_AddNode(GymWorld *world, CircuitNode node) {

}

void Circuit_RemoveNode (GymWorld *world) {

}