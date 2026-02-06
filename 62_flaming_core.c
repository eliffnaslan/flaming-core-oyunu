#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>  // Dosya işlemleri için

// === Sabitler ve Yapılar ===
#define MAX_LEVELS 5
#define TRAIL_LENGTH 18
#define NUM_OBSTACLES 12
#define LASER_LENGTH 150
#define LASER_THICKNESS 13
#define EXPLOSION_PARTICLES 20
#define OBSTACLE_EXPLOSION_PARTICLES 15
#define BULLET_TIME_SCALE 0.1f
#define MAX_FIREBALLS 40
#define MAX_DEADLY_WALLS 10

typedef enum {
    OBSTACLE_LASER,
    OBSTACLE_SHOOTER,
    OBSTACLE_DEADLY_WALL
} ObstacleType;

typedef enum {
    SCREEN_MENU,
    SCREEN_LEVELS,
    SCREEN_SETTINGS,
    SCREEN_GAMEPLAY,
    SCREEN_VICTORY,
    SCREEN_GAMEOVER,
    SCREEN_ENDING,
    SCREEN_PAUSE
} GameScreen;

typedef struct {
    Vector2 startPos;
    Vector2 endPos;
    float thickness;
    bool active;
} DeadlyWall;

typedef struct {
    Vector2 position;
    float radius;
    float laserAngle;
    bool active;
    bool exploding;
    float explosionTimer;
    ObstacleType type;
    float shootTimer;
    float shootInterval;
} Obstacle;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    float alpha;
    Color color;
    bool active;
} ExplosionParticle;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
    Color color;
    float lifeTime;
} Fireball;

// === Global değişkenler ===
Music backgroundMusic;
float musicVolume = 0.5f;  // Varsayılan ses seviyesi (0.0 ile 1.0 arasında)
Sound explosionSound;  // Engel yok etme ses efekti
Sound destroyedBallSound;  // Beyaz top yandığında çalacak ses efekti
Sound levelCompletedSound;  // Level bitirme ses efekti
int screenWidth = 1470;
int screenHeight = 818;
GameScreen currentScreen = SCREEN_MENU;
GameScreen previousScreen;
bool levelUnlocked[MAX_LEVELS] = { true, false, false, false, false  };
int currentLevel = 0;
bool allLevelsCompleted = false;
Vector2 corePosition;
Vector2 velocity;
float coreRadius = 20.0f;
bool gameOver = false;
bool victory = false;
bool burned = false;
float burnTimer = 0.0f;
bool aiming = false;
Vector2 targetPosition;
float timeScale = 1.0f;
Vector2 trail[TRAIL_LENGTH];
int trailIndex = 0;
Obstacle obstacles[NUM_OBSTACLES];
bool explosionActive = false;
float explosionDuration = 0.0f;
ExplosionParticle explosionParticles[EXPLOSION_PARTICLES];
ExplosionParticle obstacleExplosions[NUM_OBSTACLES][OBSTACLE_EXPLOSION_PARTICLES];
bool trailActive = true;
bool isPaused = false;
bool bulletTimeActive = false;
Texture2D pauseTexture;
RenderTexture2D gameplayTexture;
Fireball fireballs[MAX_FIREBALLS];
DeadlyWall deadlyWalls[MAX_DEADLY_WALLS];
float bestTimes[MAX_LEVELS] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};  // Her level için en iyi zaman
float currentLevelStartTime = 0.0f;  // Mevcut level başlangıç zamanı
char scoresFileName[] = "scores.dat";  // Skor dosyası adı

// === Fonksiyon prototipleri ===
void InitExplosion(void);
void UpdateExplosion(void);
void DrawExplosion(void);
void InitObstacleExplosion(int obstacleIndex);
void UpdateObstacleExplosions(void);
void DrawObstacleExplosions(void);
void InitGameplay(void);
void UpdateGameplay(void);
void DrawGameplay(void);
void CaptureGameplayScreen(void);
void InitFireball(int index, Vector2 position, Vector2 targetPosition);
void UpdateFireballs(void);
void DrawFireballs(void);
void SetupLevel(int level);
void DrawPauseScreen(void);
void DrawMainMenu(void);
void DrawLevelScreen(void);
void DrawSettingsScreen(void);
void DrawVictoryScreen(void);
void DrawGameOverScreen(void);
void DrawEndingScreen(void);
void LoadGameResources(void);
void UnloadGameResources(void);
void QuitGame(void);
void LoadBestTimes(void);
void SaveBestTimes(void);

// === Fonksiyonlar ===
void LoadBestTimes(void) {
    FILE *file = fopen(scoresFileName, "rb");
    if (file != NULL) {
        fread(bestTimes, sizeof(float), MAX_LEVELS, file);
        fclose(file);
    }
    // Dosya yoksa bestTimes zaten 0.0f olarak başlatıldı
}

void SaveBestTimes(void) {
    FILE *file = fopen(scoresFileName, "wb");
    if (file != NULL) {
        fwrite(bestTimes, sizeof(float), MAX_LEVELS, file);
        fclose(file);
    }
}

void InitExplosion(void) {
    explosionActive = true;
    explosionDuration = 0.0f;
    
    for (int i = 0; i < EXPLOSION_PARTICLES; i++) {
        explosionParticles[i].position = corePosition;
        
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(100, 300);
        explosionParticles[i].velocity.x = cosf(angle) * speed;
        explosionParticles[i].velocity.y = sinf(angle) * speed;
        
        explosionParticles[i].radius = GetRandomValue(3, 8);
        explosionParticles[i].alpha = 1.0f;
        explosionParticles[i].active = true;
        
        int colorChoice = GetRandomValue(0, 2);
        if (colorChoice == 0) 
            explosionParticles[i].color = WHITE;
        else if (colorChoice == 1) 
            explosionParticles[i].color = BLUE;
        else 
            explosionParticles[i].color = GREEN;
    }
}

