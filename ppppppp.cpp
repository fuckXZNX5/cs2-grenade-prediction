struct Sphere {
    Vec3 center{};
    float radius{};
};

Sphere grenadecalc;
std::mutex predmtx;

inline Vec3 AngleToDirection(const QAngle& angles)
{
    Vec3 direction;

   
    float pitch = angles.pitch * (M_PI / 180.0f);
    float yaw = angles.yaw * (M_PI / 180.0f);

    float cosPitch = cosf(pitch);
    direction.x = cosf(yaw) * cosPitch;
    direction.y = sinf(yaw) * cosPitch;
    direction.z = -sinf(pitch);  

    return direction;
}

// This entire shit took around 2.5 weeks to figure out LMFAOO
Vec3 CalculateThrowVelocity(const QAngle& viewAngles, float throwStrength, const Vec3& playerVelocity) {
    QAngle adjustedAngles = viewAngles;

    
    if (adjustedAngles.pitch < -89.f) {
        adjustedAngles.pitch += 360.f;
    }
    else if (adjustedAngles.pitch > 89.f) {
        adjustedAngles.pitch -= 360.f;
    }

   
    
    adjustedAngles.pitch -= (90.f - std::fabsf(adjustedAngles.pitch)) * 10.f / 90.f;

    
    Vec3 direction = AngleToDirection(adjustedAngles);

    
    Vec3 throwVelocity = direction * (throwStrength * 0.7f + 0.3f) * 1090.0f; // The 1090 constant was obtained by testing for 3 hours straight lmfao

    
    throwVelocity += playerVelocity * 1.25f;

    return throwVelocity;
}


std::vector<Vec3> SimulateGrenadeTrajectory(const Vec3& startPos, const Vec3& velocity) {
    Vec3 currentPos = startPos;
    Vec3 currentVel = velocity;

    std::vector<Vec3> path;
    path.reserve(100);  

    grenadecalc.radius = 5.0f; 

    constexpr int INTERPOLATION_STEPS = 10; 

    for (int step = 0; step < MAX_SIMULATION_STEPS; ++step) {
        path.emplace_back(currentPos); 

        
        currentVel.z -= CS_GRAVITY * CS_TICK_INTERVAL;

        
        Vec3 nextPos = currentPos + (currentVel * CS_TICK_INTERVAL);

        
        for (int i = 1; i <= INTERPOLATION_STEPS; ++i) {
            float t = static_cast<float>(i) / INTERPOLATION_STEPS;
            Vec3 interpolatedPos = currentPos + (nextPos - currentPos) * t; 

            grenadecalc.center = interpolatedPos;

            if (map.tracehullsphere(grenadecalc)) { 
                path.emplace_back(interpolatedPos);  
                return path;  
            }
        }

        
        currentPos = nextPos;
    }

    return path;
}


void DrawPredictedTrajectory(const std::vector<Vec3>& path) {
    auto* drawList = ImGui::GetBackgroundDrawList();

    for (size_t i = 1; i < path.size(); ++i) {
        Vec2 screenPos1, screenPos2;

        if (w2s(path[i - 1], screenPos1, current__vm, SCW, SCH) &&
            w2s(path[i], screenPos2, current__vm, SCW, SCH)) {

            drawList->AddLine(
                ImVec2(screenPos1.x, screenPos1.y),
                ImVec2(screenPos2.x, screenPos2.y),
                IM_COL32(0, 255, 0, 255)  
            );
        }
    }
}

traj1 traj;


void GrenadePred() {
    while (true) {
        if (lp.currweapid == WEAPON_HEGRENADE ||
            lp.currweapid == WEAPON_MOLOTOV ||
            lp.currweapid == WEAPON_INCGRENADE ||
            lp.currweapid == WEAPON_FLASHBANG ||
            lp.currweapid == WEAPON_SMOKEGRENADE) 
        {
           
            if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000) || ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) /*driver->RPM<float>(lp.currweapid + cs2_dumper::schemas::client_dll::C_BaseCSGrenade::m_fThrowTime) == 0.f*/)
            {

                float throw_strength = driver->RPM<float>(
                    lp.weaponptr + cs2_dumper::schemas::client_dll::C_BaseCSGrenade::m_flThrowStrength); 
                

                std::lock_guard<std::mutex> lock(predmtx);

                
                Vec3 playerVelocity = driver->RPM<Vec3>(lp.Pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_vecVelocity); 
                traj.m_vInitialVelocity = CalculateThrowVelocity(lp.ViewAngles, throw_strength, playerVelocity); 
                traj.m_vInitialPosition = lp.Origin.pos + driver->RPM<Vec3>(lp.Pawn + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset); 

                
                traj.predictedt = SimulateGrenadeTrajectory(traj.m_vInitialPosition, traj.m_vInitialVelocity); 
            }
        }

        sleepMicroseconds(1000LL);
    }
}


std::lock_guard<std::mutex> lock(predmtx);

DrawPredictedTrajectory(traj.predictedt);