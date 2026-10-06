#pragma once

// Procedural renderer for Campaign.h.  It is included by main.cpp after drawMesh(), CrewDraw.h
// and the primitive meshes exist.  No model, texture, font, UI toolkit or particle library is used.

#include "Campaign.h"
#include "CrewDraw.h"
#include "Material.h"

inline Material campaignMaterial(const glm::vec3& colour, const char* name = "campaign material", float shine = 7.0f)
{
    Material m;
    m.ka = colour * 0.32f; m.kd = colour; m.ks = glm::vec3(0.05f); m.ns = shine; m.name = name;
    return m;
}

inline glm::mat4 campaignBoxBetween(const glm::vec3& a, const glm::vec3& b, const glm::vec3& thickness)
{
    const glm::vec3 d = b - a;
    const float length = std::max(glm::length(glm::vec2(d.x,d.z)), 0.001f);
    const float yaw = std::atan2(d.x,d.z);
    return glm::translate(glm::mat4(1.0f), 0.5f*(a+b))
         * glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0,1,0))
         * glm::scale(glm::mat4(1.0f), glm::vec3(thickness.x, thickness.y, length));
}

inline void drawCampaignCargoPiece(ShaderProgram& shader, RenderStats& stats, const Mesh& cube, const Mesh& cylinder,
                                   const glm::mat4& frame, CargoType type, float scale = 1.0f)
{
    const Material mat = campaignMaterial(cargoColour(type), cargoName(type));
    const bool barrel = type == CargoType::FOOD || type == CargoType::GUNPOWDER || type == CargoType::RUM || type == CargoType::SPICES;
    if (barrel) {
        drawMesh(shader, stats, cylinder, frame * glm::scale(glm::mat4(1), glm::vec3(0.30f,0.42f,0.30f)*scale), mat);
        drawMesh(shader, stats, cylinder, frame * glm::translate(glm::mat4(1),glm::vec3(0,0.15f*scale,0))*glm::scale(glm::mat4(1),glm::vec3(0.315f,0.035f,0.315f)*scale), IRON_DARK);
        drawMesh(shader, stats, cylinder, frame * glm::translate(glm::mat4(1),glm::vec3(0,-0.15f*scale,0))*glm::scale(glm::mat4(1),glm::vec3(0.315f,0.035f,0.315f)*scale), IRON_DARK);
    } else {
        drawMesh(shader, stats, cube, frame * glm::scale(glm::mat4(1), glm::vec3(0.34f,0.30f,0.34f)*scale), mat);
        drawMesh(shader, stats, cube, frame * glm::translate(glm::mat4(1),glm::vec3(0,0.02f*scale,0))*glm::scale(glm::mat4(1),glm::vec3(0.37f,0.035f,0.37f)*scale), HEMP);
    }
}

