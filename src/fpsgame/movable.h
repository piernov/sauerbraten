// movable.h: implements physics for inanimate models

struct movableset
{
    fpsclient &cl;

    enum
    {
        BOXWEIGHT = 25,
        BARRELHEALTH = 50,
        BARRELWEIGHT = 25,
        PLATFORMWEIGHT = 1000,
        PLATFORMSPEED = 8,
        EXPLODEDELAY = 200
    };

    struct movable : dynent
    {
        int etype, mapmodel, health, weight, explode, tag, dir;
        movableset *ms;

        movable(const entity &e, movableset *ms) : 
            etype(e.type),
            mapmodel(e.attr2),
            health(e.type==BARREL ? (e.attr4 ? e.attr4 : BARRELHEALTH) : 0), 
            weight(e.type==PLATFORM || e.type==ELEVATOR ? PLATFORMWEIGHT : (e.attr3 ? e.attr3 : (e.type==BARREL ? BARRELWEIGHT : BOXWEIGHT))), 
            explode(0),
            tag(e.type==PLATFORM || e.type==ELEVATOR ? e.attr3 : 0),
            dir(e.type==PLATFORM || e.type==ELEVATOR ? (e.attr4 < 0 ? -1 : 1) : 0),
            ms(ms)
        {
            state = CS_ALIVE;
            type = ENT_INANIMATE;
            yaw = float((e.attr1+7)-(e.attr1+7)%15);
            if(e.type==PLATFORM || e.type==ELEVATOR) 
            {
                maxspeed = e.attr4 ? fabs(float(e.attr4)) : PLATFORMSPEED;
                if(tag) vel = vec(0, 0, 0);
                else if(e.type==PLATFORM) { vecfromyawpitch(yaw, 0, 1, 0, vel); vel.mul(dir*maxspeed); } 
                else vel = vec(0, 0, dir*maxspeed);
            }

            const char *mdlname = mapmodelname(e.attr2);
            if(mdlname) setbbfrommodel(this, mdlname);
        }
       
        void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
        {
            if(etype!=BOX && etype!=BARREL) return;
            vec push(dir);
            push.mul(80*damage/weight);
            vel.add(push);
            moving = true;
        }
 
        void damaged(int damage, fpsent *at, int gun = -1)
        {
            if(etype!=BARREL || state!=CS_ALIVE || explode) return;
            health -= damage;
            if(health>0) return;
            if(gun==GUN_BARREL) explode = lastmillis + EXPLODEDELAY;
            else 
            {
                state = CS_DEAD;
                ms->cl.ws.explode(true, at, o, this, guns[GUN_BARREL].damage, GUN_BARREL);
            }
        }

        void suicide()
        {
            state = CS_DEAD;
            if(etype==BARREL) ms->cl.ws.explode(true, ms->cl.player1, o, this, guns[GUN_BARREL].damage, GUN_BARREL);
        }
    };

    movableset(fpsclient &cl) : cl(cl)
    {
        CCOMMAND(platform, "ii", (movableset *self, int *tag, int *newdir), self->triggerplatform(*tag, *newdir));
    }
    
    vector<movable *> movables;
   
    void clear()
    {
        if(movables.length())
        {
            cleardynentcache();
            movables.deletecontentsp();
        }
        if(!m_dmsp && !m_classicsp) return;
        loopv(cl.et.ents) 
        {
            const entity &e = *cl.et.ents[i];
            if(e.type!=BOX && e.type!=BARREL && e.type!=PLATFORM && e.type!=ELEVATOR) continue;
            movable *m = new movable(e, this);
            movables.add(m);
            m->o = e.o;
            entinmap(m);
            updatedynentcache(m);
        }
    }

    void triggerplatform(int tag, int newdir)
    {
        newdir = max(-1, min(1, newdir));
        loopv(movables)
        {
            movable *m = movables[i];
            if(m->state!=CS_ALIVE || (m->etype!=PLATFORM && m->etype!=ELEVATOR) || m->tag!=tag) continue;
            if(!newdir)
            {
                if(m->tag) m->vel = vec(0, 0, 0);
                else m->vel.neg();
            }
            else
            {
                if(m->etype==PLATFORM) { vecfromyawpitch(m->yaw, 0, 1, 0, m->vel); m->vel.mul(newdir*m->dir*m->maxspeed); }
                else m->vel = vec(0, 0, newdir*m->dir*m->maxspeed);
            }
        }
    }

    void update(int curtime)
    {
        if(!curtime) return;
        loopv(movables)
        {
            movable *m = movables[i];
            if(m->state!=CS_ALIVE) continue;
            if(m->etype==PLATFORM || m->etype==ELEVATOR)
            {
                if(m->vel.iszero()) continue;
                for(int remaining = curtime; remaining>0;)
                {
                    int step = min(remaining, 20);
                    remaining -= step;
                    if(!moveplatform(m, vec(m->vel).mul(step/1000.0f)))
                    {
                        if(m->tag) { m->vel = vec(0, 0, 0); break; }
                        else m->vel.neg();
                    }
                }
            }
            else if(m->explode && lastmillis >= m->explode)
            {
                m->state = CS_DEAD;
                m->explode = 0;
                cl.ws.explode(true, (fpsent *)m, m->o, m, guns[GUN_BARREL].damage, GUN_BARREL);
                adddecal(DECAL_SCORCH, m->o, vec(0, 0, 1), RL_DAMRAD/2);
            }
            else if(m->moving || (m->onplayer && (m->onplayer->state!=CS_ALIVE || m->lastmoveattempt <= m->onplayer->lastmove))) moveplayer(m, 1, true);
        }
    }

    void render()
    {
        loopv(movables)
        {
            movable &m = *movables[i];
            if(m.state!=CS_ALIVE) continue;
            vec o(m.o);
            o.z -= m.eyeheight;
            const char *mdlname = mapmodelname(m.mapmodel);
            if(!mdlname) continue;
			rendermodel(NULL, mdlname, ANIM_MAPMODEL|ANIM_LOOP, o, m.yaw, 0, MDL_LIGHT | MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED, &m);
        }
    }
};
