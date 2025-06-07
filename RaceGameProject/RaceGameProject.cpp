// RaceGameProject.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include "Matrice.h"
#include "Particle.h"
#include "Vector.h"
#include <vector>
#include <sstream>
#include <cmath>

using namespace std;
using namespace tle;
const int checkpointCount = 9;
const int maxLaps = 3;
const float checkpointRadius = 15.0f;

#define PI 3.14159265359f

I3DEngine* myEngine = New3DEngine(kTLX);
IFont* font;
IFont* titleFont;

ICamera* menuCam;
ICamera* raceCam;
ICamera* winCam;

IMesh* carMesh;
IMesh* trackMesh;
IMesh* checkpointMesh;
IMesh* obstacleMesh;
IMesh* wallMesh;
IMesh* garageMesh;
IMesh* tribuneMesh;
IMesh* lampMesh;

IModel* track;
IModel* playerCar;
std::vector<IModel*> checkpoints;
std::vector<IModel*> obstacles;
std::vector<IModel*> decorations;

ISprite* minimap;
ISprite* playerIcon;
ISprite* aiIcon;

int difficultyLevel = 1;
int trackSelection = 1;
bool showMenu = true;
bool raceStarted = false;
bool raceFinished = false;
bool paused = false;
int currentCheckpoint = 0;
int lapsCompleted = 0;
float raceTimer = 0.0f;
float playerHealth = 100.0f;
int score = 0;

struct AICar {
    IModel* model;
    int nextCheckpoint = 0;
};
std::vector<AICar> aiCars;

// === Прототипы ===
void LoadTrack();
void StartRace();
void DisplayMenu();
void DisplayWin();
void DisplayPauseMenu();
void UpdateHUD();
void UpdateMinimap();
void CheckCollisions(float);
void UpdateAI(float);
void LoadCheckpoints(float[][2]);
void LoadObstacles();
void LoadDecorations();
void GenerateCountdown(float);

int main() {
    myEngine->StartWindowed();
    myEngine->AddMediaFolder("Media");

    font = myEngine->LoadFont("Arial", 24);
    titleFont = myEngine->LoadFont("Comic Sans MS", 36);

    menuCam = myEngine->CreateCamera(kManual);
    menuCam->SetPosition(0, 150, -250);
    menuCam->LookAt(0, 0, 0);
    menuCam->Activate();

    raceCam = myEngine->CreateCamera(kManual);
    winCam = myEngine->CreateCamera(kManual);
    winCam->SetPosition(0, 100, -100);
    winCam->LookAt(0, 0, 0);

    carMesh = myEngine->LoadMesh("car.x");
    trackMesh = myEngine->LoadMesh("track.x");
    checkpointMesh = myEngine->LoadMesh("checkpoint.x");
    obstacleMesh = myEngine->LoadMesh("barrel.x");
    wallMesh = myEngine->LoadMesh("wall.x");
    garageMesh = myEngine->LoadMesh("garage.x");
    tribuneMesh = myEngine->LoadMesh("tribune.x");
    lampMesh = myEngine->LoadMesh("lamp.x");

    minimap = myEngine->CreateSprite("minimap.png", 640, 360);
    playerIcon = myEngine->CreateSprite("playerIcon.png", 640, 360);
    aiIcon = myEngine->CreateSprite("aiIcon.png", 640, 360);

    track = trackMesh->CreateModel(0, 0, 0);
    LoadTrack();

    while (myEngine->IsRunning()) {
        myEngine->DrawScene();
        float frameTime = myEngine->Timer();

        if (showMenu) {
            DisplayMenu();
            if (myEngine->KeyHit(Key_1)) difficultyLevel = 1;
            if (myEngine->KeyHit(Key_2)) difficultyLevel = 2;
            if (myEngine->KeyHit(Key_3)) difficultyLevel = 3;
            if (myEngine->KeyHit(Key_Q)) { trackSelection = 1; LoadTrack(); }
            if (myEngine->KeyHit(Key_W)) { trackSelection = 2; LoadTrack(); }
            if (myEngine->KeyHit(Key_E)) { trackSelection = 3; LoadTrack(); }
            if (myEngine->KeyHit(Key_Space)) { StartRace(); GenerateCountdown(3); }
            continue;
        }

        if (paused) {
            DisplayPauseMenu();
            if (myEngine->KeyHit(Key_C)) paused = false;
            if (myEngine->KeyHit(Key_M)) { showMenu = true; raceFinished = false; paused = false; }
            continue;
        }

        if (raceFinished) {
            DisplayWin();
            if (myEngine->KeyHit(Key_R)) StartRace();
            if (myEngine->KeyHit(Key_M)) showMenu = true;
            continue;
        }

        if (myEngine->KeyHit(Key_P)) paused = true;

        if (myEngine->KeyHeld(Key_Up)) playerCar->MoveLocalZ(30 * frameTime);
        if (myEngine->KeyHeld(Key_Left)) playerCar->RotateY(-60 * frameTime);
        if (myEngine->KeyHeld(Key_Right)) playerCar->RotateY(60 * frameTime);

        Romanᅠ ᅠ ᅠ ᅠ, [07 / 06 / 2025 15:31]
            raceCam->SetPosition(playerCar->GetX() - 10, 10, playerCar->GetZ() - 20);
        raceCam->LookAt(playerCar);
        raceTimer += frameTime;

        if ((playerCar->GetPosition() - checkpoints[currentCheckpoint]->GetPosition()).Length() < checkpointRadius) {
            currentCheckpoint++;
            if (currentCheckpoint == checkpointCount) {
                currentCheckpoint = 0;
                lapsCompleted++;
                if (lapsCompleted >= maxLaps) {
                    raceFinished = true;
                    score = 1000 - static_cast<int>(raceTimer);
                    winCam->Activate();
                }
            }
        }

        CheckCollisions(frameTime);
        UpdateAI(frameTime);
        UpdateMinimap();
        UpdateHUD();

        if (playerHealth <= 0) {
            raceFinished = true;
            score = 0;
            winCam->Activate();
        }
    }

    myEngine->Stop();
    return 0;
}