inline void drawPortBuildings(ShaderProgram& shader, RenderStats& stats, const Mesh& cube, const Mesh& cylinder,
                              const PortDefinition& p, int portIndex, float seaY, bool distant, float now)
{
    glm::vec3 inward = glm::normalize(p.centre-p.dock); inward.y = 0.0f;
    glm::vec3 side(-inward.z,0.0f,inward.x);
    const Material paint = campaignMaterial(p.colour, "port paint");
    const Material roof = campaignMaterial(p.faction == PortFaction::HOSTILE ? glm::vec3(0.20f,0.04f,0.035f) : glm::vec3(0.24f,0.12f,0.055f), "roof");

    // A low quay/shore gives every harbour a readable silhouette before its workers are visible.
    drawMesh(shader, stats, cube, glm::translate(glm::mat4(1),p.centre+glm::vec3(0,seaY+0.18f,0))*glm::scale(glm::mat4(1),glm::vec3(13.0f,0.45f,10.0f)), SAND);
    const glm::vec3 pierEnd = p.dock - inward*1.1f;
    const glm::vec3 pierStart = p.centre - inward*1.0f;
    drawMesh(shader, stats, cube, campaignBoxBetween(pierStart+glm::vec3(0,seaY+0.42f,0),pierEnd+glm::vec3(0,seaY+0.42f,0),glm::vec3(2.0f,0.22f,1)), DECK_PLANK);
    for (int i=0;i<6;++i) {
        const float t=static_cast<float>(i)/5.0f; const glm::vec3 q=glm::mix(pierStart,pierEnd,t);
        for (int s=-1;s<=1;s+=2) drawMesh(shader,stats,cylinder,glm::translate(glm::mat4(1),q+side*(1.0f*s)+glm::vec3(0,seaY-0.05f,0))*glm::scale(glm::mat4(1),glm::vec3(0.18f,1.5f,0.18f)),WEATHERED_BROWN);
    }

    const int buildings = distant ? 3 : 6;
    for (int i=0;i<buildings;++i) {
        const float sx=(i%3-1)*4.1f, sz=(i/3)*3.3f;
        const glm::vec3 c=p.centre+side*sx+inward*(sz+1.5f)+glm::vec3(0,seaY,0);
        const float h=1.8f+0.35f*static_cast<float>((i+portIndex)%3);
        drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),c+glm::vec3(0,0.45f*h,0))*glm::scale(glm::mat4(1),glm::vec3(3.1f,h,2.35f)),i<2?WEATHERED_BROWN:paint);
        drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),c+glm::vec3(0,h+0.08f,0))*glm::rotate(glm::mat4(1),glm::radians(45.0f),glm::vec3(0,0,1))*glm::scale(glm::mat4(1),glm::vec3(2.45f,0.24f,2.65f)),roof);
        if (!distant && i<2) {
            int shown=0;
            for(int t=0;t<CampaignConfig::CARGO_COUNT&&shown<3;++t) if(p.stock[t]>0) {
                drawCampaignCargoPiece(shader,stats,cube,cylinder,glm::translate(glm::mat4(1),c+side*(-0.8f+0.8f*shown)+inward*(-1.55f)+glm::vec3(0,0.28f,0)),static_cast<CargoType>(t),0.8f);
                ++shown;
            }
        }
    }

    if(!distant){
        // A readable working waterfront: three market awnings, a blacksmith chimney, tavern sign and repair stocks. Roles and schedules in NPCSystem
        // use these same parts of the settlement, so they are gameplay landmarks rather than a dense pile of unrelated decoration.
        for(int i=0;i<3;++i){const glm::vec3 stall=p.centre+side*(-3.0f+3.0f*i)-inward*2.7f+glm::vec3(0,seaY,0);
            drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),stall+glm::vec3(0,0.55f,0))*glm::scale(glm::mat4(1),glm::vec3(2.2f,0.12f,1.5f)),i%2?SAIL_CREAM:paint);
            drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),stall+glm::vec3(0,0.22f,0))*glm::scale(glm::mat4(1),glm::vec3(1.8f,0.42f,1.1f)),WEATHERED_BROWN);}
        const glm::vec3 smith=p.centre+side*5.0f+inward*4.0f+glm::vec3(0,seaY,0);
        drawMesh(shader,stats,cylinder,glm::translate(glm::mat4(1),smith+glm::vec3(0,2.7f,0))*glm::scale(glm::mat4(1),glm::vec3(0.42f,3.4f,0.42f)),CHARCOAL);
        drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),smith+side*1.1f+glm::vec3(0,0.65f,0))*glm::scale(glm::mat4(1),glm::vec3(1.1f,0.35f,0.7f)),IRON_DARK);
        const glm::vec3 tavern=p.centre-side*4.6f+inward*2.5f+glm::vec3(0,seaY+1.4f,0);
        drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),tavern-side*1.8f)*glm::scale(glm::mat4(1),glm::vec3(0.12f,1.3f,0.12f)),WEATHERED_BROWN);
        drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),tavern-side*2.2f)*glm::scale(glm::mat4(1),glm::vec3(0.85f,0.55f,0.12f)),GOLD);
        const glm::vec3 yard=p.dock+side*6.0f+inward*1.5f+glm::vec3(0,seaY,0);
        for(int i=-1;i<=1;i+=2){drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),yard+side*(1.5f*i)+glm::vec3(0,1.2f,0))*glm::scale(glm::mat4(1),glm::vec3(0.25f,2.4f,4.5f)),WEATHERED_BROWN);
            drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),yard+side*(1.5f*i)+glm::vec3(0,2.4f,0))*glm::scale(glm::mat4(1),glm::vec3(0.40f,0.22f,4.8f)),IRON_DARK);}
        if(p.damage>0.05f)for(int i=0;i<4;++i)drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),p.centre+side*(-3.0f+2.0f*i)+inward*0.8f+glm::vec3(0,seaY+0.25f,0))*glm::rotate(glm::mat4(1),0.4f*i,glm::vec3(0,1,0))*glm::scale(glm::mat4(1),glm::vec3(1.3f,0.28f,0.35f)),CHARCOAL);
    }

    // Lighthouse/watch tower, flag, and a compact loading merchant at the second berth.
    const glm::vec3 tower=p.centre+side*5.2f-inward*1.8f+glm::vec3(0,seaY,0);
    drawMesh(shader,stats,cylinder,glm::translate(glm::mat4(1),tower+glm::vec3(0,1.8f,0))*glm::scale(glm::mat4(1),glm::vec3(1.4f,3.6f,1.4f)),CLIFF_ROCK);
    drawMesh(shader,stats,cylinder,glm::translate(glm::mat4(1),tower+glm::vec3(0,3.8f,0))*glm::scale(glm::mat4(1),glm::vec3(0.12f,2.0f,0.12f)),HEMP);
    const glm::vec3 berth=p.dock+side*3.2f-inward*0.7f+glm::vec3(0,seaY+0.12f+0.07f*std::sin(now+portIndex),0);
    drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),berth)*glm::rotate(glm::mat4(1),p.dockHeading,glm::vec3(0,1,0))*glm::scale(glm::mat4(1),glm::vec3(1.25f,0.45f,3.1f)),HULL_TIMBER);
    drawMesh(shader,stats,cylinder,glm::translate(glm::mat4(1),berth+glm::vec3(0,1.25f,0))*glm::scale(glm::mat4(1),glm::vec3(0.12f,2.4f,0.12f)),WEATHERED_BROWN);
    drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),berth+glm::vec3(0,1.65f,0))*glm::rotate(glm::mat4(1),p.dockHeading,glm::vec3(0,1,0))*glm::scale(glm::mat4(1),glm::vec3(1.15f,1.15f,0.06f)),SAILCLOTH);
}

