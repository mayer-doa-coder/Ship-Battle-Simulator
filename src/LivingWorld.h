#pragma once

// Bounded, deterministic archipelago simulation. No renderer, GL calls or third-party engine.
// IDs index persistent state, never the visible set: sailing away does not reset loot or ships.
#include "Campaign.h"
#include "Scenery.h"
#include <array>
#include <string>

namespace WorldConfig {
constexpr int REGIONS=8, SITES=48, TRAFFIC=32, EVENTS=8, DROPS=40;
constexpr float ACTIVE_RANGE=165.0f, LANDMARK_RANGE=360.0f, DISCOVER_RANGE=70.0f;
constexpr float ENCOUNTER_INTERVAL=75.0f, RESTOCK_SECONDS=75.0f, SHIP_RESPAWN=600.0f;
constexpr float EVENT_MIN_RANGE=70.0f, EVENT_MAX_RANGE=110.0f;
}
enum class WorldFaction { PIRATE, NAVY, MERCHANT, SMUGGLER, NEUTRAL, SETTLEMENT, COUNT };
enum class RegionKind { TROPICAL, VOLCANIC, ROCKY, MOUNTAIN, SWAMP, RUINED, ABANDONED, MYSTERIOUS };
enum class InterestKind { RUINS, HIDEOUT, FORTRESS, LIGHTHOUSE, WRECK, VILLAGE, CAVE, TEMPLE, CAMP, OUTPOST, TREASURE, HAUNTED };
enum class EncounterKind { ASSIST, SURVIVORS, AMBUSH, INTERCEPTION, DISTRESS, FLOTSAM, SQUALL, CONVOY, SMUGGLERS, DISCOVERY, CLUE, BURNING };
inline const char* worldFactionName(WorldFaction f) {
    static const char* names[]={"Pirates","Navy","Merchants","Smugglers","Sailors","Settlements"}; return names[static_cast<int>(f)];
}
inline const char* regionName(int r) {
    static const char* names[]={"Emerald Keys","Cinder Sea","Iron Reefs","Cloudfall Reach","Mangrove Sound","Meridian Ruins","Widow's Waters","Veil of Echoes"}; return names[std::clamp(r,0,7)];
}
inline const char* interestName(InterestKind k) {
    static const char* names[]={"Ancient ruins","Pirate hideout","Fortress","Lighthouse","Sunken wreck","Abandoned village","Hidden cave","Jungle temple","Smuggler camp","Naval outpost","Treasure island","Haunted shrine"}; return names[static_cast<int>(k)];
}
inline const char* encounterName(EncounterKind k) {
    static const char* names[]={"Merchant needs wood","Rescue survivors","Pirate ambush","Naval inspection","Distress signal","Floating treasure","Squall ahead","Enemy convoy","Smuggler rendezvous","Uncharted island","Map fragment","Burning ship"}; return names[static_cast<int>(k)];
}
inline float worldDistance(glm::vec3 a, glm::vec3 b) { return glm::length(glm::vec2(a.x-b.x,a.z-b.z)); }
inline unsigned worldHash(unsigned x) { x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; return x^(x>>16); }
inline float worldUnit(unsigned x) { return static_cast<float>(worldHash(x)&65535u)/65535.0f; }
inline WorldFaction portWorldFaction(const PortDefinition& p, int index) {
    if(p.faction==PortFaction::NAVAL)return WorldFaction::NAVY;
    if(p.faction==PortFaction::HOSTILE)return WorldFaction::PIRATE;
    if(index==9||index==12)return WorldFaction::SMUGGLER;
    return WorldFaction::SETTLEMENT;
}
struct RegionDefinition { glm::vec3 centre{0}; RegionKind kind{}; WorldFaction faction{}; CargoType resource{}; int risk=1; };
struct WorldSite {
    int region=0; InterestKind kind{}; glm::vec3 centre{0}; float radius=8, height=6, angle=0;
    bool discovered=false, clue=false, solved=false, collected=false, underwater=false;
    int huntStep=0; float harvestAt=0;
    glm::vec3 shore() const { return centre+glm::vec3(std::sin(angle),0,std::cos(angle))*(radius+3.0f); }
    glm::vec3 cluePoint() const { return centre+glm::vec3(std::sin(angle+0.9f),0,std::cos(angle+0.9f))*(radius*0.70f); }
    glm::vec3 cache() const { return centre+glm::vec3(std::sin(angle-0.8f),underwater?-7.0f:0.0f,std::cos(angle-0.8f))*(underwater?1.0f:radius*0.72f); }
};
struct WorldVessel {
    WorldFaction faction=WorldFaction::NEUTRAL; int from=0,to=1, health=8, maxHealth=8, waypoint=0;
    glm::vec3 pos{0}; float heading=0,speed=0,helm=0,fireLeft=8,deadLeft=0,planLeft=0,wanted=0;
    float scale=0.5f; bool hostile=false, fishing=false; float disabled=0;
};
struct WorldEncounter { bool active=false; EncounterKind kind{}; glm::vec3 pos{0}; float left=0,progress=0; int region=0, ship=-1, port=-1; };
struct WorldDrop { bool active=false; glm::vec3 pos{0}; CargoType cargo=CargoType::WOOD; int count=1; float left=0; };
struct WorldContext { glm::vec3 player{0}; float speed=0,wind=0,rain=0; bool night=false; int mission=0; bool missionCombat=false; };

