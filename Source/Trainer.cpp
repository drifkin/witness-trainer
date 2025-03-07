#include "pch.h"
#include "Trainer.h"
#include "Panels.h"

std::shared_ptr<Trainer> Trainer::Create(const std::shared_ptr<Memory>& memory) {
    auto trainer = std::shared_ptr<Trainer>(new Trainer());

    memory->AddSigScan({0x84, 0xC0, 0x75, 0x59, 0xBA, 0x20, 0x00, 0x00, 0x00}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        // This int is actually desired_movement_direction, which immediately preceeds camera_position
        trainer->_cameraPos = Memory::ReadStaticInt(offset, index + 0x19, data) + 0x10;

        // This doesn't have a consistent offset from the scan, so search until we find "mov eax, [addr]"
        for (; index < data.size(); index++) {
            if (data[index - 2] == 0x8B && data[index - 1] == 0x05) {
                trainer->_noclipEnabled = Memory::ReadStaticInt(offset, index, data);
                break;
            }
        }
    });

    memory->AddSigScan({0xC7, 0x45, 0x77, 0x00, 0x00, 0x80, 0x3F, 0xC7, 0x45, 0x7F, 0x00, 0x00, 0x80, 0x3F}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_cameraAng = Memory::ReadStaticInt(offset, index + 0x17, data);
    });

    memory->AddSigScan({0x0F, 0x29, 0x7C, 0x24, 0x70, 0x44, 0x0F, 0x29, 0x54, 0x24, 0x60}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_noclipSpeed = Memory::ReadStaticInt(offset, index + 0x4F, data);
    });

    memory->AddSigScan({0x76, 0x09, 0xF3, 0x0F, 0x11, 0x05}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_fovCurrent = Memory::ReadStaticInt(offset, index + 0x0F, data);
    });

    memory->AddSigScan({0x74, 0x41, 0x48, 0x85, 0xC0, 0x74, 0x04, 0x48, 0x8B, 0x48, 0x10}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_globals = Memory::ReadStaticInt(offset, index + 0x14, data);
    });

    memory->AddSigScan({0x84, 0xC0, 0x74, 0x19, 0x0F, 0x2F, 0xB7}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_doorOpen = offset + index + 0x0B;
        trainer->_solvedTargetOffset = *(int*)&data[index + 0x07];
    });

    memory->AddSigScan({0x84, 0xC0, 0x74, 0x11, 0x0F, 0x2F, 0xBF}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_doorClose = offset + index + 0x0B;
    });

    // This sigscan is rather awkward, it's not pointing to the start of a line, because I modify that line.
    memory->AddSigScan({0x18, 0x48, 0x8B, 0xCF, 0x89, 0x9F}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_powerOn = offset + index - 0x05;
    });

    memory->AddSigScan({0x48, 0x89, 0x58, 0x08, 0x48, 0x89, 0x70, 0x10, 0x48, 0x89, 0x78, 0x18, 0x48, 0x8B, 0x3D}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_campaignState = Memory::ReadStaticInt(offset, index + 0x27, data);
    });

    memory->AddSigScan({0xF3, 0x0F, 0x59, 0xFD, 0xF3, 0x0F, 0x5C, 0xC8}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        // This doesn't have a consistent offset from the scan, so search until we find "jmp +08"
        for (; index < data.size(); index++) {
            if (data[index - 2] == 0xEB && data[index - 1] == 0x08) {
                trainer->_walkAcceleration = Memory::ReadStaticInt(offset, index - 0x06, data);
                trainer->_walkDeceleration = Memory::ReadStaticInt(offset, index + 0x04, data);
                break;
            }
        }
        // Once again, there's no consistent offset, so we read until "movss xmm1, [addr]"
        for (; index < data.size(); index++) {
            if (data[index - 4] == 0xF3 && data[index - 3] == 0x0F && data[index - 2] == 0x10 && data[index - 1] == 0x0D) {
                trainer->_runSpeed = Memory::ReadStaticInt(offset, index, data);
                break;
            }
        }
    });

    memory->AddSigScan({0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0xE9, 0xB3}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_recordPlayerUpdate = offset + index - 0x0C;
    });

    memory->AddSigScan({0xF2, 0x0F, 0x58, 0xC8, 0x66, 0x0F, 0x5A, 0xC1, 0xF2}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_activePanelOffsets.push_back(Memory::ReadStaticInt(offset, index + 0x36, data, 5));
        trainer->_activePanelOffsets.push_back(data[index + 0x5A]); // This is 0x10 in both versions I have, but who knows.
        trainer->_activePanelOffsets.push_back(*(int*) &data[index + 0x54]);
    });

    memory->AddSigScan({0x41, 0xB8, 0x61, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xD3}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        for (; index > 0; index--) {
            if (data[index + 8] == 0x74 && data[index + 9] == 0x10) {
                trainer->_mainMenuColor = offset + index;
                break;
            }
        }
    });

    memory->AddSigScan({0x0F, 0x57, 0xC0, 0x0F, 0x2F, 0x80, 0xB4, 0x00, 0x00, 0x00, 0x0F, 0x92, 0xC0, 0xC3}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        auto console = Memory::ReadStaticInt(offset, index-4, data);
        trainer->_consoleWindowYB = {console, 0x4C};
        trainer->_consoleOpenTarget = {console, 0xB4};
    });

    memory->AddSigScan({0x83, 0xF8, 0x03, 0x7C, 0x41, 0x84, 0xC9, 0x74, 0x1F}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_wantCampaignSave = Memory::ReadStaticInt(offset, index + 0x2A, data, 5);
    });

    memory->AddSigScan({0x74, 0x14, 0x48, 0x8B, 0x95}, [trainer](__int64 offset, int index, const std::vector<byte>& data) {
        trainer->_epNameOffset = *(int*)&data[index + 0x05];
    });

    // We need to save _memory before we exit, otherwise we can't destroy properly.
    trainer->_memory = memory;

    size_t numFailedScans = memory->ExecuteSigScans();
    if (trainer->_globals && trainer->_globals == 0x5B28C0) numFailedScans -= 1; // FOV scan is expected to fail on older versions.
    if (numFailedScans != 0) return nullptr; // Sigscans failed, we'll try again later.

    trainer->SetMainMenuColor(true);
    return trainer;
}