void UpdateExplosion(void) {
    if (!explosionActive) return;
    
    explosionDuration += GetFrameTime();
    
    for (int i = 0; i < EXPLOSION_PARTICLES; i++) {
        explosionParticles[i].position.x += explosionParticles[i].velocity.x * GetFrameTime();
        explosionParticles[i].position.y += explosionParticles[i].velocity.y * GetFrameTime();
        
        explosionParticles[i].alpha -= GetFrameTime() * 1.0f;
        if (explosionParticles[i].alpha < 0) explosionParticles[i].alpha = 0;
    }
    
    if (explosionDuration >= 1.5f) {
        explosionActive = false;
        CaptureGameplayScreen();
        currentScreen = SCREEN_GAMEOVER;
    }
}

void DrawExplosion(void) {
    if (!explosionActive) return;
    
    for (int i = 0; i < EXPLOSION_PARTICLES; i++) {
        if (!explosionParticles[i].active) continue;
        
        Color particleColor = explosionParticles[i].color;
        particleColor.a = (unsigned char)(explosionParticles[i].alpha * 255);
        
        DrawCircleV(explosionParticles[i].position, explosionParticles[i].radius, particleColor);
    }
}

void InitObstacleExplosion(int obstacleIndex) {
    obstacles[obstacleIndex].exploding = true;
    obstacles[obstacleIndex].explosionTimer = 0.0f;
    
    for (int i = 0; i < OBSTACLE_EXPLOSION_PARTICLES; i++) {
        obstacleExplosions[obstacleIndex][i].position = obstacles[obstacleIndex].position;
        
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(80, 200);
        obstacleExplosions[obstacleIndex][i].velocity.x = cosf(angle) * speed;
        obstacleExplosions[obstacleIndex][i].velocity.y = sinf(angle) * speed;
        
        obstacleExplosions[obstacleIndex][i].radius = GetRandomValue(2, 6);
        obstacleExplosions[obstacleIndex][i].alpha = 1.0f;
        obstacleExplosions[obstacleIndex][i].active = true;
        
        // Engel tipine göre farklı patlama renkleri
        if (obstacles[obstacleIndex].type == OBSTACLE_SHOOTER) {
            int colorChoice = GetRandomValue(0, 2);
            if (colorChoice == 0) 
                obstacleExplosions[obstacleIndex][i].color = YELLOW;
            else if (colorChoice == 1) 
                obstacleExplosions[obstacleIndex][i].color = RED;
            else 
                obstacleExplosions[obstacleIndex][i].color = ORANGE;
        } else {
            int colorChoice = GetRandomValue(0, 2);
            if (colorChoice == 0) 
                obstacleExplosions[obstacleIndex][i].color = BLACK;
            else if (colorChoice == 1) 
                obstacleExplosions[obstacleIndex][i].color = DARKGRAY;
            else 
                obstacleExplosions[obstacleIndex][i].color = GRAY;
        }
    }
}

void UpdateObstacleExplosions(void) {
    for (int j = 0; j < NUM_OBSTACLES; j++) {
        if (obstacles[j].exploding) {
            obstacles[j].explosionTimer += GetFrameTime();
            
            for (int i = 0; i < OBSTACLE_EXPLOSION_PARTICLES; i++) {
                if (!obstacleExplosions[j][i].active) continue;
                
                obstacleExplosions[j][i].position.x += obstacleExplosions[j][i].velocity.x * GetFrameTime();
                obstacleExplosions[j][i].position.y += obstacleExplosions[j][i].velocity.y * GetFrameTime();
                
                obstacleExplosions[j][i].alpha -= GetFrameTime() * 2.0f;
                if (obstacleExplosions[j][i].alpha < 0) {
                    obstacleExplosions[j][i].alpha = 0;
                    obstacleExplosions[j][i].active = false;
                }
            }
            
            if (obstacles[j].explosionTimer >= 0.5f) {
                obstacles[j].exploding = false;
                obstacles[j].active = false;
            }
        }
    }
}

void DrawObstacleExplosions(void) {
    for (int j = 0; j < NUM_OBSTACLES; j++) {
        if (!obstacles[j].exploding) continue;
        
        for (int i = 0; i < OBSTACLE_EXPLOSION_PARTICLES; i++) {
            if (!obstacleExplosions[j][i].active) continue;
            
            Color particleColor = obstacleExplosions[j][i].color;
            particleColor.a = (unsigned char)(obstacleExplosions[j][i].alpha * 255);
            
            DrawCircleV(obstacleExplosions[j][i].position, obstacleExplosions[j][i].radius, particleColor);
        }
    }
}

void InitFireball(int index, Vector2 position, Vector2 targetPosition) {
    fireballs[index].position = position;
    
    Vector2 direction = Vector2Subtract(targetPosition, position);
    Vector2 normDirection = Vector2Normalize(direction);
    float speed = 200.0f;
    
    fireballs[index].velocity = Vector2Scale(normDirection, speed);
    fireballs[index].radius = 8.0f;
    fireballs[index].active = true;
    fireballs[index].color = (Color){ 255, 69, 0, 255 }; // OrangeRed
    fireballs[index].lifeTime = 0.0f;
}