// === Реализации функций ===

void LoadCheckpoints(float coords[][2]) {
    for (int i = 0; i < checkpointCount; ++i) {
        IModel* cp = checkpointMesh->CreateModel(coords[i][0], 0, coords[i][1]);
        cp->SetSkin("checkpoint.jpg");
        checkpoints.push_back(cp);
    }
}

void LoadObstacles() {
    float pos[3][2] = { {20, 50}, {-20, 60}, {0, 90} };
    for (int i = 0; i < 3; ++i) {
        IModel* obs = obstacleMesh->CreateModel(pos[i][0], 0, pos[i][1]);
        obs->SetSkin("barrel.jpg");
        obstacles.push_back(obs);
    }
}

void LoadDecorations() {
    for (int i = -50; i <= 50; i += 10) {
        IModel* wall1 = wallMesh->CreateModel(i, 0, -20);
        IModel* wall2 = wallMesh->CreateModel(i, 0, 120);
        wall1->SetSkin("wall.jpg");
        wall2->SetSkin("wall.jpg");
        decorations.push_back(wall1);
        decorations.push_back(wall2);
    }
    for (int i = 0; i <= 100; i += 10) {
        IModel* wall1 = wallMesh->CreateModel(-50, 0, i);
        IModel* wall2 = wallMesh->CreateModel(50, 0, i);
        wall1->SetSkin("wall.jpg");
        wall2->SetSkin("wall.jpg");
        decorations.push_back(wall1);
        decorations.push_back(wall2);
    }

    IModel* garage = garageMesh->CreateModel(-80, 0, -20);
    garage->SetSkin("garageSkin.jpg");
    decorations.push_back(garage);

    IModel* tribune = tribuneMesh->CreateModel(100, 0, 50);
    tribune->SetSkin("tribune.jpg");
    decorations.push_back(tribune);

    IModel* lamps[] = {
        lampMesh->CreateModel(20, 0, 10),
        lampMesh->CreateModel(20, 0, 100),
        lampMesh->CreateModel(-20, 0, 10),
        lampMesh->CreateModel(-20, 0, 100)
    };
    for (auto lamp : lamps)
        decorations.push_back(lamp);
}

void LoadTrack() {
    checkpoints.clear();
    obstacles.clear();
    decorations.clear();
    aiCars.clear();

    float easy[checkpointCount][2] = {
        {0, 0}, {40, 0}, {80, 30}, {80, 80}, {40, 110},
        {0, 110}, {-40, 80}, {-40, 30}, {0, 0}
    };
    float medium[checkpointCount][2] = {
        {0, 0}, {30, 20}, {60, 0}, {90, 20}, {60, 60},
        {30, 80}, {0, 60}, {-30, 30}, {0, 0}
    };
    float hard[checkpointCount][2] = {
        {0, 0}, {30, 0}, {60, 20}, {80, 50}, {60, 90},
        {30, 120}, {-10, 90}, {-30, 40}, {0, 0}
    };

    if (trackSelection == 1) LoadCheckpoints(easy);
    else if (trackSelection == 2) LoadCheckpoints(medium);
    else LoadCheckpoints(hard);

    LoadObstacles();
    LoadDecorations();

    playerCar = carMesh->CreateModel(0, 0, -50);
    playerCar->SetSkin("RedGlow.jpg");

    aiCars.push_back({ carMesh->CreateModel(5, 0, -50) });
    aiCars.back().model->SetSkin("BlueGlow.jpg");

    aiCars.push_back({ carMesh->CreateModel(-5, 0, -50) });
    aiCars.back().model->SetSkin("GreenGlow.jpg");
}