// Restore default game settings when shutting down the trainer.
Trainer::~Trainer() {
    SetNoclip(false);
    SetRandomDoorsPractice(false);
    SetCanSave(true);
    SetInfiniteChallenge(false);
    float fov = GetFov();
    if (fov < 50.53401947f) SetFov(50.53401947f);
    if (fov > 88.50715637f) SetFov(88.50715637f);
    SetSprintSpeed(2.0f);
    SetMainMenuColor(false);
}

int Trainer::GetActivePanel() {
    return _memory->ReadData<int>(_activePanelOffsets, 1)[0] - 1;
}

std::shared_ptr<Trainer::EntityData> Trainer::GetEntityData(int id) {
    if (id == -1) return nullptr;
    int64_t entity = _memory->ReadData<int64_t>({_globals, 0x18, id * 8}, 1)[0];
    if (entity == 0) return nullptr; // Entity is not defined, manager is likely being re-allocated due to load.
    int readId = _memory->ReadData<int>({_globals, 0x18, id * 8, 0x10}, 1)[0];
    if (id != (readId - 1)) return nullptr; // Entity is no longer a valid object (or is not the entity we expected to read)

    std::string typeName = _memory->ReadString({_globals, 0x18, id * 8, 0x08, 0x08});
    if (typeName == "Machine_Panel") return GetPanelData(id);
    if (typeName == "Pattern_Point") return GetEPData(id);
    // Unknown typeName, assume memory was freed.
    return nullptr;
}