inline void drawCampaignPorts(ShaderProgram& shader, RenderStats& stats, const Mesh& sphere, const Mesh& cylinder, const Mesh& cube,
                              const CampaignSystem& campaign, const glm::vec3& eye, float seaY, float now, bool crewVisible)
{
    for (int p=0;p<CampaignConfig::PORT_COUNT;++p) {
        const PortDefinition& port=campaign.ports.ports[p];
        const float distance=glm::length(glm::vec2(eye.x-port.centre.x,eye.z-port.centre.z));
        if(distance>150.0f) continue;
        const bool distant=distance>65.0f;
        drawPortBuildings(shader,stats,cube,cylinder,port,p,seaY,distant,now);
        if(distant || !crewVisible) continue;
        CrewContext ctx; ShipCrew dummy; dummy.ctx=ctx;
        const CrewDrawContext dc{shader,stats,sphere,cylinder,cube,0.8f,now};
        const int lod=distance>38.0f?2:(distance>20.0f?1:0);
        for(int i=0;i<CampaignConfig::NPCS_PER_PORT;++i) {
            const PortNpc& n=campaign.npcs.npc[p][i];
            CrewMember m; m.variant=(p*3+i)%8; m.pos=n.pos+glm::vec3(0,seaY+0.46f,0);
            const glm::vec3 d=n.to-n.from; m.facing=std::atan2(d.x,d.z); m.anim=now*(n.role==PortNpcRole::WORKER?1.6f:0.8f)+i;
            m.walkPhase=m.anim*5.0f; m.moving=true; m.carrying=n.carrying;
            m.role=n.role==PortNpcRole::GUARD?CrewRole::MARINE:(n.role==PortNpcRole::MERCHANT?CrewRole::FIRST_MATE:CrewRole::DECK_CREW);
            m.task=n.role==PortNpcRole::GUARD?CrewTask::PATROL_STAND:(n.carrying?CrewTask::CARRY:CrewTask::WALK);
            drawCrewMember(dc,glm::mat4(1.0f),m,ctx,dummy,lod,n.role==PortNpcRole::GUARD&&(port.faction==PortFaction::HOSTILE||port.relationship<-20));
            if(n.carrying && campaign.trading.transfer.active)
                drawCampaignCargoPiece(shader,stats,cube,cylinder,glm::translate(glm::mat4(1),m.pos+glm::vec3(0,0.28f,0)),campaign.trading.transfer.type,0.54f);
        }
    }
}

