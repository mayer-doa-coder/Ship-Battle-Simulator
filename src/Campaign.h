#pragma once

// Living-world campaign simulation.  This file intentionally contains no OpenGL: ports, cargo,
// docking, trading, map discovery, treasure clues and campaign progression can all be stepped and
// tested without a rendering context.  CampaignDraw.h turns this state into procedural geometry.

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace CampaignConfig {
constexpr int PORT_COUNT = 13;
constexpr int CARGO_COUNT = 14;
constexpr int MISSION_COUNT = 10;
constexpr int NPCS_PER_PORT = 13;
constexpr int MAX_CARGO = 24;
constexpr float DISCOVERY_RANGE = 42.0f;
constexpr float APPROACH_RANGE = 13.0f;
constexpr float DOCK_RANGE = 2.6f;
constexpr float TRANSFER_SECONDS = 3.2f;
constexpr float MAP_HALF_EXTENT = 850.0f;
}

enum class CargoType { FOOD, WOOD, WEAPONS, GUNPOWDER, SPICES, CLOTH, GOLD, RUM, TOOLS, TREASURE, PEARLS, OBSIDIAN, RELICS, CONTRABAND, COUNT };

inline const char* cargoName(CargoType t)
{
    static const char* const names[] = { "Food", "Wood", "Weapons", "Gunpowder", "Spices", "Cloth", "Gold", "Rum", "Tools", "Treasure", "Pearls", "Obsidian", "Relics", "Contraband" };
    return names[static_cast<int>(t)];
}

inline glm::vec3 cargoColour(CargoType t)
{
    static const glm::vec3 colours[] = {
        {0.64f,0.48f,0.22f}, {0.42f,0.22f,0.08f}, {0.25f,0.28f,0.31f}, {0.18f,0.16f,0.14f}, {0.78f,0.36f,0.10f},
        {0.22f,0.42f,0.72f}, {0.88f,0.65f,0.10f}, {0.48f,0.14f,0.08f}, {0.35f,0.42f,0.46f}, {0.94f,0.76f,0.16f},
        {0.83f,0.88f,0.95f}, {0.19f,0.12f,0.25f}, {0.20f,0.65f,0.50f}, {0.50f,0.08f,0.22f}
    };
    return colours[static_cast<int>(t)];
}

struct CargoSystem {
    std::array<int, CampaignConfig::CARGO_COUNT> quantity{};
    int capacity = CampaignConfig::MAX_CARGO;

    int total() const { int n = 0; for (int q : quantity) n += q; return n; }
    int freeSpace() const { return std::max(0, capacity - total()); }
    int count(CargoType t) const { return quantity[static_cast<int>(t)]; }
    bool add(CargoType t, int n = 1) {
        n = std::min(n, freeSpace());
        quantity[static_cast<int>(t)] += n;
        return n > 0;
    }
    bool remove(CargoType t, int n = 1) {
        int& q = quantity[static_cast<int>(t)];
        const int moved = std::min(q, n);
        q -= moved;
        return moved > 0;
    }
};

enum class PortFaction { FRIENDLY, FREE, NAVAL, HOSTILE };

struct PortDefinition {
    const char* name = "";
    glm::vec3 centre{0.0f};
    glm::vec3 dock{0.0f};
    float dockHeading = 0.0f;
    PortFaction faction = PortFaction::FREE;
    std::array<int, CampaignConfig::CARGO_COUNT> stock{};
    std::array<int, CampaignConfig::CARGO_COUNT> demand{};
    glm::vec3 colour{0.5f};
    std::array<int, CampaignConfig::CARGO_COUNT> production{};
    float marketClock = 0.0f;
    float damage = 0.0f;
    bool underAttack = false;
    int relationship = 0;
};