std::shared_ptr<Trainer::EntityData> Trainer::GetPanelData(int id) {
    int nameOffset = _solvedTargetOffset - 0x7C;
    int tracedEdgesOffset = _solvedTargetOffset - 0x6C;
    int stateOffset = _solvedTargetOffset - 0x14;
    int hasEverBeenSolvedOffset = _solvedTargetOffset + 0x04;

    auto data = std::make_shared<EntityData>();
    data->name = _memory->ReadString({_globals, 0x18, id * 8, nameOffset});
    int state = _memory->ReadData<int>({_globals, 0x18, id * 8, stateOffset}, 1)[0];
    int hasEverBeenSolved = _memory->ReadData<int>({_globals, 0x18, id * 8, hasEverBeenSolvedOffset}, 1)[0];
    data->solved = hasEverBeenSolved;
    if (state == 0 && hasEverBeenSolved == 0) data->state = "Has never been solved";
    else if (state == 0 && hasEverBeenSolved == 1) data->state = "Was previously solved";
    else if (state == 1) data->state = "Solved";
    else if (state == 2) data->state = "Failed";
    else if (state == 3) data->state = "Exited";
    else if (state == 4) data->state = "Negation pending";
    else data->state = "Unknown";

    auto numEdges = _memory->ReadData<int>({_globals, 0x18, id * 8, tracedEdgesOffset}, 1)[0];
    if (numEdges != 0) {
        // Explicitly recomputing this intermediate, since the edges array might have re-allocated.
        auto edgeDataPtr = _memory->ReadData<int>({_globals, 0x18, id * 8, tracedEdgesOffset + 8}, 1)[0];
        if (edgeDataPtr != 0) {
            // Each Traced_Edge object is 0x34 bits == 13 bytes
            auto edgeData = _memory->ReadData<float>({_globals, 0x18, id * 8, tracedEdgesOffset + 8, 0x0}, 13 * numEdges);
            data->tracedEdges.resize(numEdges * 3);
            for (int i=0; i<numEdges; i++) {
                // position_a is a Vector3 at 0x18 (=element 6)
                data->tracedEdges[i*3+0] = edgeData[i*13 + 6];
                data->tracedEdges[i*3+1] = edgeData[i*13 + 7];
                data->tracedEdges[i*3+2] = edgeData[i*13 + 8];
            }
        }
    }

    return data;

    /* Note: Care must be taken to invalidate cache entries when reading this data, since AutoArray reallocates.
    int numDotsOffset = _solvedTargetOffset + 0x11C;
    int dotPositionsOffset = _solvedTargetOffset + 0x12C;
    int numEdges = _memory->ReadData<int>({_globals, 0x18, id * 8, tracedEdgesOffset}, 1)[0];

    if (numEdges > 0) {
        std::vector<Traced_Edge> edges = _memory->ReadData<Traced_Edge>({_globals, 0x18, id*8, tracedEdgesOffset + 0x08, 0}, numEdges);
        int numDots = _memory->ReadData<int>({_globals, 0x18, id*8, numDotsOffset}, 1)[0];
        std::vector<float> positions = _memory->ReadData<float>({_globals, 0x18, id*8, dotPositionsOffset, 0}, numDots*2);
        for (auto edge : edges) {
            data->tracedEdges.push_back(positions[edge.index_a * 2]); // x1
            data->tracedEdges.push_back(positions[edge.index_a * 2 + 1]); // y1
            data->tracedEdges.push_back(positions[edge.index_b * 2]); // x2
            data->tracedEdges.push_back(positions[edge.index_b * 2 + 1]); // y2
        }
    }
    */
}

std::shared_ptr<Trainer::EntityData> Trainer::GetEPData(int id) {
    auto data = std::make_shared<EntityData>();
    data->name = _memory->ReadString({_globals, 0x18, id * 8, _epNameOffset});
    return data;
}

void Trainer::ShowMissingPanels() {
    std::vector<std::string> missingPanels;
    for (const auto& [id, panelName] : PANELS) {
        std::shared_ptr<EntityData> data = GetEntityData(id);
        assert(data);
        if (data && !data->solved) missingPanels.push_back(panelName);
    }
    if (missingPanels.empty()) {
        MessageBoxA(NULL, "You solved all the panels!", "", MB_OK);
        return;
    }

    std::string message;
    for (const auto& missingPanel : missingPanels) {
        if (message.size() == 0) {
            message += missingPanel;
        } else {
            message += ", " + missingPanel;
        }
        if (message.size() > 1000) {
            message += ", ...";
            break;
        }
    }
    std::string title = std::to_string(missingPanels.size()) + " unsolved, counted panels";
    MessageBoxA(NULL, message.c_str(), title.c_str(), MB_OK);
}