void StartRace() {
    raceStarted = true;
    raceFinished = false;
    showMenu = false;
    paused = false;
    currentCheckpoint = 0;
    lapsCompleted = 0;
    raceTimer = 0.0f;
    playerHealth = 100.0f;
    score = 0;

    Romanᅠ ᅠ ᅠ ᅠ, [07 / 06 / 2025 15:31]
        playerCar->SetPosition(0, 0, -50);
    aiCars[0].model->SetPosition(5, 0, -50);
    aiCars[1].model->SetPosition(-5, 0, -50);

    raceCam->AttachToParent(playerCar);
    raceCam->SetPosition(0, 5, -15);
    raceCam->RotateLocalX(10);
    raceCam->Activate();
}

void DisplayMenu() {
    titleFont->Draw("Select Difficulty: 1-Easy  2-Medium  3-Hard", 400, 240, kCyan);
    titleFont->Draw("Select Track: Q-Track1  W-Track2  E-Track3", 400, 280, kCyan);
    titleFont->Draw("Press SPACE to Start", 500, 340, kGreen);
}

void DisplayWin() {
    std::ostringstream msg;
    msg << ((playerHealth <= 0) ? "You Lost!" : "You Win!")
        << " Score: " << score << "\nPress R to restart or M to menu.";
    titleFont->Draw(msg.str().c_str(), 420, 300, kYellow);
}

void DisplayPauseMenu() {
    titleFont->Draw("PAUSED", 550, 250, kWhite);
    titleFont->Draw("Press C to Continue or M to Menu", 420, 300, kWhite);
}

void UpdateHUD() {
    std::ostringstream hud;
    hud << "Lap: " << lapsCompleted + 1 << "/" << maxLaps
        << " | Time: " << static_cast<int>(raceTimer)
        << " | Health: " << static_cast<int>(playerHealth)
        << " | Score: " << score;
    font->Draw(hud.str().c_str(), 20, 20, kWhite);
}

void UpdateMinimap() {
    playerIcon->SetPosition(playerCar->GetX() / 4 + 640, playerCar->GetZ() / 4 + 360);
    aiIcon->SetPosition(aiCars[0].model->GetX() / 4 + 640, aiCars[0].model->GetZ() / 4 + 360);
}

void CheckCollisions(float dt) {
    for (auto& obs : obstacles)
        if ((playerCar->GetPosition() - obs->GetPosition()).Length() < 10)
            playerHealth -= 10 * dt;

    for (auto& wall : decorations)
        if ((playerCar->GetPosition() - wall->GetPosition()).Length() < 10)
            playerHealth -= 20 * dt;
}

void UpdateAI(float dt) {
    float aiSpeed = 10.0f + difficultyLevel * 5;
    for (auto& ai : aiCars) {
        if (ai.nextCheckpoint >= checkpointCount) ai.nextCheckpoint = 0;
        IModel* target = checkpoints[ai.nextCheckpoint];
        float dx = target->GetX() - ai.model->GetX();
        float dz = target->GetZ() - ai.model->GetZ();
        float angle = atan2(dx, dz) * 180 / PI;
        float diff = angle - ai.model->GetYRot();
        if (diff > 180) diff -= 360;
        if (diff < -180) diff += 360;
        ai.model->RotateY(diff * 0.05f);
        ai.model->MoveLocalZ(aiSpeed * dt);
        if ((ai.model->GetPosition() - target->GetPosition()).Length() < checkpointRadius)
            ai.nextCheckpoint++;
    }
}

void GenerateCountdown(float seconds) {
    float timeLeft = seconds;
    while (timeLeft > 0) {
        myEngine->DrawScene();
        float dt = myEngine->Timer();
        timeLeft -= dt;
        std::ostringstream msg;
        msg << "Race starts in: " << static_cast<int>(timeLeft + 1);
        titleFont->Draw(msg.str().c_str(), 560, 300, kRed);
    }
}