inline PortDefinition makePort(const char* name, glm::vec3 centre, glm::vec3 dock, float heading, PortFaction faction,
                               glm::vec3 colour, std::initializer_list<CargoType> exports, std::initializer_list<CargoType> imports)
{
    PortDefinition p;
    p.name = name; p.centre = centre; p.dock = dock; p.dockHeading = heading; p.faction = faction; p.colour = colour;
    p.stock.fill(0); p.demand.fill(0);
    for (CargoType t : exports) { p.stock[static_cast<int>(t)] = 12; p.production[static_cast<int>(t)] = 1; }
    for (CargoType t : imports) p.demand[static_cast<int>(t)] = 8;
    return p;
}

struct PortSystem {
    std::array<PortDefinition, CampaignConfig::PORT_COUNT> ports;
    std::array<bool, CampaignConfig::PORT_COUNT> discovered{};

    PortSystem()
    {
        ports[0] = makePort("Nassau", {-120.0f,0.0f,-85.0f}, {-109.0f,0.0f,-77.0f}, -2.35f, PortFaction::FRIENDLY,
                            {0.74f,0.57f,0.25f}, {CargoType::FOOD,CargoType::RUM}, {CargoType::TOOLS,CargoType::WEAPONS});
        ports[1] = makePort("Port Royal", {125.0f,0.0f,-90.0f}, {114.0f,0.0f,-82.0f}, 2.28f, PortFaction::NAVAL,
                            {0.55f,0.60f,0.68f}, {CargoType::WEAPONS,CargoType::GUNPOWDER}, {CargoType::FOOD,CargoType::CLOTH});
        ports[2] = makePort("Tortuga", {155.0f,0.0f,105.0f}, {143.0f,0.0f,97.0f}, 0.98f, PortFaction::FREE,
                            {0.58f,0.28f,0.18f}, {CargoType::SPICES,CargoType::CLOTH}, {CargoType::WOOD,CargoType::TOOLS});
        ports[3] = makePort("Havana", {-165.0f,0.0f,120.0f}, {-153.0f,0.0f,111.0f}, -0.86f, PortFaction::HOSTILE,
                            {0.60f,0.20f,0.18f}, {CargoType::GOLD,CargoType::SPICES}, {CargoType::WEAPONS,CargoType::GUNPOWDER});
        ports[4] = makePort("Kingston", {0.0f,0.0f,195.0f}, {0.0f,0.0f,182.0f}, 0.0f, PortFaction::FRIENDLY,
                            {0.30f,0.55f,0.32f}, {CargoType::WOOD,CargoType::FOOD,CargoType::TOOLS}, {CargoType::RUM,CargoType::SPICES});
        const char* names[] = {"Palm Haven","Ember Anchorage","Granite Watch","Cloudfall","Mangrove Market","Old Meridian","Widow's Rest","Veil Harbour"};
        for (int r=0;r<8;++r) {
            const float a=6.2831853f*static_cast<float>(r)/8.0f;
            const glm::vec3 radial(std::sin(a),0,std::cos(a));
            const glm::vec3 centre=radial*(r%2==0?430.0f:620.0f);
            const CargoType rare = r==1?CargoType::OBSIDIAN:(r==0||r==2?CargoType::PEARLS:(r==5||r==7?CargoType::RELICS:CargoType::CONTRABAND));
            ports[5+r]=makePort(names[r],centre,centre-radial*10.0f,a,
                r==2?PortFaction::NAVAL:(r==6?PortFaction::HOSTILE:PortFaction::FREE),
                {0.28f+0.045f*r,0.48f-0.025f*r,0.30f},
                {rare,r%2==0?CargoType::FOOD:CargoType::WOOD}, {CargoType::TOOLS,CargoType::CLOTH,CargoType::RUM});
        }
        discovered[0] = true;
    }

    int nearest(const glm::vec3& at, float* distanceOut = nullptr) const
    {
        int result = -1; float best = 1e9f;
        for (int i = 0; i < CampaignConfig::PORT_COUNT; ++i) {
            const float d = glm::length(glm::vec2(at.x - ports[i].dock.x, at.z - ports[i].dock.z));
            if (d < best) { best = d; result = i; }
        }
        if (distanceOut) *distanceOut = best;
        return result;
    }