void Trainer::ShowNearbyEntities() {
    int32_t maxId = _memory->ReadData<int>({_globals, 0x14}, 1)[0];

    std::vector<std::pair<float, int32_t>> nearbyEntities(20, {99999.9f, 0});

    auto basePos = GetCameraPos();
    for (int32_t id = 0; id < maxId; id++) {
        if (id == 0x1E465) continue; // Skip over Entity_Human
        int32_t entity = _memory->ReadData<int>({_globals, 0x18, id * 8}, 1)[0];
        if (entity == 0) continue;
        std::vector<float> pos = _memory->ReadData<float>({_globals, 0x18, id * 8, 0x24}, 3);

        float norm = std::pow(basePos[0] - pos[0], 2) + std::pow(basePos[1] - pos[1], 2) + std::pow(basePos[2] - pos[2], 2);
        for (int i = 0; i < nearbyEntities.size(); i++) {
            if (norm < nearbyEntities[i].first) {
                nearbyEntities.insert(nearbyEntities.begin() + i, {norm, id});
                nearbyEntities.resize(nearbyEntities.size() - 1);
                break;
            }
        }
    }

    DebugPrint("Entity ID\tDistance\t     X\t     Y\t     Z\tType");
    for (const auto& [norm, entityId] : nearbyEntities) {
        std::vector<float> pos = _memory->ReadData<float>({_globals, 0x18, entityId * 8, 0x24}, 3);
        std::string typeName = _memory->ReadString({_globals, 0x18, entityId * 8, 0x08, 0x08});

        std::stringstream message;
        message << "0x" << std::hex << std::setfill('0') << std::setw(5) << entityId << '\t';
        message << std::sqrt(norm) << '\t';
        message << pos[0] << '\t' << pos[1] << '\t' << pos[2] << '\t' << typeName;
        DebugPrint(message.str());
    }
}

void Trainer::ExportEntities() {
    int32_t maxId = _memory->ReadData<int>({_globals, 0x14}, 1)[0];

    DebugPrint("Entity ID\tType\tName\t     X\t     Y\t     Z");
    for (int32_t id = 1; id < maxId; id++) {
        int32_t entity = _memory->ReadData<int>({_globals, 0x18, id * 8}, 1)[0];
        if (entity == 0) continue;
        std::string typeName = _memory->ReadString({_globals, 0x18, id * 8, 0x08, 0x08});
        std::string entityName = _memory->ReadString({_globals, 0x18, id * 8, 0x58});
        std::vector<float> pos = _memory->ReadData<float>({_globals, 0x18, id * 8, 0x24}, 3);

        std::stringstream message;
        message << "0x" << std::hex << std::setfill('0') << std::setw(5) << id << '\t';
        message << typeName << '\t';
        message << entityName << '\t';
        message << pos[0] << '\t' << pos[1] << '\t' << pos[2] << '\t';
        DebugPrint(message.str());
    }
}

void Trainer::SnapToPoint(const std::vector<float>& point) {
    auto cameraPos = GetCameraPos();

    float Ax = cameraPos[0];
    float Ay = cameraPos[1];
    float Az = cameraPos[2];
    float Bx = point[0];
    float By = point[1];
    float Bz = point[2];

    float hypotenuse2 = sqrt((Ax - Bx) * (Ax - Bx) + (Ay - By) * (Ay - By));
    float hypotenuse3 = sqrt((Ax - Bx) * (Ax - Bx) + (Ay - By) * (Ay - By) + (Az - Bz) * (Az - Bz));

    std::vector<float> cameraAng{
        asin((By - Ay) / hypotenuse2),
        acos(hypotenuse2 / hypotenuse3),
    };
    if (Ax > Bx) cameraAng[0] = 3.141592654f - cameraAng[0];
    if (Az > Bz) cameraAng[1] = -cameraAng[1];

    SetCameraAng(cameraAng);
}