void UpdateFireballs(void) {
    float deltaTime = GetFrameTime() * timeScale;
    
    for (int i = 0; i < MAX_FIREBALLS; i++) {
        if (!fireballs[i].active) continue;
        
        fireballs[i].position.x += fireballs[i].velocity.x * deltaTime;
        fireballs[i].position.y += fireballs[i].velocity.y * deltaTime;
        fireballs[i].lifeTime += deltaTime;
        
        // Ekran dışına çıkanları deaktive et
        if (fireballs[i].position.x < -20 || fireballs[i].position.x > screenWidth + 20 ||
            fireballs[i].position.y < -20 || fireballs[i].position.y > screenHeight + 20 ||
            fireballs[i].lifeTime > 5.0f) {
            fireballs[i].active = false;
            continue;
        }
        
        // Beyaz topla çarpışma kontrolü
        if (CheckCollisionCircles(fireballs[i].position, fireballs[i].radius, corePosition, coreRadius)) {
            gameOver = true;
            burned = true;
            burnTimer = 0.0f;
            trailActive = false;

            PlaySound(destroyedBallSound); 
            
            for (int t = 0; t < TRAIL_LENGTH; t++) {
                trail[t] = (Vector2){ -1000, -1000 };
            }
            
            InitExplosion();
            corePosition = (Vector2){ -1000, -1000 };
            fireballs[i].active = false;
        }
    }
}

void DrawFireballs(void) {
    for (int i = 0; i < MAX_FIREBALLS; i++) {
        if (!fireballs[i].active) continue;
        
        // Ateş topunun merkezi
        DrawCircleV(fireballs[i].position, fireballs[i].radius, fireballs[i].color);
        
        // Ateş efekti için küçük parçacıklar
        for (int j = 0; j < 3; j++) {
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float distance = GetRandomValue(5, 12) / 10.0f * fireballs[i].radius;
            Vector2 particlePos = {
                fireballs[i].position.x + cosf(angle) * distance,
                fireballs[i].position.y + sinf(angle) * distance
            };
            
            Color particleColor = (Color){ 255, 255, 0, 200 }; // Sarı alev parçacıkları
            DrawCircleV(particlePos, fireballs[i].radius * 0.6f, particleColor);
        }
    }
}