    void discoverNear(const glm::vec3& at)
    {
        for (int i = 0; i < CampaignConfig::PORT_COUNT; ++i)
            if (glm::length(glm::vec2(at.x - ports[i].centre.x, at.z - ports[i].centre.z)) < CampaignConfig::DISCOVERY_RANGE)
                discovered[i] = true;
    }
};

enum class DockState { SAILING, APPROACH, MOORING, DOCKED, WORKING, DEPARTING };

struct DockingSystem {
    DockState state = DockState::SAILING;
    int port = -1;
    float stateTime = 0.0f;
    bool arrived = false;

    bool secured() const { return state == DockState::DOCKED || state == DockState::WORKING; }
    float approachSpeedLimit() const { return state == DockState::APPROACH ? 0.9f : (state == DockState::MOORING ? 0.35f : 99.0f); }
    const char* name() const {
        static const char* n[] = { "SAILING", "APPROACH", "MOORING", "DOCKED", "CARGO OPS", "DEPARTING" };
        return n[static_cast<int>(state)];
    }

    void step(const PortSystem& ps, glm::vec3& ship, float& heading, float& speed, bool anchorHolding, bool cargoWorking, float dt)
    {
        arrived = false; stateTime += dt;
        float distance = 0.0f;
        const int nearest = ps.nearest(ship, &distance);
        if (state == DockState::SAILING && distance < CampaignConfig::APPROACH_RANGE) { state = DockState::APPROACH; port = nearest; stateTime = 0.0f; }
        if ((state == DockState::APPROACH || state == DockState::MOORING) && (nearest != port || distance > CampaignConfig::APPROACH_RANGE + 5.0f)) {
            state = DockState::SAILING; port = -1; stateTime = 0.0f; return;
        }
        if (state == DockState::APPROACH) {
            speed = std::clamp(speed, -approachSpeedLimit(), approachSpeedLimit());
            if (distance < 5.0f) { state = DockState::MOORING; stateTime = 0.0f; }
        }
        if (state == DockState::MOORING) {
            speed *= std::exp(-3.5f * dt);
            const PortDefinition& p = ps.ports[port];
            const float k = 1.0f - std::exp(-1.2f * dt);
            ship.x += (p.dock.x - ship.x) * k; ship.z += (p.dock.z - ship.z) * k;
            float delta = std::atan2(std::sin(p.dockHeading - heading), std::cos(p.dockHeading - heading));
            heading += delta * k;
            distance = glm::length(glm::vec2(ship.x - p.dock.x, ship.z - p.dock.z));
            if ((distance < CampaignConfig::DOCK_RANGE && (std::fabs(speed) < 0.18f || anchorHolding)) || stateTime > 7.0f) {
                state = DockState::DOCKED; stateTime = 0.0f; arrived = true; ship.x = p.dock.x; ship.z = p.dock.z; heading = p.dockHeading; speed = 0.0f;
            }
        }
        if (secured()) {
            const PortDefinition& p = ps.ports[port];
            ship.x = p.dock.x; ship.z = p.dock.z; heading = p.dockHeading; speed = 0.0f;
            state = cargoWorking ? DockState::WORKING : DockState::DOCKED;
            if (!anchorHolding && !cargoWorking && stateTime > 1.0f) { state = DockState::DEPARTING; stateTime = 0.0f; }
        } else if (state == DockState::DEPARTING && stateTime > 2.5f) { state = DockState::SAILING; port = -1; stateTime = 0.0f; }
    }
};

struct CargoTransfer {
    bool active = false;
    bool loading = true;
    CargoType type = CargoType::FOOD;
    int requested = 0;
    int completed = 0;
    float cycle = 0.0f;
    int port = -1;

    float phase() const { return std::clamp(cycle / CampaignConfig::TRANSFER_SECONDS, 0.0f, 1.0f); }
};

struct TradingSystem {
    int gold = 250;
    int selected = 0;
    CargoTransfer transfer;