bool Trainer::GetNoclip() {
    return (bool) _memory->ReadData<int>({_noclipEnabled}, 1)[0];
}

float Trainer::GetNoclipSpeed() {
    return _memory->ReadData<float>({_noclipSpeed}, 1)[0];
}

std::vector<float> Trainer::GetPlayerPos() {
    return _memory->ReadData<float>({_globals, 0x18, 0x1E465 * 8, 0x24}, 3);
}

std::vector<float> Trainer::GetCameraPos() {
    return _memory->ReadData<float>({_cameraPos}, 3);
}

std::vector<float> Trainer::GetCameraAng() {
    return _memory->ReadData<float>({_cameraAng}, 2);
}

float Trainer::GetFov() {
    if (_fovCurrent == 0) return 0.0f; // FOV is not available on some old patches
    return _memory->ReadData<float>({_fovCurrent}, 1)[0];
}

bool Trainer::CanSave() {
    return _memory->ReadData<byte>({_campaignState, 0x50}, 1)[0] == 0x00;
}

float Trainer::GetSprintSpeed() {
    return _memory->ReadData<float>({_runSpeed}, 1)[0];
}

bool Trainer::GetInfiniteChallenge() {
    return _memory->ReadData<byte>({_recordPlayerUpdate}, 1)[0] == 0x0F;
}

bool Trainer::GetConsoleOpen() {
    return _memory->ReadData<float>(_consoleOpenTarget, 1)[0] == 1.0f;
}

bool Trainer::GetRandomDoorsPractice() {
    return _memory->ReadData<byte>({_doorOpen}, 1)[0] == 0x90;
}

void Trainer::SetNoclip(bool enabled) {
    _memory->WriteData<byte>({_noclipEnabled}, {static_cast<byte>(enabled)});
}

void Trainer::SetNoclipSpeed(float speed) {
    if (speed <= 0.0f) return;
    _memory->WriteData<float>({_noclipSpeed}, {speed});
}

void Trainer::SetPlayerPos(const std::vector<float>& pos) {
    _memory->WriteData<float>({_globals, 0x18, 0x1E465 * 8, 0x24}, pos);
}

void Trainer::SetCameraPos(const std::vector<float>& pos) {
    _memory->WriteData<float>({_cameraPos}, pos);
}

void Trainer::SetCameraAng(const std::vector<float>& ang) {
    _memory->WriteData<float>({_cameraAng}, ang);
}

void Trainer::SetFov(float fov) {
    if (!_fovCurrent) return;
    _memory->WriteData<float>({_fovCurrent}, {fov});
}

void Trainer::SetCanSave(bool canSave) {
    _memory->WriteData<byte>({_campaignState, 0x50}, {canSave ? (byte) 0x00 : (byte) 0x01});
}

void Trainer::SetSprintSpeed(float speed) {
    if (speed == 0) return;
    float sprintSpeed = GetSprintSpeed();
    if (sprintSpeed == 0.0f) return; // sanity check, to avoid an accidental div0
    float multiplier = speed / sprintSpeed;
    if (multiplier == 1.0f) return;
    _memory->WriteData<float>({_runSpeed}, {speed});
    _memory->WriteData<float>({_walkAcceleration}, {_memory->ReadData<float>({_walkAcceleration}, 1)[0] * multiplier});
    _memory->WriteData<float>({_walkDeceleration}, {_memory->ReadData<float>({_walkDeceleration}, 1)[0] * multiplier});
}

void Trainer::SetConsoleOpen(bool enable) {
    if (enable) {
        _memory->WriteData<float>(_consoleWindowYB, {0.0f});
        _memory->WriteData<float>(_consoleOpenTarget, {1.0f});
    } else {
        _memory->WriteData<float>(_consoleOpenTarget, {0.0f});
    }
}

void Trainer::SaveCampaign() {
    _memory->WriteData<byte>({_wantCampaignSave}, {0x01});
    for (int i=0; i<100; i++) {
        ::Sleep(10); // Wait a bit for the game to run
        byte wantCampaignSave = _memory->ReadData<byte>({_wantCampaignSave}, 1)[0];
        if (wantCampaignSave == 0x00) break;
    }
}