struct LivingWorldSystem {
    std::array<RegionDefinition,WorldConfig::REGIONS> regions{};
    std::array<WorldSite,WorldConfig::SITES> sites{};
    std::array<WorldVessel,WorldConfig::TRAFFIC> ships{};
    std::array<WorldEncounter,WorldConfig::EVENTS> events{};
    std::array<WorldDrop,WorldConfig::DROPS> drops{};
    std::array<int,6> reputation{};
    float elapsed=0,encounterLeft=35,hazardClock=0; unsigned sequence=0;
    int region=0,discoveries=0,artifacts=0,rescued=0,decoration=0,weaponUpgrade=0,lastCampaign=0;
    std::string message;
    bool messagePending=false;
    std::vector<int> visibleSites;

    explicit LivingWorldSystem(const PortSystem& ports=PortSystem()) {
        for(int r=0;r<8;++r) {
            RegionDefinition& reg=regions[r]; reg.centre=ports.ports[5+r].centre; reg.kind=static_cast<RegionKind>(r);
            reg.faction=r==2?WorldFaction::NAVY:(r==6?WorldFaction::PIRATE:(r==4||r==7?WorldFaction::SMUGGLER:WorldFaction::SETTLEMENT));
            reg.resource=r==1?CargoType::OBSIDIAN:(r==0||r==2?CargoType::PEARLS:(r==5||r==7?CargoType::RELICS:CargoType::CONTRABAND));
            reg.risk=1+(r%4);
            for(int n=0;n<6;++n) {
                int id=r*6+n; WorldSite& s=sites[id]; s.region=r;s.kind=static_cast<InterestKind>((r*6+n)%12);
                const float a=6.2831853f*(n/6.0f+0.07f*worldUnit(id+11));
                const float reach=95.0f+worldUnit(id+371)*65.0f;
                s.centre=reg.centre+glm::vec3(std::sin(a),0,std::cos(a))*reach;
                s.radius=7.0f+worldUnit(id+982)*7.0f; s.height=4.0f+worldUnit(id+223)*7.0f;
                if(r==1||r==3)s.height+=12.0f;
                s.angle=a+3.14159265f; s.underwater=s.kind==InterestKind::WRECK||n==5;
            }
        }
        for(int i=0;i<WorldConfig::TRAFFIC;++i) {
            WorldVessel& s=ships[i];s.from=i%CampaignConfig::PORT_COUNT;s.to=(s.from+1+i%3)%CampaignConfig::PORT_COUNT;
            s.faction=static_cast<WorldFaction>(i%6);s.fishing=i%7==0;s.scale=s.fishing?0.30f:0.48f+0.04f*(i%3);
            s.maxHealth=s.health=s.faction==WorldFaction::NAVY?14:8;
            // Stagger departures well outside their berths. Multiple routes never begin as one
            // pile of hulls, and the player sees hostile traffic approaching before gun range.
            const glm::vec3 seaward=glm::normalize(ports.ports[s.from].dock-ports.ports[s.from].centre);
            const glm::vec3 laneSide(-seaward.z,0.0f,seaward.x);
            const float lane=static_cast<float>(i/13)-1.0f;
            s.pos=ports.ports[s.from].dock+seaward*32.0f+laneSide*(lane*24.0f);
            s.heading=std::atan2(ports.ports[s.to].dock.x-s.pos.x,ports.ports[s.to].dock.z-s.pos.z);
        }
    }
    int regionAt(glm::vec3 p) const { int best=0;float d=1e9f;for(int i=0;i<8;++i){float v=worldDistance(p,regions[i].centre);if(v<d){d=v;best=i;}}return best; }
    int nearestSite(glm::vec3 p, float range) const { int result=-1;for(int i=0;i<WorldConfig::SITES;++i){float d=worldDistance(p,sites[i].centre);if(d<range){range=d;result=i;}}return result; }
    void adjust(WorldFaction f,int amount) { int& r=reputation[static_cast<int>(f)];r=std::clamp(r+amount,-100,100); }
    void appendSolids(std::vector<Obstacle>& out,glm::vec3 near,float range=140) const {
        for(const WorldSite& s:sites)if(!s.underwater&&worldDistance(near,s.centre)<range+s.radius){
            // Three lobes leave a seaward cove; the collision and visible land use the very same lobes.
            for(int k=-1;k<=1;++k){float a=s.angle+3.14159265f+k*0.85f;glm::vec3 c=s.centre+glm::vec3(std::sin(a),0,std::cos(a))*s.radius*0.40f;
                out.push_back({c.x,c.z,s.radius*(k==0?0.64f:0.50f),s.height});}
        }
    }
    float groundAt(const WorldSite& s,glm::vec3 p) const {
        float h=0.12f;
        for(int k=-1;k<=1;++k){float a=s.angle+3.14159265f+k*0.85f;glm::vec3 c=s.centre+glm::vec3(std::sin(a),0,std::cos(a))*s.radius*0.40f;
            float r=s.radius*(k==0?0.64f:0.50f),d=worldDistance(c,p);
            if(d<r)h=std::max(h,s.height*(k==0?1.0f:0.7f)*std::sqrt(std::max(0.0f,1-d*d/(r*r))));}
        return s.underwater?-7.5f:h;
    }
    void drop(glm::vec3 p,CargoType cargo,int count=1) {
        for(WorldDrop& d:drops)if(!d.active){d={true,p,cargo,count,240};return;}
    }
    bool hit(glm::vec3 p,int damage,int owner) {
        for(int i=0;i<WorldConfig::TRAFFIC;++i){WorldVessel& s=ships[i];if(s.health<=0||owner==100+i)continue;
            glm::vec3 d=p-s.pos;float along=d.x*std::sin(s.heading)+d.z*std::cos(s.heading),across=d.x*std::cos(s.heading)-d.z*std::sin(s.heading);
            if(std::fabs(along)>3.0f*s.scale||std::fabs(across)>1.15f*s.scale||std::fabs(d.y)>1.7f*s.scale)continue;
            s.health=std::max(0,s.health-damage);s.hostile=true;
            if(owner==0){adjust(s.faction,-4);if(s.faction==WorldFaction::PIRATE)adjust(WorldFaction::MERCHANT,1);}
            if(s.health==0){s.deadLeft=WorldConfig::SHIP_RESPAWN;drop(s.pos,s.faction==WorldFaction::SMUGGLER?CargoType::CONTRABAND:CargoType::TOOLS,2);}
            return true;
        }return false;
    }
    bool cargoLoss(CampaignSystem& c,glm::vec3 at) {
        for(int i=CampaignConfig::CARGO_COUNT-1;i>=0;--i)if(c.cargo.quantity[i]>0){
            c.cargo.remove(static_cast<CargoType>(i));drop(at+glm::vec3(2,0,1),static_cast<CargoType>(i));message="Cargo overboard - recover it with F";messagePending=true;return true;}
        return false;
    }
    EncounterKind chooseEncounter(const WorldContext& c) const {
        if(c.rain>0.65f)return sequence%2?EncounterKind::SURVIVORS:EncounterKind::BURNING;
        if(c.night)return sequence%2?EncounterKind::SMUGGLERS:EncounterKind::CLUE;
        if(c.missionCombat)return EncounterKind::FLOTSAM;
        if(reputation[1]<-15||c.mission==9)return EncounterKind::INTERCEPTION;
        if(region==2)return sequence%2?EncounterKind::CONVOY:EncounterKind::DISTRESS;
        if(region==4||region==5)return sequence%2?EncounterKind::CLUE:EncounterKind::DISCOVERY;
        return static_cast<EncounterKind>(sequence%12);
    }
    void encounter(const WorldContext& ctx,const std::vector<Obstacle>& solids,CampaignSystem& c) {
        int slot=-1;for(int i=0;i<WorldConfig::EVENTS;++i)if(!events[i].active){slot=i;break;}if(slot<0)return;
        WorldEncounter e;e.kind=chooseEncounter(ctx);e.region=region;e.left=150;
        bool placed=false;
        for(int attempt=0;attempt<24;++attempt){float a=worldUnit(++sequence)*6.2831853f;
            e.pos=ctx.player+glm::vec3(std::sin(a),0,std::cos(a))*(WorldConfig::EVENT_MIN_RANGE+worldUnit(sequence+591)*(WorldConfig::EVENT_MAX_RANGE-WorldConfig::EVENT_MIN_RANGE));
            if(!shipTouchesLand(solids,e.pos,a,-1.5f)){placed=true;break;}}
        if(!placed)return;
        e.active=true;
        if(e.kind==EncounterKind::AMBUSH||e.kind==EncounterKind::CONVOY||e.kind==EncounterKind::INTERCEPTION){
            // Convert a nearby live traffic vessel; destroyed hulls cannot be resurrected by an encounter.
            for(int i=0;i<WorldConfig::TRAFFIC;++i){auto& v=ships[i];const float d=worldDistance(ctx.player,v.pos);if(v.health>0&&d>WorldConfig::EVENT_MIN_RANGE&&d<145.0f){
                e.ship=i;v.hostile=true;v.faction=e.kind==EncounterKind::INTERCEPTION?WorldFaction::NAVY:WorldFaction::PIRATE;e.pos=v.pos;break;}}
            if(e.ship<0)e.kind=EncounterKind::DISTRESS;
        }
        if(e.kind==EncounterKind::AMBUSH){float d;int p=c.ports.nearest(e.pos,&d);if(d<50){e.port=p;c.ports.ports[p].underAttack=true;}}
        events[slot]=e;message=encounterName(e.kind);messagePending=true;
    }
    void step(CampaignSystem& c,const WorldContext& ctx,float dt,const std::vector<Obstacle>& solids) {
        if(dt<=0)return;elapsed+=dt;region=regionAt(ctx.player);visibleSites.clear();
        for(int i=0;i<WorldConfig::SITES;++i){auto& s=sites[i];float d=worldDistance(ctx.player,s.centre);
            if(d<WorldConfig::LANDMARK_RANGE+s.radius)visibleSites.push_back(i);
            if(d<WorldConfig::DISCOVER_RANGE&&!s.discovered){s.discovered=true;++discoveries;message=std::string("Discovered: ")+interestName(s.kind);messagePending=true;c.trading.gold+=15;}}
        if(c.completed>lastCampaign){adjust(c.completed==9?WorldFaction::NAVY:WorldFaction::SETTLEMENT,5);lastCampaign=c.completed;}
        for(int p=0;p<CampaignConfig::PORT_COUNT;++p){auto& port=c.ports.ports[p];port.relationship=reputation[static_cast<int>(portWorldFaction(port,p))];port.marketClock+=dt;
            if(port.underAttack)port.damage=std::min(1.0f,port.damage+dt*0.002f);else port.damage=std::max(0.0f,port.damage-dt*0.0008f);
            if(port.marketClock>=WorldConfig::RESTOCK_SECONDS){port.marketClock-=WorldConfig::RESTOCK_SECONDS;
                for(int t=0;t<CampaignConfig::CARGO_COUNT;++t){if(port.production[t])port.stock[t]=std::min(30,port.stock[t]+(port.underAttack?0:2));
                    else {port.stock[t]=std::max(0,port.stock[t]-1);port.demand[t]=std::min(12,port.demand[t]+1);}}}}
        encounterLeft-=dt;if(encounterLeft<=0){encounterLeft=WorldConfig::ENCOUNTER_INTERVAL+worldUnit(++sequence)*25;encounter(ctx,solids,c);}
        for(auto& e:events)if(e.active){e.left-=dt;if(e.ship>=0&&ships[e.ship].health<=0){c.trading.gold+=60;adjust(WorldFaction::SETTLEMENT,3);e.left=0;}
            if(e.left<=0){if(e.port>=0)c.ports.ports[e.port].underAttack=false;e.active=false;}}
        for(auto& d:drops)if(d.active){d.left-=dt;d.pos.x+=0.03f*dt; if(d.left<=0)d.active=false;}
        for(int i=0;i<WorldConfig::TRAFFIC;++i){auto& s=ships[i];
            if(s.health<=0){s.deadLeft-=dt;if(s.deadLeft<=0&&worldDistance(s.pos,ctx.player)>120){s.health=s.maxHealth;s.pos=c.ports.ports[s.from].dock+glm::normalize(c.ports.ports[s.from].dock-c.ports.ports[s.from].centre)*10.0f;s.hostile=false;}continue;}
            if(worldDistance(s.pos,ctx.player)>WorldConfig::ACTIVE_RANGE)continue; // sleeping ships retain all state; bounded local simulation
            s.disabled=std::max(0.0f,s.disabled-dt);s.fireLeft=std::max(0.0f,s.fireLeft-dt);s.planLeft-=dt;
            glm::vec3 goal=c.ports.ports[s.to].dock+glm::normalize(c.ports.ports[s.to].dock-c.ports.ports[s.to].centre)*10.0f;
            bool aggressive=s.hostile||(s.faction==WorldFaction::PIRATE&&reputation[0]<15)||(s.faction==WorldFaction::NAVY&&reputation[1]<-20);
            if(aggressive&&worldDistance(ctx.player,s.pos)<90)goal=ctx.player;
            if(worldDistance(s.pos,goal)<7&&!aggressive){std::swap(s.from,s.to);s.disabled=5; c.ports.ports[s.to].stock[0]=std::min(30,c.ports.ports[s.to].stock[0]+1);}
            if(s.planLeft<=0){s.planLeft=0.45f;float desired=std::atan2(goal.x-s.pos.x,goal.z-s.pos.z);
                if(aggressive&&worldDistance(ctx.player,s.pos)<18)desired+=1.4f;
                ShipCoursePlan plan=planShipCourse(solids,s.pos,s.heading,desired,s.speed,i%2?1:-1);s.wanted=plan.heading;s.speed=plan.reverse?-0.45f:std::max(0.35f,plan.speedScale)*(s.fishing?0.5f:1.1f);}
            s.heading+=std::clamp(navigationWrap(s.wanted-s.heading),-0.5f*dt,0.5f*dt);
            if(s.disabled<=0){glm::vec3 next=s.pos+glm::vec3(std::sin(s.heading),0,std::cos(s.heading))*s.speed*dt;
                if(!shipTouchesLand(solids,next,s.heading))s.pos=next;else{s.speed=-0.5f;s.planLeft=0;}}
        }
    }
    // F is shared with campaign interactions. Only a physically reachable object consumes it.
    bool interact(CampaignSystem& c,glm::vec3 actor,glm::vec3 ship,bool onFoot,bool diving,float seaY) {
        for(auto& d:drops)if(d.active&&worldDistance(actor,d.pos)<3.5f&&!diving){if(c.cargo.freeSpace()<d.count){message="Hold full";return true;}c.cargo.add(d.cargo,d.count);d.active=false;message="Recovered floating cargo";return true;}
        for(auto& e:events)if(e.active&&worldDistance(ship,e.pos)<7&&!diving){
            if(e.kind==EncounterKind::AMBUSH||e.kind==EncounterKind::CONVOY){message="Defeat the marked vessel";return true;}
            if(e.kind==EncounterKind::ASSIST&&!c.cargo.remove(CargoType::WOOD)){message="Bring one wood to repair the merchant";return true;}
            if(e.kind==EncounterKind::BURNING&&!c.cargo.remove(CargoType::TOOLS)){message="Bring tools to save the burning ship";return true;}
            if(e.kind==EncounterKind::INTERCEPTION){if(c.cargo.count(CargoType::CONTRABAND)){c.cargo.remove(CargoType::CONTRABAND);adjust(WorldFaction::NAVY,-5);message="Patrol confiscated contraband";}else{adjust(WorldFaction::NAVY,2);message="Inspection cleared";}if(e.ship>=0)ships[e.ship].hostile=false;}
            else if(e.kind==EncounterKind::SMUGGLERS){if(c.trading.gold<45||c.cargo.freeSpace()==0){message="Smugglers ask 45 gold and free cargo space";return true;}c.trading.gold-=45;c.cargo.add(CargoType::CONTRABAND);adjust(WorldFaction::SMUGGLER,3);message="Contraband acquired";}
            else if(e.kind==EncounterKind::CLUE||e.kind==EncounterKind::DISCOVERY){int id=nearestSite(actor,200);if(id>=0){sites[id].clue=true;sites[id].discovered=true;message="Clue: inspect the paired stones, then search west";}else message="Sail outward to the regional landmarks";}
            else if(e.kind==EncounterKind::SQUALL){message="Ride out the squall or sail clear";return true;}
            else if(e.kind==EncounterKind::FLOTSAM){if(c.cargo.freeSpace()==0){message="Make room for floating treasure";return true;}c.cargo.add(CargoType::GOLD);c.trading.gold+=65;message="Floating treasure recovered";}
            else {if(c.cargo.freeSpace()==0){message="Make room for the survivors' supplies";return true;}c.cargo.add(CargoType::TOOLS);c.trading.gold+=40;adjust(WorldFaction::MERCHANT,4);++rescued;message="Assistance complete - supplies and reputation gained";}
            e.active=false;if(e.port>=0)c.ports.ports[e.port].underAttack=false;return true;
        }
        if(!onFoot)return false;
        for(auto& s:sites)if(worldDistance(actor,s.centre)<s.radius+8){
            if(!s.clue&&worldDistance(actor,s.shore())<4){s.clue=true;s.huntStep=1;message="Clue: find the paired stones inland; then search west";return true;}
            if(s.clue&&!s.solved&&worldDistance(actor,s.cluePoint())<2.5f){s.solved=true;s.huntStep=2;message=s.underwater?"The wreck below the stones holds the cache - dive":"Search west of the paired stones for buried cargo";return true;}
            glm::vec3 cache=s.cache();cache.y=seaY+(s.underwater?-7.0f:groundAt(s,cache)+0.5f);
            if(s.solved&&!s.collected&&glm::length(actor-cache)<3.0f&&(!s.underwater||diving)){
                if(c.cargo.freeSpace()==0){message="Hold full - cache remains here";return true;}
                s.collected=true;s.huntStep=3;++artifacts;s.harvestAt=elapsed+180;c.cargo.add(CargoType::RELICS);c.trading.gold+=80+regions[s.region].risk*35;
                if(s.kind==InterestKind::FORTRESS||s.kind==InterestKind::OUTPOST)weaponUpgrade=1;else decoration=1;
                message="Cache recovered - artifact and ship reward unlocked";return true;
            }
            if(s.collected&&elapsed>=s.harvestAt&&worldDistance(actor,s.shore())<3){if(c.cargo.freeSpace()==0){message="Hold full";return true;}c.cargo.add(regions[s.region].resource);s.harvestAt=elapsed+180;message="Regional resource gathered";return true;}
        }return false;
    }
};