    int price(const PortDefinition& p, CargoType t, bool buying) const
    {
        static const int base[] = { 9,12,34,29,38,20,72,18,24,95,85,65,120,100 };
        const int i = static_cast<int>(t);
        const float scarcity=std::clamp(1.35f-0.035f*p.stock[i]+0.025f*p.demand[i],0.55f,1.85f);
        const float market=(p.production[i]?0.78f:1.15f)*scarcity*(1.0f+0.3f*p.damage);
        const float relation=std::clamp(1.0f-0.003f*p.relationship,0.8f,1.25f);
        return std::max(2, static_cast<int>(base[i]*market*(buying?1.12f*relation:0.82f/relation)));
    }

    bool queue(bool loading, CargoType type, int count, int port)
    {
        if (transfer.active || count <= 0) return false;
        transfer = {}; transfer.active = true; transfer.loading = loading; transfer.type = type; transfer.requested = count; transfer.port = port;
        return true;
    }

    bool step(CargoSystem& hold, PortSystem& ps, float dt)
    {
        if (!transfer.active) return false;
        if (transfer.port < 0 || transfer.port >= CampaignConfig::PORT_COUNT || ps.ports[transfer.port].relationship < -40) { transfer.active=false; return false; }
        transfer.cycle += dt;
        if (transfer.cycle < CampaignConfig::TRANSFER_SECONDS) return false;
        transfer.cycle -= CampaignConfig::TRANSFER_SECONDS;
        PortDefinition& p = ps.ports[transfer.port];
        const int i = static_cast<int>(transfer.type);
        bool moved = false;
        if (transfer.loading) {
            const int cost = price(p, transfer.type, true);
            if (hold.freeSpace() > 0 && p.stock[i] > 0 && gold >= cost) { hold.add(transfer.type); --p.stock[i]; gold -= cost; moved = true; }
        } else if (hold.count(transfer.type) > 0) {
            const int value=price(p,transfer.type,false);
            hold.remove(transfer.type); ++p.stock[i]; p.demand[i]=std::max(0,p.demand[i]-1); gold += value; moved = true;
        }
        if (moved) ++transfer.completed;
        if (!moved || transfer.completed >= transfer.requested) transfer.active = false;
        return moved;
    }

    bool queueMarketTrade(const CargoSystem& hold, const PortSystem& ps, int port)
    {
        if (transfer.active || port < 0) return false;
        const PortDefinition& p = ps.ports[port];
        if(p.relationship < -40 || p.underAttack)return false;
        selected=std::clamp(selected,0,CampaignConfig::CARGO_COUNT-1);
        if(p.demand[selected]>0&&hold.quantity[selected]>0)return queue(false,static_cast<CargoType>(selected),1,port);
        if(p.stock[selected]>0&&hold.freeSpace()>0&&gold>=price(p,static_cast<CargoType>(selected),true))return queue(true,static_cast<CargoType>(selected),1,port);
        for (int i = 0; i < CampaignConfig::CARGO_COUNT; ++i)
            if (p.demand[i] > 0 && hold.quantity[i] > 0) return queue(false, static_cast<CargoType>(i), 1, port);
        for (int i = 0; i < CampaignConfig::CARGO_COUNT; ++i)
            if (p.stock[i] > 0 && hold.freeSpace() > 0 && gold >= price(p, static_cast<CargoType>(i), true)) return queue(true, static_cast<CargoType>(i), 1, port);
        return false;
    }
};

enum class MissionType { DELIVERY, ESCORT, TRADE, RESCUE, STORM, TREASURE, RAID, HUNT, DEFEND, ESCAPE };
enum class MissionStatus { LOCKED, ACTIVE, COMPLETE, FAILED };

struct MissionDefinition {
    MissionType type{};
    const char* title = "";
    const char* objective = "";
    const char* action = "";
    const char* success = "";
    const char* failure = "";
    int startPort = -1, destinationPort = -1;
    CargoType cargo = CargoType::FOOD;
    int cargoCount = 0, rewardGold = 0, rewardReputation = 0;
    glm::vec3 site{0.0f};
};