inline void drawCampaignShipCargo(ShaderProgram& shader, RenderStats& stats, const Mesh& cube, const Mesh& cylinder,
                                  const CampaignSystem& campaign, const glm::mat4& hull, const CrewLayout& layout)
{
    int slot=0;
    for(int t=0;t<CampaignConfig::CARGO_COUNT && slot<14;++t) for(int q=0;q<campaign.cargo.quantity[t] && slot<14;++q,++slot) {
        const int row=slot/7, col=slot%7;
        const glm::vec3 at(-0.36f+0.12f*col,layout.waistY+0.18f+0.22f*row,0.12f-0.18f*row);
        drawCampaignCargoPiece(shader,stats,cube,cylinder,hull*glm::translate(glm::mat4(1),at),static_cast<CargoType>(t),0.48f);
    }
}

inline void drawCampaignTreasureSite(ShaderProgram& shader, RenderStats& stats, const Mesh& cube, const Mesh& cylinder,
                                     const CampaignSystem& campaign, float seaY, float now)
{
    const TreasureMapSystem& map=campaign.treasureMap;
    if(!map.clueFound || map.collected) return;
    const glm::vec3 at=map.site+glm::vec3(0,seaY,0);
    // The landmark can be recognized from the approximate clue; the chest itself appears only once the bearing has narrowed.
    drawMesh(shader,stats,cylinder,glm::translate(glm::mat4(1),at+glm::vec3(0,1.1f,0))*glm::scale(glm::mat4(1),glm::vec3(1.0f,2.2f,1.0f)),CLIFF_ROCK);
    drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),at+glm::vec3(0,2.35f,0))*glm::rotate(glm::mat4(1),0.35f*std::sin(now),glm::vec3(0,1,0))*glm::scale(glm::mat4(1),glm::vec3(1.5f,0.15f,0.15f)),HEMP);
    if(map.exact) {
        drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),at+glm::vec3(1.2f,0.25f,0.8f))*glm::scale(glm::mat4(1),glm::vec3(0.75f,0.42f,0.52f)),CHEST_WOOD);
        drawMesh(shader,stats,cube,glm::translate(glm::mat4(1),at+glm::vec3(1.2f,0.48f,0.8f))*glm::scale(glm::mat4(1),glm::vec3(0.78f,0.09f,0.55f)),GOLD);
    }
}

inline void drawCampaignMissionActors(ShaderProgram& shader, RenderStats& stats, const Mesh& cube, const Mesh& cylinder,
                                      const CampaignSystem& campaign, float seaY, float now)
{
    const MissionDefinition& m=campaign.mission.current();
    if(campaign.mission.status!=MissionStatus::ACTIVE) return;
    glm::vec3 pos(0.0f); float heading=0.0f; bool ship=false; bool stranded=false;
    if(m.type==MissionType::ESCORT && campaign.mission.stage>0) {
        const glm::vec3 a=campaign.ports.ports[m.startPort].dock,b=campaign.ports.ports[m.destinationPort].dock;
        pos=glm::mix(a,b,std::clamp(campaign.escortProgress,0.0f,1.0f)); heading=std::atan2(b.x-a.x,b.z-a.z); ship=true;
    } else if(m.type==MissionType::RESCUE) { pos=m.site; heading=-0.55f; ship=true; stranded=true; }
    if(!ship)return;
    pos.y=seaY+0.12f+(stranded?0.0f:0.08f*std::sin(now*1.4f));
    const glm::mat4 root=glm::translate(glm::mat4(1),pos)*glm::rotate(glm::mat4(1),heading,glm::vec3(0,1,0));
    drawMesh(shader,stats,cube,root*glm::scale(glm::mat4(1),glm::vec3(1.05f,0.45f,2.8f)),stranded?CHARCOAL:HULL_TIMBER);
    drawMesh(shader,stats,cylinder,root*glm::translate(glm::mat4(1),glm::vec3(0,1.25f,0))*glm::rotate(glm::mat4(1),stranded?glm::radians(64.0f):0.0f,glm::vec3(0,0,1))*glm::scale(glm::mat4(1),glm::vec3(0.10f,2.4f,0.10f)),WEATHERED_BROWN);
    if(!stranded) drawMesh(shader,stats,cube,root*glm::translate(glm::mat4(1),glm::vec3(0,1.55f,0))*glm::scale(glm::mat4(1),glm::vec3(1.25f,1.15f,0.055f)),SAILCLOTH);
    else {
        for(int i=0;i<3;++i) drawMesh(shader,stats,cylinder,glm::translate(glm::mat4(1),pos+glm::vec3(-0.65f+0.65f*i,0.18f,1.8f))*glm::scale(glm::mat4(1),glm::vec3(0.18f,0.36f,0.18f)),crewCloth(glm::vec3(0.65f,0.18f+0.12f*i,0.08f)));
    }
}