void Trainer::SetInfiniteChallenge(bool enable) {
    if (enable) {
        // Jump over abort_speed_run, with NOP padding
        _memory->WriteData<byte>({_recordPlayerUpdate}, {0xEB, 0x07, 0x66, 0x90});
    } else {
        // (original code) Load entity_manager into rcx
        _memory->WriteData<byte>({_recordPlayerUpdate}, {0x48, 0x8B, 0x4B, 0x18});
    }
}

void Trainer::SetMainMenuColor(bool enable) {
    if (enable) { // Set the main menu to red by *not* setting the green or blue component.
        _memory->WriteData<byte>({_mainMenuColor}, {0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}); // 8-byte NOP
    } else { // Restore the original setting by copy/pasting from the block below.
        std::vector<byte> code = _memory->ReadData<byte>({_mainMenuColor + 0x12}, 8);
        _memory->WriteData<byte>({_mainMenuColor}, code);
    }
}

void Trainer::SetRandomDoorsPractice(bool enable) {
    if (_solvedTargetOffset == 0) return;

    int idToPowerOffset = _solvedTargetOffset + 0x20;
    int onTargetOffset = _solvedTargetOffset + 0x10;

    // This block is up here because it needs to be rerun when we reload the game.
    if (enable) {
        // When the panel is solved, power nothing.
        _memory->WriteData<int>({_globals, 0x18, 0x1983 * 8, idToPowerOffset}, {0x00000000});
        _memory->WriteData<int>({_globals, 0x18, 0x1987 * 8, idToPowerOffset}, {0x00000000});

        // To make sure that the panels are randomized the first time they power on, we mark them as having already been solved.
        _memory->WriteData<float>({_globals, 0x18, 0x1983 * 8, _solvedTargetOffset}, {1.0f});
        _memory->WriteData<float>({_globals, 0x18, 0x1987 * 8, _solvedTargetOffset}, {1.0f});
    } else {
        // When the panel is solved, power the double doors.
        _memory->WriteData<int>({_globals, 0x18, 0x1983 * 8, idToPowerOffset}, {0x00017C68});
        _memory->WriteData<int>({_globals, 0x18, 0x1987 * 8, idToPowerOffset}, {0x00017C68});
    }

    // If the injection state matches the enable request, no futher action is needed.
    if (enable == GetRandomDoorsPractice()) return;

    if (enable) {
        // When the panel closes, always turn it off
        _memory->WriteData<byte>({_doorClose}, {0x90, 0x90});
        // When the panel opens, always reset it, and power it on.
        _memory->WriteData<byte>({_doorOpen}, {0x90, 0x90});
        // When the panel powers on, only randomize if it's solved
        _memory->WriteData<int>({_powerOn}, {_solvedTargetOffset});
        _memory->WriteData<byte>({_powerOn + 0x04}, {0x77, 0x18});

        // When the panel powers on, mark it as not solved
        _memory->WriteData<byte>({_powerOn + 0x3B}, {
            0x48, 0x63, 0x00, // movsxd rax, dword ptr [rax]
            0x66, 0xC7, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov word ptr ds:[rdi + <offset>], 0x00
        });
        // Note: We're writing to this register (which is a float!) as a word, since we don't have enough instruction space.
        // This float should only ever be set to 0x3F800000 (1.0f), though -- so we can just write to the high bits.
        _memory->WriteData<int>({_powerOn + 0x41}, {_solvedTargetOffset + 2});
    } else {
        // When the panel closes, if the puzzle has been solved, turn it off
        _memory->WriteData<byte>({_doorClose}, {0x76, 0x08});
        // When the panel opens, if the puzzle has not been solved, reset it and power it on
        _memory->WriteData<byte>({_doorOpen}, {0x76, 0x10});
        // When the panel powers on, only randomize if it's off
        _memory->WriteData<int>({_powerOn}, {onTargetOffset});
        _memory->WriteData<byte>({_powerOn + 0x04}, {0x76, 0x18});
    }
}