struct MissionSystem {
    std::array<MissionDefinition, CampaignConfig::MISSION_COUNT> missions;
    MissionStatus status = MissionStatus::ACTIVE;
    int active = 0;
    float progress = 0.0f;
    float goal = 1.0f;
    float elapsed = 0.0f;
    float finishTimer = 0.0f;
    int stage = 0;
    bool justCompleted = false;
    bool justFailed = false;

    MissionSystem()
    {
        missions[0] = {MissionType::DELIVERY,"Relief for Port Royal","Deliver 3 food from Nassau to Port Royal","Dock at Nassau; load; sail east; unload","All food unloaded at Port Royal","Cargo lost or your ship sinks",0,1,CargoType::FOOD,3,180,1,{-2,0,-2}};
        missions[1] = {MissionType::ESCORT,"The Spice Convoy","Escort a merchant from Port Royal to Tortuga","Stay near the merchant throughout the crossing","Merchant reaches Tortuga","Convoy abandoned or destroyed",1,2,CargoType::SPICES,0,240,1,{34,0,4}};
        missions[2] = {MissionType::TRADE,"Caribbean Exchange","Import weapons and export cloth","Buy weapons at Port Royal; sell cloth at Tortuga","Both trades completed","Ship or required goods lost",2,1,CargoType::WEAPONS,2,260,1,{0,0,0}};
        missions[3] = {MissionType::RESCUE,"No Sail on the Horizon","Rescue the stranded ship west of Kingston","Reach the wreck and remain alongside","Survivors safely aboard","Leave the survivors or sink",4,4,CargoType::TOOLS,0,220,1,{-38,0,168}};
        missions[4] = {MissionType::STORM,"The Black Squall","Carry spices to Kingston through a major storm","Keep the cargo aboard and survive rough weather","Dock safely at Kingston","Cargo lost or player ship sinks",2,4,CargoType::SPICES,3,420,2,{0,0,0}};
        missions[5] = {MissionType::TREASURE,"The Cartographer's Secret","Follow the clue and recover hidden treasure","Use the map; close range reveals the exact site","Treasure chest recovered","Player ship sinks",4,-1,CargoType::TREASURE,1,520,2,{-230,0,35}};
        missions[6] = {MissionType::RAID,"Break Havana's Supply Line","Raid the hostile harbour at Havana","Damage the supply ship and port battery","Supply line destroyed","Player ship sinks",3,3,CargoType::GUNPOWDER,0,600,2,{-153,0,111}};
        missions[7] = {MissionType::HUNT,"The Red Jackal","Hunt and defeat Captain Silas Vane","Find the named captain and sink his flagship","Captain Vane defeated","Player ship sinks",3,0,CargoType::GOLD,0,750,3,{-60,0,260}};
        missions[8] = {MissionType::DEFEND,"Stand at Port Royal","Defend the friendly port from attackers","Destroy the attacker within sight of Port Royal","Port survives the attack","Player ship sinks",1,1,CargoType::WEAPONS,0,820,3,{114,0,-82}};
        missions[9] = {MissionType::ESCAPE,"Run the Devil's Channel","Escape the naval pursuit and reach Nassau","Open the range, then dock safely at Nassau","Pursuit escaped; campaign complete","Player ship sinks",1,0,CargoType::TREASURE,0,1200,5,{-109,0,-77}};
    }

    const MissionDefinition& current() const { return missions[active]; }
    float fraction() const { return std::clamp(progress / std::max(goal, 0.001f), 0.0f, 1.0f); }
    void setProgress(float p, float g = 1.0f) { progress = std::max(progress, p); goal = std::max(g, 0.001f); }
    void complete() { if (status != MissionStatus::ACTIVE) return; status = MissionStatus::COMPLETE; finishTimer = 5.0f; justCompleted = true; progress = goal; }
    void fail() { if (status != MissionStatus::ACTIVE) return; status = MissionStatus::FAILED; finishTimer = 4.0f; justFailed = true; }
};