// Level ayarlama fonksiyonu
void SetupLevel(int level) {
    // Tüm engelleri deaktive et
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        obstacles[i].active = false;
        obstacles[i].exploding = false;
    }
    
    // Tüm ateş toplarını deaktive et
    for (int i = 0; i < MAX_FIREBALLS; i++) {
        fireballs[i].active = false;
    }
    
    // Ölümcül duvarları deaktive et
    for (int i = 0; i < MAX_DEADLY_WALLS; i++) {
        deadlyWalls[i].active = false;
    }
    
    if (level == 0) {
        // Level 1: 4 lazer engel
        obstacles[0] = (Obstacle){ {367, 204}, 20, 0.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[1] = (Obstacle){ {1103, 186}, 20, 90.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[2] = (Obstacle){ {459, 577}, 20, 180.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[3] = (Obstacle){ {1011, 569}, 20, 270.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
    }
    else if (level == 1) {
        // Level 2: 4 lazer engel + 4 ateş topu fırlatan engel
        obstacles[0] = (Obstacle){ {276, 204}, 20, 0.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[1] = (Obstacle){ {1194, 204}, 20, 90.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[2] = (Obstacle){ {276, 613}, 20, 180.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[3] = (Obstacle){ {1194, 613}, 20, 270.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        
        // Ateş topu fırlatan engeller
        obstacles[4] = (Obstacle){ {735, 136}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 2.0f };
        obstacles[5] = (Obstacle){ {184, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 2.5f };
        obstacles[6] = (Obstacle){ {1286, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 2.2f };
        obstacles[7] = (Obstacle){ {735, 681}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 2.7f };
    }
    else if (level == 2) {
        // Level 3: Daha zor bir kombinasyon
        obstacles[0] = (Obstacle){ {367, 176}, 20, 45.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[1] = (Obstacle){ {1103, 176}, 20, 135.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[2] = (Obstacle){ {367, 611}, 20, 225.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[3] = (Obstacle){ {1103, 611}, 20, 315.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        
        // Ateş topu fırlatan engeller (daha kısa ateşleme aralıkları)
        obstacles[4] = (Obstacle){ {735, 204}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.8f };
        obstacles[5] = (Obstacle){ {367, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.5f };
        obstacles[6] = (Obstacle){ {1103, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.7f };
        obstacles[7] = (Obstacle){ {735, 613}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.6f };

        // Ölümcül duvarlar
        deadlyWalls[0].startPos = (Vector2){ 367, 409 };
        deadlyWalls[0].endPos = (Vector2){ 735, 613 };
        deadlyWalls[0].thickness = 3.0f;
        deadlyWalls[0].active = true;

        deadlyWalls[1].startPos = (Vector2){ 1103, 409  };
        deadlyWalls[1].endPos = (Vector2){ 735, 204 };
        deadlyWalls[1].thickness = 3.0f;
        deadlyWalls[1].active = true;
    }
    else if (level == 3) {
        // Level 4: Level 3'ün aynısı + 8 ölümcül duvar
    
        // Level 3'ün lazer engelleri
        obstacles[0] = (Obstacle){ {367, 176}, 20, 45.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[1] = (Obstacle){ {1103, 176}, 20, 135.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[2] = (Obstacle){ {367, 611}, 20, 225.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[3] = (Obstacle){ {1103, 611}, 20, 315.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
    
        // Level 3'ün shooter engelleri
        obstacles[4] = (Obstacle){ {735, 204}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.8f };
        obstacles[5] = (Obstacle){ {367, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.5f };
        obstacles[6] = (Obstacle){ {1103, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.7f };
        obstacles[7] = (Obstacle){ {735, 613}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.6f };

        // Level 3'ün eski 2 ölümcül duvarı
        deadlyWalls[0].startPos = (Vector2){ 367, 409 };
        deadlyWalls[0].endPos = (Vector2){ 735, 613 };
        deadlyWalls[0].thickness = 3.0f;
        deadlyWalls[0].active = true;

        deadlyWalls[1].startPos = (Vector2){ 1103, 409 };
        deadlyWalls[1].endPos = (Vector2){ 735, 204 };
        deadlyWalls[1].thickness = 3.0f;
        deadlyWalls[1].active = true;

        // Yeni 8 ölümcül duvar
        deadlyWalls[2].startPos = (Vector2){ 367, 176 };
        deadlyWalls[2].endPos = (Vector2){ 120, 409 };
        deadlyWalls[2].thickness = 3.0f;
        deadlyWalls[2].active = true;

        deadlyWalls[3].startPos = (Vector2){ 367, 611 };
        deadlyWalls[3].endPos = (Vector2){ 120, 409 };
        deadlyWalls[3].thickness = 3.0f;
        deadlyWalls[3].active = true;

        deadlyWalls[4].startPos = (Vector2){ 1103, 176 };
        deadlyWalls[4].endPos = (Vector2){ 1300, 409 };
        deadlyWalls[4].thickness = 3.0f;
        deadlyWalls[4].active = true;

        deadlyWalls[5].startPos = (Vector2){ 1103, 611 };
        deadlyWalls[5].endPos = (Vector2){ 1300, 409 };
        deadlyWalls[5].thickness = 3.0f;
        deadlyWalls[5].active = true;

        deadlyWalls[6].startPos = (Vector2){ 367, 611 };
        deadlyWalls[6].endPos = (Vector2){ 735, 750 };
        deadlyWalls[6].thickness = 3.0f;
        deadlyWalls[6].active = true;

        deadlyWalls[7].startPos = (Vector2){ 1103, 611 };
        deadlyWalls[7].endPos = (Vector2){ 735, 750 };
        deadlyWalls[7].thickness = 3.0f;
        deadlyWalls[7].active = true;

        // 7. duvar
        deadlyWalls[8].startPos = (Vector2){ 367, 176 };
        deadlyWalls[8].endPos = (Vector2){ 735, 60 };
        deadlyWalls[8].thickness = 3.0f;
        deadlyWalls[8].active = true;

        // 8. duvar  
        deadlyWalls[9].startPos = (Vector2){ 1103, 176 };
        deadlyWalls[9].endPos = (Vector2){ 735, 60 };
        deadlyWalls[9].thickness = 3.0f;
        deadlyWalls[9].active = true;
    }
else if (level == 4) {
    // Level 5: Level 4'ün aynısı + kesişim noktalarında shooter engeller
    
    // Level 3'ün lazer engelleri
        obstacles[0] = (Obstacle){ {367, 176}, 20, 45.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[1] = (Obstacle){ {1103, 176}, 20, 135.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[2] = (Obstacle){ {367, 611}, 20, 225.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
        obstacles[3] = (Obstacle){ {1103, 611}, 20, 315.0f, true, false, 0.0f, OBSTACLE_LASER, 0.0f, 0.0f };
    
        // Level 3'ün shooter engelleri
        obstacles[4] = (Obstacle){ {735, 204}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.8f };
        obstacles[5] = (Obstacle){ {367, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.5f };
        obstacles[6] = (Obstacle){ {1103, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.7f };
        obstacles[7] = (Obstacle){ {735, 613}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 1.6f };

        // Level 3'ün eski 2 ölümcül duvarı
        deadlyWalls[0].startPos = (Vector2){ 367, 409 };
        deadlyWalls[0].endPos = (Vector2){ 735, 613 };
        deadlyWalls[0].thickness = 3.0f;
        deadlyWalls[0].active = true;

        deadlyWalls[1].startPos = (Vector2){ 1103, 409 };
        deadlyWalls[1].endPos = (Vector2){ 735, 204 };
        deadlyWalls[1].thickness = 3.0f;
        deadlyWalls[1].active = true;

        // Yeni 8 ölümcül duvar
        deadlyWalls[2].startPos = (Vector2){ 367, 176 };
        deadlyWalls[2].endPos = (Vector2){ 120, 409 };
        deadlyWalls[2].thickness = 3.0f;
        deadlyWalls[2].active = true;

        deadlyWalls[3].startPos = (Vector2){ 367, 611 };
        deadlyWalls[3].endPos = (Vector2){ 120, 409 };
        deadlyWalls[3].thickness = 3.0f;
        deadlyWalls[3].active = true;

        deadlyWalls[4].startPos = (Vector2){ 1103, 176 };
        deadlyWalls[4].endPos = (Vector2){ 1300, 409 };
        deadlyWalls[4].thickness = 3.0f;
        deadlyWalls[4].active = true;

        deadlyWalls[5].startPos = (Vector2){ 1103, 611 };
        deadlyWalls[5].endPos = (Vector2){ 1300, 409 };
        deadlyWalls[5].thickness = 3.0f;
        deadlyWalls[5].active = true;

        deadlyWalls[6].startPos = (Vector2){ 367, 611 };
        deadlyWalls[6].endPos = (Vector2){ 735, 750 };
        deadlyWalls[6].thickness = 3.0f;
        deadlyWalls[6].active = true;

        deadlyWalls[7].startPos = (Vector2){ 1103, 611 };
        deadlyWalls[7].endPos = (Vector2){ 735, 750 };
        deadlyWalls[7].thickness = 3.0f;
        deadlyWalls[7].active = true;

        // 7. duvar
        deadlyWalls[8].startPos = (Vector2){ 367, 176 };
        deadlyWalls[8].endPos = (Vector2){ 735, 60 };
        deadlyWalls[8].thickness = 3.0f;
        deadlyWalls[8].active = true;

        // 8. duvar  
        deadlyWalls[9].startPos = (Vector2){ 1103, 176 };
        deadlyWalls[9].endPos = (Vector2){ 735, 60 };
        deadlyWalls[9].thickness = 3.0f;
        deadlyWalls[9].active = true;

        obstacles[8] = (Obstacle){ {120, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 4.8f };
        obstacles[9] = (Obstacle){ {1300, 409}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 4.5f };
        obstacles[10] = (Obstacle){ {735, 750}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 4.7f };
        obstacles[11] = (Obstacle){ {735, 60}, 20, 0.0f, true, false, 0.0f, OBSTACLE_SHOOTER, 0.0f, 4.6f };        
    // Kesişim noktalarında ek shooter engeller (mevcut obstacle dizisinde boş yer yoksa NUM_OBSTACLES'ı artırın)
    // Bu engelleri obstacles dizisinin sonuna ekleyin
    }

    
    // Patlama parçacıklarını sıfırla
    for (int j = 0; j < NUM_OBSTACLES; j++) {
        for (int i = 0; i < OBSTACLE_EXPLOSION_PARTICLES; i++) {
            obstacleExplosions[j][i].active = false;
        }
    }
}

void InitGameplay(void) {
    corePosition = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    velocity = (Vector2){ 0.0f, 0.0f };
    gameOver = false;
    victory = false;
    burned = false;
    burnTimer = 0.0f;
    aiming = false;
    timeScale = 1.0f;
    trailIndex = 0;
    trailActive = true;
    explosionActive = false;
    isPaused = false;
    bulletTimeActive = false;
    
    // Trail'i temizle
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        trail[i] = (Vector2){ -1000, -1000 };
    }
    
    // Level ayarlamalarını yap
    SetupLevel(currentLevel);

    // Level başlangıç zamanını kaydet
    currentLevelStartTime = GetTime();
}

void CaptureGameplayScreen(void) {
    BeginTextureMode(gameplayTexture);
        DrawGameplay();
    EndTextureMode();
}

void UpdateGameplay(void) {
    // Space tuşu kontrolü
    if (IsKeyPressed(KEY_SPACE)) {
        bulletTimeActive = !bulletTimeActive;
        timeScale = bulletTimeActive ? BULLET_TIME_SCALE : 1.0f;
        return;
    }
    
    // Pause butonu kontrolü
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        Rectangle pauseButton = { screenWidth - 50, 10, 40, 40 };
        
        if (CheckCollisionPointRec(mousePos, pauseButton)) {
            isPaused = true;
            CaptureGameplayScreen();
            previousScreen = currentScreen;
            currentScreen = SCREEN_PAUSE;
            return;
        }
    }
    
    // Patlama efekti varsa sadece patlamayı güncelle
    if (explosionActive) {
        UpdateExplosion();
        return;
    }
    
    UpdateObstacleExplosions();
    UpdateFireballs();
    
    if (gameOver || victory) {
        if (burned && !explosionActive) burnTimer += GetFrameTime();
        return;
    }
    
    // Mouse hedefleme kontrolü
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        Rectangle pauseButton = { screenWidth - 50, 10, 40, 40 };

        if (!CheckCollisionPointRec(mousePos, pauseButton)) {
            aiming = true;
            bulletTimeActive = true;
            timeScale = BULLET_TIME_SCALE;
            targetPosition = mousePos;
        }
    }
    else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && aiming) {
        aiming = false;
        bulletTimeActive = false;
        timeScale = 1.0f;
        Vector2 direction = Vector2Subtract(targetPosition, corePosition);
        Vector2 normDirection = Vector2Normalize(direction);
        velocity = Vector2Scale(normDirection, 500.0f);
    }
   
    // Oyuncu hareketini güncelle
    corePosition.x += velocity.x * GetFrameTime() * timeScale;
    corePosition.y += velocity.y * GetFrameTime() * timeScale;
    
    // Ekran sınırları kontrolü
    if ((corePosition.x - coreRadius <= 0 && velocity.x < 0) || 
        (corePosition.x + coreRadius >= screenWidth && velocity.x > 0)) {
        velocity.x = -velocity.x;
    }
    if ((corePosition.y - coreRadius <= 0 && velocity.y < 0) || 
        (corePosition.y + coreRadius >= screenHeight && velocity.y > 0)) {
        velocity.y = -velocity.y;
    }
    
    // Trail güncelleme
    if (trailActive) {
        trail[trailIndex] = corePosition;
        trailIndex = (trailIndex + 1) % TRAIL_LENGTH;
    }
    
    // Engel kontrolleri
    int activeObstacles = 0;
    
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        if (!obstacles[i].active || obstacles[i].exploding) continue;
        
        activeObstacles++;
        
        if (obstacles[i].type == OBSTACLE_LASER) {
            // Lazer engelleri güncelle
            float laserRotationSpeed = bulletTimeActive ? 90.0f : 180.0f;
            obstacles[i].laserAngle += laserRotationSpeed * GetFrameTime() * (bulletTimeActive ? 1.0f : timeScale);
            
            if (obstacles[i].laserAngle >= 360.0f) {
                obstacles[i].laserAngle -= 360.0f;
            }
        
            Vector2 laserEnd = {
                obstacles[i].position.x + cosf(DEG2RAD * obstacles[i].laserAngle) * LASER_LENGTH,
                obstacles[i].position.y + sinf(DEG2RAD * obstacles[i].laserAngle) * LASER_LENGTH
            };

            // Lazer çarpışma kontrolü
            if (CheckCollisionPointLine(corePosition, obstacles[i].position, laserEnd, LASER_THICKNESS / 2 + coreRadius)) {
                gameOver = true;
                burned = true;
                burnTimer = 0.0f;
                trailActive = false;

                PlaySound(destroyedBallSound); 
                
                for (int t = 0; t < TRAIL_LENGTH; t++) {
                    trail[t] = (Vector2){ -1000, -1000 };
                }
                
                InitExplosion();
                corePosition = (Vector2){ -1000, -1000 };
            }
        }
        else if (obstacles[i].type == OBSTACLE_SHOOTER) {
            // Ateş topu fırlatan engelleri güncelle
            obstacles[i].shootTimer += GetFrameTime() * timeScale;
            
            // Ateşleme aralığı tamamlandığında yeni ateş topu fırlat
            if (obstacles[i].shootTimer >= obstacles[i].shootInterval) {
                obstacles[i].shootTimer = 0.0f;
                
                // Boş bir fireball slot'u bul
                for (int j = 0; j < MAX_FIREBALLS; j++) {
                    if (!fireballs[j].active) {
                        InitFireball(j, obstacles[i].position, corePosition);
                        break;
                    }
                }
            }
        }

        // Engel çarpışma kontrolü
        if (CheckCollisionCircles(corePosition, coreRadius, obstacles[i].position, obstacles[i].radius)) {
            PlaySound(explosionSound); 
            InitObstacleExplosion(i);
            
            Vector2 collisionPoint = Vector2Normalize(Vector2Subtract(obstacles[i].position, corePosition));
            collisionPoint = Vector2Scale(collisionPoint, coreRadius);
            collisionPoint = Vector2Add(corePosition, collisionPoint);
            
            Vector2 normal = Vector2Normalize(Vector2Subtract(collisionPoint, obstacles[i].position));
            Vector2 reflection = Vector2Subtract(velocity, Vector2Scale(normal, 2 * Vector2DotProduct(velocity, normal)));
            
            float speed = Vector2Length(velocity);
            velocity = Vector2Scale(Vector2Normalize(reflection), speed);
        }
    }
    
    // Ölümcül duvar çarpışma kontrolü (sadece level 3'te)
    if (currentLevel >= 2) {
        for (int i = 0; i < MAX_DEADLY_WALLS; i++) {
            if (!deadlyWalls[i].active) continue;
    
            if (CheckCollisionPointLine(corePosition, 
                           deadlyWalls[i].startPos, 
                           deadlyWalls[i].endPos, 
                           deadlyWalls[i].thickness / 2 + coreRadius)) {
                gameOver = true;
                burned = true;
                burnTimer = 0.0f;
                trailActive = false;

                PlaySound(destroyedBallSound); 
                
                for (int t = 0; t < TRAIL_LENGTH; t++) {
                    trail[t] = (Vector2){ -1000, -1000 };
                }
                
                InitExplosion();
                corePosition = (Vector2){ -1000, -1000 };
            }
        }
    }

    // Level tamamlama kontrolü
    int totalActiveObstacles = 0;
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        if (obstacles[i].active || obstacles[i].exploding) {
            totalActiveObstacles++;
        }
    }
    
    if (totalActiveObstacles == 0) {
        victory = true;
        PlaySound(levelCompletedSound);  // Level bitirme ses efekti çal
        velocity = (Vector2){ 0.0f, 0.0f };

        // Zaman hesaplama ve kaydetme
        float completionTime = GetTime() - currentLevelStartTime;
        if (bestTimes[currentLevel] == 0.0f || completionTime < bestTimes[currentLevel]) {
            bestTimes[currentLevel] = completionTime;
            SaveBestTimes();  // Yeni rekor varsa kaydet
        }
        
        for (int i = 0; i < TRAIL_LENGTH; i++) {
            trail[i] = trail[trailIndex];
        }
        
        trailActive = false;
        CaptureGameplayScreen();
        currentScreen = (currentLevel + 1 >= MAX_LEVELS) ? SCREEN_ENDING : SCREEN_VICTORY;
    }
}

void DrawGameplay() {
    ClearBackground(DARKGRAY);
    if (aiming) DrawRectangle(0, 0, screenWidth, screenHeight, Fade(WHITE, 0.2f));
    
    // Trail çizimi
    if (trailActive || victory) {
        Color trailColor = (Color){ 50, 150, 255, 255 };
        for (int i = 0; i < TRAIL_LENGTH; i++) {
            int index = (trailIndex + i) % TRAIL_LENGTH;
            float alpha = (float)i / (float)TRAIL_LENGTH;
            float pulse = 0.5f + 0.5f * sinf(GetTime() * 5.0f + i * 0.3f);

            Color fadedColor = trailColor;
            fadedColor.a = (unsigned char)(pulse * 255 * alpha);
            float sizeFactor = 1.0f - (0.5f * (1.0f - alpha));

            DrawCircleV(trail[index], coreRadius * 0.4f * sizeFactor, fadedColor);
        }
    }

    // Oyuncu çizimi
    if (!burned || burnTimer < 1.0f)
        DrawCircleV(corePosition, coreRadius, burned ? Fade(RED, 1.0f - burnTimer) : RAYWHITE);

    // Hedef çizgisi
    if (aiming) {
        Vector2 mousePos = GetMousePosition();
        Rectangle pauseButton = { screenWidth - 50, 10, 40, 40 };
        
        if (!CheckCollisionPointRec(mousePos, pauseButton)) {
            DrawLineV(corePosition, targetPosition, RED);
        }
    }

    // Engeller ve lazerler
    for (int i = 0; i < NUM_OBSTACLES; i++) {
        if (!obstacles[i].active) continue;
        
        if (!obstacles[i].exploding) {
            if (obstacles[i].type == OBSTACLE_LASER) {
                Vector2 laserEnd = {
                    obstacles[i].position.x + cosf(DEG2RAD * obstacles[i].laserAngle) * LASER_LENGTH,
                    obstacles[i].position.y + sinf(DEG2RAD * obstacles[i].laserAngle) * LASER_LENGTH
                };

                DrawCircleV(obstacles[i].position, obstacles[i].radius, BLACK);
                DrawLineEx(obstacles[i].position, laserEnd, LASER_THICKNESS, RED);
            }
            else if (obstacles[i].type == OBSTACLE_SHOOTER) {
                DrawCircleV(obstacles[i].position, obstacles[i].radius, ORANGE);
                
                // Ateşleme zamanına yaklaştıkça yanıp sönen efekt
                if (obstacles[i].shootTimer / obstacles[i].shootInterval > 0.7f) {
                    float chargePulse = sinf(obstacles[i].shootTimer * 8.0f);
                    chargePulse = (chargePulse + 1.0f) / 2.0f; // 0-1 aralığına normalize et
                    DrawCircleV(obstacles[i].position, obstacles[i].radius * 1.3f * chargePulse, 
                               Fade(YELLOW, 0.5f * chargePulse));
                }
            }
        }
    }

    // Ölümcül duvarları çiz (sadece level 3'te)
    if (currentLevel >= 2) {
        for (int i = 0; i < MAX_DEADLY_WALLS; i++) {
            if (!deadlyWalls[i].active) continue;
        
            Color wallColor = RED;
            float pulse = 0.7f + 0.3f * sinf(GetTime() * 4.0f);
            wallColor.a = (unsigned char)(pulse * 255);
        
            DrawLineEx(deadlyWalls[i].startPos, deadlyWalls[i].endPos, deadlyWalls[i].thickness, wallColor);
        }
    }
    
    DrawFireballs();
    DrawObstacleExplosions();
    DrawExplosion();
    
    // UI elementleri
    DrawText(TextFormat("Level: %d/%d", currentLevel + 1, MAX_LEVELS), 10, 10, 20, WHITE);

    // En iyi zamanı göster
    if (bestTimes[currentLevel] > 0.0f) {
        DrawText(TextFormat("Best: %.2f", bestTimes[currentLevel]), 10, 40, 20, YELLOW);
    }

    DrawTextureEx(pauseTexture, (Vector2){screenWidth - 50, 10}, 0.0f, 0.09f, WHITE);
    DrawText(bulletTimeActive ? "BULLET-TIME [active]" : "[passive] BULLET-TIME", 
        screenWidth/2 - 100, screenHeight - 30, 20, 
        bulletTimeActive ? LIME : GRAY);
}

void DrawPauseScreen() {
    DrawTextureRec(
        gameplayTexture.texture, 
        (Rectangle){ 0, 0, (float)gameplayTexture.texture.width, (float)-gameplayTexture.texture.height },
        (Vector2){ 0, 0 },
        Fade(WHITE, 0.5f)
    );

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
    DrawText("PAUSED", screenWidth/2 - MeasureText("PAUSED", 40)/2, screenHeight/2 - 40, 40, WHITE);
    
    Rectangle continueButton = { screenWidth/2 - 100, screenHeight/2 + 20, 200, 40 };
    Rectangle menuButton = { screenWidth/2 - 100, screenHeight/2 + 70, 200, 40 };
    
    if (GuiButton(continueButton, "CONTINUE")) {
        isPaused = false;
        bulletTimeActive = true;  // Bullet time'ı aktif et
        timeScale = BULLET_TIME_SCALE;  // Time scale'i bullet time değerine ayarla
        currentScreen = SCREEN_GAMEPLAY;
    }
    
    if (GuiButton(menuButton, "MAIN MENU")) {
        currentScreen = SCREEN_MENU;
    }
}

void DrawMainMenu() {
    ClearBackground(DARKGRAY);
    
    DrawText("FLAMING CORE", screenWidth/2 - MeasureText("FLAMING CORE", 50)/2, 100, 50, WHITE);
    
    Rectangle playButton = { screenWidth/2 - 100, 250, 200, 40 };
    Rectangle levelsButton = { screenWidth/2 - 100, 300, 200, 40 };
    Rectangle settingsButton = { screenWidth/2 - 100, 350, 200, 40 };
    Rectangle exitButton = { screenWidth/2 - 100, 400, 200, 40 };
    
    if (GuiButton(playButton, "PLAY")) {
        currentLevel = 0;
        InitGameplay();
        currentScreen = SCREEN_GAMEPLAY;
    }
    
    if (GuiButton(levelsButton, "LEVELS")) currentScreen = SCREEN_LEVELS;
    if (GuiButton(settingsButton, "SETTINGS")) currentScreen = SCREEN_SETTINGS;
    if (GuiButton(exitButton, "EXIT")) QuitGame();
    
    DrawText("Use LEFT MOUSE to aim and shoot", screenWidth/2 - MeasureText("Use LEFT MOUSE to aim and shoot", 20)/2, 500, 20, LIGHTGRAY);
    DrawText("Use SPACE for bullet-time", screenWidth/2 - MeasureText("Use SPACE for bullet-time", 20)/2, 530, 20, LIGHTGRAY);
}

void DrawLevelScreen() {
    ClearBackground(DARKGRAY);
    
    DrawText("SELECT LEVEL", screenWidth/2 - MeasureText("SELECT LEVEL", 40)/2, 100, 40, WHITE);
    
    for (int i = 0; i < MAX_LEVELS; i++) {
        Rectangle levelButton = { screenWidth/2 - 100, 200 + i*60, 200, 40 };
        bool enabled = levelUnlocked[i];
        
        if (GuiButton(levelButton, TextFormat("LEVEL %d", i+1)) && enabled) {
            currentLevel = i;
            InitGameplay();
            currentScreen = SCREEN_GAMEPLAY;
        }
        
        if (!enabled) {
            DrawRectangle(levelButton.x, levelButton.y, levelButton.width, levelButton.height, Fade(BLACK, 0.5f));
            DrawText("LOCKED", levelButton.x + levelButton.width/2 - MeasureText("LOCKED", 20)/2, 
                     levelButton.y + levelButton.height/2 - 10, 20, GRAY);
        }
    }
    
    Rectangle backButton = { screenWidth/2 - 100, 500, 200, 40 };
    if (GuiButton(backButton, "BACK")) currentScreen = SCREEN_MENU;
}

void DrawSettingsScreen() {
    ClearBackground(DARKGRAY);
    DrawText("SETTINGS", 660, 100, 30, PURPLE);

    DrawText("Music Volume", 660, 150, 20, LIGHTGRAY);
    GuiSlider((Rectangle){ 630, 170, 200, 20 }, "0", "100", &musicVolume, 0.0f, 1.0f);
    SetMusicVolume(backgroundMusic, musicVolume);


    
    if (GuiButton((Rectangle){ 630, 200, 200, 40 }, "RESET GAME")) {
        for (int i = 1; i < MAX_LEVELS; i++) levelUnlocked[i] = false;
        for (int i = 0; i < MAX_LEVELS; i++) bestTimes[i] = 0.0f;  // Skorları sıfırla
        SaveBestTimes();  // Sıfırlanmış skorları kaydet
        allLevelsCompleted = false;
    }
    
    if (GuiButton((Rectangle){ 680, 300, 100, 40 }, "BACK")) currentScreen = SCREEN_MENU;
}

void DrawVictoryScreen() {
    DrawTextureRec(
        gameplayTexture.texture, 
        (Rectangle){ 0, 0, (float)gameplayTexture.texture.width, (float)-gameplayTexture.texture.height },
        (Vector2){ 0, 0 },
        WHITE
    );

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(DARKGREEN, 0.8f));
    DrawText("LEVEL COMPLETED!", screenWidth/2 - MeasureText("LEVEL COMPLETED!", 40)/2, screenHeight/2 - 40, 40, WHITE);
    
    Rectangle nextButton = { screenWidth/2 - 100, screenHeight/2 + 20, 200, 40 };
    Rectangle menuButton = { screenWidth/2 - 100, screenHeight/2 + 70, 200, 40 };
    
    if (GuiButton(nextButton, "NEXT LEVEL")) {
        if (currentLevel + 1 < MAX_LEVELS) {
            currentLevel++;
            levelUnlocked[currentLevel] = true;
            InitGameplay();
            currentScreen = SCREEN_GAMEPLAY;
        }
    }
    
    if (GuiButton(menuButton, "MAIN MENU")) {
        levelUnlocked[currentLevel + 1] = true;
        currentScreen = SCREEN_MENU;
    }
}

void DrawGameOverScreen() {
    DrawTextureRec(
        gameplayTexture.texture, 
        (Rectangle){ 0, 0, (float)gameplayTexture.texture.width, (float)-gameplayTexture.texture.height },
        (Vector2){ 0, 0 },
        WHITE
    );

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(MAROON, 0.8f));
    DrawText("GAME OVER!", screenWidth/2 - MeasureText("GAME OVER!", 40)/2, screenHeight/2 - 40, 40, WHITE);
    
    Rectangle retryButton = { screenWidth/2 - 100, screenHeight/2 + 20, 200, 40 };
    Rectangle menuButton = { screenWidth/2 - 100, screenHeight/2 + 70, 200, 40 };
    
    if (GuiButton(retryButton, "RETRY")) {
        InitGameplay();
        currentScreen = SCREEN_GAMEPLAY;
    }
    
    if (GuiButton(menuButton, "MAIN MENU")) currentScreen = SCREEN_MENU;
}

void DrawEndingScreen() {
    ClearBackground(BLACK);
    
    DrawText("CONGRATULATIONS!", screenWidth/2 - MeasureText("CONGRATULATIONS!", 40)/2, 150, 40, WHITE);
    DrawText("You've completed all levels!", screenWidth/2 - MeasureText("You've completed all levels!", 30)/2, 220, 30, LIGHTGRAY);
    DrawText("Thanks for playing!", screenWidth/2 - MeasureText("Thanks for playing!", 25)/2, 300, 25, LIGHTGRAY);
    
    Rectangle menuButton = { screenWidth/2 - 100, 400, 200, 40 };
    
    if (GuiButton(menuButton, "MAIN MENU")) {
        allLevelsCompleted = true;
        currentScreen = SCREEN_MENU;
    }
}

void LoadGameResources() {
    gameplayTexture = LoadRenderTexture(screenWidth, screenHeight);
    pauseTexture = LoadTexture("pause_icon.png");
    explosionSound = LoadSound("explosion.mp3");
    destroyedBallSound = LoadSound("destroyedBall.mp3");
    levelCompletedSound = LoadSound("levelcompleted.mp3");
}

void UnloadGameResources() {
    UnloadRenderTexture(gameplayTexture);
    UnloadTexture(pauseTexture);
    UnloadSound(explosionSound); 
    UnloadSound(destroyedBallSound);
    UnloadSound(levelCompletedSound);
}

void QuitGame() {
    CloseWindow();  // Raylib'in pencereyi kapatma işlevi
}

int main() {
    InitWindow(screenWidth, screenHeight, "Flaming Core");
    InitAudioDevice();
    backgroundMusic = LoadMusicStream("Galactic_Drift.mp3");
    PlayMusicStream(backgroundMusic);
    SetMusicVolume(backgroundMusic, musicVolume);
    SetTargetFPS(60);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    
    LoadGameResources();
    LoadBestTimes();  // Skorları yükle

    // Ana oyun döngüsü
    while (!WindowShouldClose()) {
        UpdateMusicStream(backgroundMusic);
        // Güncelleme
        switch (currentScreen) {
            case SCREEN_GAMEPLAY:
                UpdateGameplay();
                break;
            default:
                break;
        }
        
        // Çizim
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        switch (currentScreen) {
            case SCREEN_MENU:
                DrawMainMenu();
                break;
            case SCREEN_LEVELS:
                DrawLevelScreen();
                break;
            case SCREEN_SETTINGS:
                DrawSettingsScreen();
                break;
            case SCREEN_GAMEPLAY:
                DrawGameplay();
                break;
            case SCREEN_VICTORY:
                DrawVictoryScreen();
                break;
            case SCREEN_GAMEOVER:
                DrawGameOverScreen();
                break;
            case SCREEN_ENDING:
                DrawEndingScreen();
                break;
            case SCREEN_PAUSE:
                DrawPauseScreen();
                break;
            default:
                break;
        }
        
        EndDrawing();
    }
    
    UnloadGameResources();
    UnloadMusicStream(backgroundMusic);
    CloseWindow();
    return 0;
}