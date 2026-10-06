#pragma once
#include "LivingWorld.h"

// Existing primitive/hull meshes only. Distance gates bound the detailed work; silhouettes remain visible farther out.
inline void drawLivingWorld(ShaderProgram& shader,RenderStats& stats,const Mesh& sphere,const Mesh& cylinder,const Mesh& cube,
                            const Mesh& hull,const LivingWorldSystem& world,glm::vec3 eye,float seaY,float now) {
    const auto part=[&](const Mesh& mesh,glm::vec3 p,glm::vec3 size,const Material& mat,float yaw=0.0f){
        drawMesh(shader,stats,mesh,glm::translate(glm::mat4(1),p)*glm::rotate(glm::mat4(1),yaw,glm::vec3(0,1,0))*glm::scale(glm::mat4(1),size),mat);};
    static const glm::vec3 colours[]={{0.20f,0.42f,0.15f},{0.18f,0.14f,0.15f},{0.43f,0.46f,0.49f},{0.32f,0.38f,0.33f},
        {0.18f,0.29f,0.16f},{0.53f,0.45f,0.29f},{0.30f,0.27f,0.22f},{0.25f,0.25f,0.38f}};
    // Iterate a fixed 48-site registry using the actual camera (free camera/diver can leave the ship).
    for(int id=0;id<WorldConfig::SITES;++id){const WorldSite& s=world.sites[id];float distance=worldDistance(eye,s.centre);
        if(distance>WorldConfig::LANDMARK_RANGE+s.radius)continue;
        bool near=distance<75,middle=distance<140;const Material terrain=campaignMaterial(colours[s.region]);
        glm::vec3 centre=s.centre+glm::vec3(0,seaY,0);
        if(!s.underwater){
            for(int k=middle?-1:0;k<=(middle?1:0);++k){float a=s.angle+3.14159265f+k*0.85f;glm::vec3 c=centre+glm::vec3(std::sin(a),0,std::cos(a))*s.radius*0.40f;
                float r=s.radius*(k==0?0.64f:0.50f),height=s.height*(k==0?1.0f:0.7f);
                part(sphere,c,{2*r,2*height,2*r},terrain);
                if(middle)part(sphere,c-glm::vec3(0,0.15f,0),{2*r+1,0.65f,2*r+1},SAND);
            }
        }
        glm::vec3 landmark=s.cluePoint();landmark.y=seaY+world.groundAt(s,landmark);
        const bool tower=s.kind==InterestKind::LIGHTHOUSE||s.kind==InterestKind::OUTPOST;
        if(tower){part(cylinder,landmark+glm::vec3(0,7,0),{2,14,2},SAIL_WHITE);part(cylinder,landmark+glm::vec3(0,14,0),{3,1.1f,3},GOLD);}
        else if(s.kind==InterestKind::FORTRESS||s.kind==InterestKind::TEMPLE||s.kind==InterestKind::RUINS){
            for(int k=0;k<(middle?4:2);++k)part(cube,landmark+glm::vec3((k%2?1.0f:-1.0f)*3,2.5f,(k/2?1.0f:-1.0f)*2),{2.3f,5.0f+(k%2),2.3f},CLIFF_ROCK);
            part(cube,landmark+glm::vec3(0,4,0),{7,0.8f,1.2f},CLIFF_ROCK);
        }else if(s.kind==InterestKind::CAVE||s.kind==InterestKind::HAUNTED){
            part(cube,landmark+glm::vec3(-2,2,0),{1.4f,4,2},terrain);part(cube,landmark+glm::vec3(2,2,0),{1.4f,4,2},terrain);
            part(cube,landmark+glm::vec3(0,4.3f,0),{5.4f,1.2f,2.5f},terrain);
        }else if(s.kind==InterestKind::VILLAGE||s.kind==InterestKind::HIDEOUT||s.kind==InterestKind::CAMP){
            for(int k=0;k<(near?4:1);++k){glm::vec3 p=landmark+glm::vec3(k*2.4f,0,0);part(cube,p+glm::vec3(0,1,0),{1.8f,2,2.4f},WEATHERED_BROWN);part(cube,p+glm::vec3(0,2.1f,0),{2.3f,0.3f,2.8f},SAIL_CREAM);}
        }
        if(s.region==1&&!s.underwater){glm::vec3 top=centre+glm::vec3(-std::sin(s.angle)*s.radius*0.4f,s.height,-std::cos(s.angle)*s.radius*0.4f);
            part(sphere,top,{4,0.45f,4},campaignMaterial({0.95f,0.20f,0.035f}));}
        if(!near)continue;
        if(s.region==3&&!s.underwater){glm::vec3 p=centre+glm::vec3(0,s.height*0.45f,0);part(cube,p,{1.2f,s.height,0.2f},campaignMaterial({0.60f,0.78f,0.87f}));
            for(int k=0;k<4;++k)part(sphere,centre+glm::vec3(0,std::fmod(now*2+k,s.height),0),{1.4f,0.28f,0.35f},SAIL_WHITE);}
        // Paired stones are the environmental clue; no exact chart marker exposes the cache.
        for(int k=-1;k<=1;k+=2)part(cylinder,landmark+glm::vec3(k*0.6f,0.8f,0),{0.55f,1.6f,0.55f},GOLD);
        glm::vec3 sign=s.shore()+glm::vec3(0,seaY+0.6f,0);part(cube,sign,{0.8f,0.7f,0.15f},WEATHERED_BROWN);
        if(s.solved&&!s.collected){glm::vec3 p=s.cache();p.y=seaY+(s.underwater?-7.0f:world.groundAt(s,p)+0.25f);
            part(cube,p,{0.8f,0.5f,0.55f},CHEST_WOOD);part(cube,p+glm::vec3(0,0.25f,0),{0.83f,0.06f,0.58f},GOLD);}
        // Vegetation differs by region, placement and height are stable per site.
        if(!s.underwater)for(int k=0;k<8;++k){float a=worldUnit(id*19+k)*6.2831853f;glm::vec3 p=s.centre+glm::vec3(std::sin(a),0,std::cos(a))*s.radius*0.8f;
            p.y=seaY+world.groundAt(s,p);float height=2+worldUnit(id*7+k)*3;
            if(s.region==0||s.region==4){part(cylinder,p+glm::vec3(0,height/2,0),{0.25f,height,0.25f},TREE_TRUNK);part(sphere,p+glm::vec3(0,height,0),{3,0.6f,2},PALM_FROND,a);}
            else part(cube,p+glm::vec3(0,0.6f,0),{1.1f,1.2f,0.8f},terrain,a);}
        // A bounded seabed patch at every island gives diveable reefs and wrecks independent of the ship-following ambient fish.
        glm::vec3 bottom=centre+glm::vec3(0,-9.5f,0);part(sphere,bottom,{s.radius*3,1.5f,s.radius*3},SAND);
        if(s.underwater){part(hull,centre+glm::vec3(0,-7.5f,0),{3,1.8f,9},CHARCOAL,0.6f);
            part(cylinder,centre+glm::vec3(2,-6.8f,1),{0.25f,5,0.25f},WEATHERED_BROWN);}
        for(int k=0;k<10;++k){float a=k*2.39996f;glm::vec3 p=bottom+glm::vec3(std::sin(a)*(5+k*0.4f),1,std::cos(a)*(5+k*0.4f));
            part(sphere,p,{0.8f,1.6f,0.9f},campaignMaterial(k%2?glm::vec3(0.52f,0.20f,0.32f):glm::vec3(0.18f,0.44f,0.35f)));
            if(eye.y<seaY+3){float t=now*0.6f+k;part(sphere,p+glm::vec3(std::sin(t)*2,2+std::sin(t*0.7f),std::cos(t)*2),{0.12f,0.13f,0.45f},GOLD,t);}}
    }
    for(const auto& s:world.ships){float d=worldDistance(eye,s.pos);if(d>130||s.deadLeft>WorldConfig::SHIP_RESPAWN-6)continue;
        if(s.health<=0)continue;
        glm::vec3 p=s.pos;p.y=seaY+0.12f+0.09f*std::sin(now+s.pos.x*0.1f);
        glm::mat4 root=glm::translate(glm::mat4(1),p)*glm::rotate(glm::mat4(1),s.heading,glm::vec3(0,1,0));
        const auto piece=[&](const Mesh& mesh,glm::vec3 at,glm::vec3 size,const Material& mat){drawMesh(shader,stats,mesh,root*glm::translate(glm::mat4(1),at*s.scale)*glm::scale(glm::mat4(1),size*s.scale),mat);};
        piece(hull,{0,0,0},{1.8f,1.2f,5.6f},s.health<s.maxHealth/2?CHARCOAL:HULL_TIMBER);
        for(int m=0;m<(s.fishing?1:2);++m){float z=(m==0?0.9f:-1.2f);piece(cylinder,{0,2,z},{0.12f,4,0.12f},WEATHERED_BROWN);
            piece(cube,{0,2.7f,z},{2.7f,1.7f,0.04f},s.faction==WorldFaction::NAVY?SAIL_WHITE:(s.faction==WorldFaction::PIRATE?SAIL_BLACK:SAIL_CREAM));}
        if(d<50)piece(cube,{0,0.65f,-1.8f},{1.3f,0.65f,1.2f},DECK_WOOD);
    }
    for(const auto& d:world.drops)if(d.active&&worldDistance(eye,d.pos)<80)drawCampaignCargoPiece(shader,stats,cube,cylinder,
        glm::translate(glm::mat4(1),glm::vec3(d.pos.x,seaY+0.12f+0.1f*std::sin(now+d.pos.x),d.pos.z)),d.cargo,2);
    for(const auto& e:world.events)if(e.active&&worldDistance(eye,e.pos)<100&&e.ship<0){glm::vec3 p=e.pos+glm::vec3(0,seaY+0.12f,0);
        if(e.kind==EncounterKind::SURVIVORS||e.kind==EncounterKind::ASSIST||e.kind==EncounterKind::BURNING||e.kind==EncounterKind::DISTRESS){
            part(hull,p,{1.5f,0.7f,3.8f},CHARCOAL);part(cylinder,p+glm::vec3(0,0.7f,0),{0.23f,0.7f,0.23f},SAIL_CREAM);part(sphere,p+glm::vec3(0,1.2f,0),{0.3f,0.3f,0.3f},SAND);
        }else part(cube,p,{0.75f,0.6f,0.75f},GOLD);
    }
    // One migrating whale per region; a continuous submerged/surfacing path, never a spawn at the camera.
    for(int r=0;r<8;++r){float t=now*0.035f+r;glm::vec3 p=world.regions[r].centre+glm::vec3(std::sin(t)*110,0,std::cos(t)*110);
        if(worldDistance(eye,p)>90)continue;p.y=seaY-1.5f+0.8f*std::sin(now*0.21f+r);
        part(sphere,p,{2,1.6f,7.5f},campaignMaterial({0.12f,0.20f,0.25f}),t+1.5708f);
        part(sphere,p+glm::vec3(-std::cos(t)*3.5f,0,std::sin(t)*3.5f),{2.8f,0.18f,0.9f},CHARCOAL,t+1.5708f);}
}