struct TreasureMapSystem {
    bool clueFound = false;
    bool approximate = false;
    bool exact = false;
    bool collected = false;
    glm::vec3 site{-230.0f,0.0f,35.0f};
    float uncertainty = 42.0f;

    void step(const glm::vec3& player)
    {
        if (!clueFound || collected) return;
        approximate = true;
        const float d = glm::length(glm::vec2(player.x - site.x, player.z - site.z));
        uncertainty = std::clamp(d * 0.28f, 1.0f, 42.0f);
        exact = d < 22.0f;
    }
};

enum class PortNpcRole { WORKER, MERCHANT, GUARD, SAILOR, FISHERMAN, CIVILIAN, SMITH };
struct PortNpc { PortNpcRole role = PortNpcRole::WORKER; glm::vec3 pos{0.0f}; glm::vec3 from{0.0f}, to{0.0f}; float phase = 0.0f; bool carrying = false; };

struct NPCSystem {
    std::array<std::array<PortNpc, CampaignConfig::NPCS_PER_PORT>, CampaignConfig::PORT_COUNT> npc{};

    NPCSystem()
    {
        for (int p = 0; p < CampaignConfig::PORT_COUNT; ++p) for (int i = 0; i < CampaignConfig::NPCS_PER_PORT; ++i) {
            PortNpc& n = npc[p][i];
            n.role = i < 5 ? PortNpcRole::WORKER : (i < 7 ? PortNpcRole::MERCHANT : (i<9?PortNpcRole::GUARD:static_cast<PortNpcRole>(i-6)));
            n.phase = 0.17f * static_cast<float>(i) + 0.11f * static_cast<float>(p);
        }
    }

    void step(const PortSystem& ps, const DockingSystem& dock, const CargoTransfer& transfer, float now, float dt=0.016f, bool night=false)
    {
        for (int p = 0; p < CampaignConfig::PORT_COUNT; ++p) {
            const PortDefinition& port = ps.ports[p];
            glm::vec3 shoreDir = glm::normalize(port.centre - port.dock); shoreDir.y = 0.0f;
            glm::vec3 side(-shoreDir.z,0.0f,shoreDir.x);
            for (int i = 0; i < CampaignConfig::NPCS_PER_PORT; ++i) {
                PortNpc& n = npc[p][i];
                const float u = 0.5f + 0.5f * std::sin(now * (0.34f + 0.025f*i) + n.phase * 19.0f);
                if (n.role == PortNpcRole::WORKER) {
                    n.from = port.centre + side * (-4.0f + 1.7f * i) + shoreDir * 0.7f;
                    n.to = port.dock + side * (-1.4f + 0.7f * i);
                    n.carrying = dock.port == p && transfer.active && i < 3;
                    const float trip = n.carrying ? std::fmod(transfer.phase() + 0.22f * i, 1.0f) : u;
                    const glm::vec3 target=glm::mix(n.from, n.to, trip < 0.5f ? trip * 2.0f : (2.0f - trip * 2.0f));
                    if(glm::length(n.pos-port.centre)>25.0f)n.pos=n.from;
                    n.pos+=glm::clamp(target-n.pos,glm::vec3(-dt*1.4f),glm::vec3(dt*1.4f));
                } else {
                    if(n.role==PortNpcRole::FISHERMAN){n.from=port.dock+side*1.4f;n.to=port.dock-side*1.4f;}
                    else if(n.role==PortNpcRole::SMITH){n.from=port.centre+side*5.0f+shoreDir*4.0f;n.to=n.from-side*1.3f;}
                    else if(n.role==PortNpcRole::SAILOR){n.from=port.dock+side*6.0f+shoreDir*1.5f;n.to=port.dock+side*3.0f;}
                    else if(n.role==PortNpcRole::CIVILIAN){n.from=port.centre-side*3.0f-shoreDir*2.7f;n.to=port.centre+side*3.0f-shoreDir*2.7f;}
                    else if(n.role==PortNpcRole::MERCHANT){n.from=port.centre-side*2.8f-shoreDir*2.7f;n.to=n.from+side*1.0f;}
                    else {n.from=port.centre+side*(i%2?4.5f:-4.5f)-shoreDir;n.to=port.dock+side*(i%2?1.8f:-1.8f);}
                    const glm::vec3 target=night&&n.role!=PortNpcRole::GUARD?n.from:glm::mix(n.from,n.to,u);
                    if(glm::length(n.pos-port.centre)>25.0f)n.pos=n.from;
                    n.pos+=glm::clamp(target-n.pos,glm::vec3(-dt*0.8f),glm::vec3(dt*0.8f)); n.carrying = false;
                }
            }
        }
    }
};

struct NavigationSystem {
    bool mapOpen = false;
    bool mapKeyWasDown = false;
    float zoom = 1.0f;
    glm::vec2 pan{0.0f};
    int nearestPort = -1;
    float nearestDistance = 0.0f;
    float targetBearing = 0.0f;
    float targetDistance = 0.0f;
    glm::vec3 target{0.0f};

    static float bearing(const glm::vec3& from, const glm::vec3& to) { return std::atan2(to.x-from.x, to.z-from.z); }
    void update(const PortSystem& ports, const glm::vec3& player, const glm::vec3& destination)
    {
        nearestPort = ports.nearest(player, &nearestDistance);
        target = destination;
        targetBearing = bearing(player, destination);
        targetDistance = glm::length(glm::vec2(destination.x-player.x, destination.z-player.z));
    }
};

struct CampaignSystem {
    PortSystem ports;
    CargoSystem cargo;
    TradingSystem trading;
    DockingSystem docking;
    MissionSystem mission;
    NavigationSystem navigation;
    TreasureMapSystem treasureMap;
    NPCSystem npcs;
    int reputation = 0;
    int completed = 0;
    int lastDockPort = -1;
    int enemySinksSeen = 0;
    int hitsAtMissionStart = 0;
    int cargoMovedThisMission = 0;
    float rescueTime = 0.0f;
    float escortProgress = 0.0f;
    float stormTime = 0.0f;
    bool campaignComplete = false;
    bool advanced = false;

    glm::vec3 destination() const
    {
        const MissionDefinition& m = mission.current();
        if (m.type == MissionType::TREASURE || m.type == MissionType::RESCUE || m.type == MissionType::RAID || m.type == MissionType::HUNT) return m.site;
        const int p = mission.stage == 0 && m.startPort >= 0 ? m.startPort : m.destinationPort;
        return p >= 0 ? ports.ports[p].dock : m.site;
    }

    void beginCurrent()
    {
        mission.status = MissionStatus::ACTIVE; mission.progress = 0.0f; mission.goal = 1.0f; mission.elapsed = 0.0f; mission.finishTimer = 0.0f; mission.stage = 0;
        cargoMovedThisMission = 0; rescueTime = escortProgress = stormTime = 0.0f; hitsAtMissionStart = 0;
        if (mission.current().type == MissionType::TREASURE) { treasureMap.clueFound = true; treasureMap.approximate = true; }
    }

    void advance(float dt)
    {
        advanced = false; mission.justCompleted = mission.justFailed = false;
        if (campaignComplete) return;
        if (mission.status == MissionStatus::ACTIVE) return;
        mission.finishTimer -= dt;
        if (mission.finishTimer > 0.0f) return;
        if (mission.status == MissionStatus::FAILED) { beginCurrent(); advanced = true; return; }
        trading.gold += mission.current().rewardGold; reputation += mission.current().rewardReputation; ++completed;
        if (mission.active + 1 >= CampaignConfig::MISSION_COUNT) { campaignComplete = true; return; }
        ++mission.active; beginCurrent(); advanced = true;
    }
};

