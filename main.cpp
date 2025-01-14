#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <SDL_haptic.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <climits>
#include <map>
#include <ctime>
#include <chrono>

#include <dirent.h>

#include "Player.hpp"
#include "Map.hpp"
#include "Rectangle.hpp"
#include "Vector2D.hpp"
#include "Point.hpp"
#include "Collider.hpp"
#include "Explosion.hpp"
#include "Container.hpp"
#include "Powerup.hpp"
#include "Defines.hpp"
#include "Options.hpp"
#include "PlayerInput.hpp"
#include "InputManager.hpp"
#include "DestructibleBlock.hpp"

/* Global variable definitions */

bool gRunning = true;
bool software = false;
int gRndTiles = 0;
std::string gWallTileNames[MAX_TILESET + 1] = { WALL_TILES_DIRT, WALL_TILES_ICE, WALL_TILES_URBAN };

Mix_Music* gGameMusic[MAX_TILESET + 1] = { NULL, NULL, NULL };
Mix_Music* introMusic[MAX_TILESET + 1] = { NULL, NULL, NULL };
Mix_Music* menuMusic = { NULL };
Mix_Chunk* sfxFire = NULL;
Mix_Chunk* sfxBounce[3] = { NULL, NULL, NULL };
Mix_Chunk* sfxDie = NULL;
Mix_Chunk* sfxPowerupSpeed[MAX_TILESET + 1] = { NULL, NULL, NULL };
Mix_Chunk* sfxPowerupBounce[MAX_TILESET + 1] = { NULL, NULL, NULL };
Mix_Chunk* sfxPowerupBullet[MAX_TILESET + 1] = { NULL, NULL, NULL };
Mix_Chunk* sfxBulletWall = NULL;
Mix_Chunk* sfxBulletBrick = NULL;
Mix_Chunk* sfxMenu = NULL;
Mix_Chunk* sfxMenuConfirm = NULL;
Mix_Chunk* sfxPause = NULL;
Mix_Chunk* sfxUnpause = NULL;
Mix_Chunk* sfxReady = NULL;
Mix_Chunk* sfxNotReady = NULL;
std::default_random_engine generator(time(NULL));
std::uniform_int_distribution<int> distTiles(0,MAX_TILESET);

bool playersIn[4] = { false, false, false, false };
int playersInCount = 0;

std::string mapfilename;

bool altHeld = false;
bool gFullscreen = true;

SDL_Window* win = NULL;
SDL_Renderer* ren = NULL;
InputManager* gInput = nullptr;
uint32_t rmask, gmask, bmask, amask;

std::string basePath;
void NewExplosion(const float x, const float y, SDL_Renderer* ren, std::map<int, RenderableObject*>& vRenderable, std::vector<Explosion*>& vExplosions);

int Title();
int Menu();
int DisplayControls();
Options* OptionsMenu();
int WinScreen(bool (&winningPlayer)[4], Player (&players)[4]);
void Quit(int status);
int Pause();
int ConfirmQuit();


int main(int argc, char** argv) {

    freopen("error.log", "w", stdout);

    unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
    generator = std::default_random_engine(seed1);

    uint32_t ticks = SDL_GetTicks();
    uint32_t old_time = SDL_GetTicks();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER |SDL_INIT_JOYSTICK) < 0) {
        std::cout << "Error initializing SDL: " << SDL_GetError() << std::endl;
        Quit(1);
    }

    #ifdef _DEBUG_BUILD
    std::cout << "SDL Initialized" << std::endl;
    #endif

    basePath = SDL_GetBasePath();

    #ifdef _DEBUG_BUILD
    std::cout << basePath << std::endl;
    #endif

    if (IMG_Init(IMG_INIT_PNG) < 0) {
        std::cout << "Error initializing SDL_IMG: " << SDL_GetError() << std::endl;
        Quit(2);
    }

    #ifdef _DEBUG_BUILD
    std::cout << "IMG Initialized" << std::endl;
    #endif

    if (TTF_Init() != 0) {
        std::cout << "Error initializing SDL_TTF: " << SDL_GetError() << std::endl;
        Quit(3);
    }

    #ifdef _DEBUG_BUILD
    std::cout << "TTF Initialized" << std::endl;
    #endif
    if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
        std::cout << "Unable to initialize OGG playback. SDL_Error: " << Mix_GetError() << std::endl;
        std::cout << Mix_GetError() << std::endl;
        Quit(4);
    }
    if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
    {
        std::cout << "Error initializing SDL_Mixer: " << Mix_GetError() << std::endl;
        Quit(4);
    }

    #ifdef _DEBUG_BUILD
    std::cout << "Mixer Initialized" << std::endl;
    #endif

    gGameMusic[0] = Utility::LoadMusic(GAME_MUSIC1);
    gGameMusic[1] = Utility::LoadMusic(GAME_MUSIC2);
    gGameMusic[2] = Utility::LoadMusic(GAME_MUSIC3);

    introMusic[0] = Utility::LoadMusic(INTRO_MUSIC1);
    introMusic[1] = Utility::LoadMusic(INTRO_MUSIC2);
    introMusic[2] = Utility::LoadMusic(INTRO_MUSIC3);

    menuMusic = Utility::LoadMusic(MENU_MUSIC);
    sfxFire = Utility::LoadSound(SFX_FIRE);
    sfxBounce[0] = Utility::LoadSound(SFX_BOUNCE);
    sfxBounce[1] = Utility::LoadSound(SFX_BOUNCE2);
    sfxBounce[2] = Utility::LoadSound(SFX_BOUNCE3);
    sfxDie = Utility::LoadSound(SFX_DIE);

    sfxPowerupBounce[0] = Utility::LoadSound(SFX_POWERUP_BOUNCE1);
    sfxPowerupBounce[1] = Utility::LoadSound(SFX_POWERUP_BOUNCE1);
    sfxPowerupBounce[2] = Utility::LoadSound(SFX_POWERUP_BOUNCE1);

    sfxPowerupBullet[0] = Utility::LoadSound(SFX_POWERUP_BULLET1);
    sfxPowerupBullet[1] = Utility::LoadSound(SFX_POWERUP_BULLET2);
    sfxPowerupBullet[2] = Utility::LoadSound(SFX_POWERUP_BULLET3);

    sfxPowerupSpeed[0] = Utility::LoadSound(SFX_POWERUP_SPEED1);
    sfxPowerupSpeed[1] = Utility::LoadSound(SFX_POWERUP_SPEED2);
    sfxPowerupSpeed[2] = Utility::LoadSound(SFX_POWERUP_SPEED3);

    sfxBulletWall = Utility::LoadSound(SFX_BULLET_WALL);
    sfxBulletBrick = Utility::LoadSound(SFX_BULLET_BRICK);
    sfxMenu = Utility::LoadSound(SFX_MENU);
    sfxMenuConfirm = Utility::LoadSound(SFX_MENU_CONFIRM);
    sfxPause = Utility::LoadSound(SFX_PAUSE);
    sfxUnpause = Utility::LoadSound(SFX_UNPAUSE);

    sfxReady = Utility::LoadSound(SFX_READY);
    sfxNotReady = Utility::LoadSound(SFX_NOTREADY);

    #ifdef _DEBUG_BUILD
    std::cout << "Resources loaded" << std::endl;
    #endif

    if (gGameMusic[0] == nullptr || gGameMusic[1] == nullptr || gGameMusic[2] == nullptr) {
        std::cout << "Could not load music. Exiting." << std::endl;
        Quit(5);
    }

    if (sfxFire == nullptr || sfxBounce[0] == nullptr || sfxBounce[1] == nullptr
        || sfxBounce[2] == nullptr || sfxDie == nullptr ) {
            std::cout << "Could not load sound effects. Exiting." << std::endl;
        Quit(6);
    }

    if (sfxPowerupSpeed[0] == nullptr || sfxPowerupSpeed[1] == nullptr || sfxPowerupSpeed[2] == nullptr ||
        sfxPowerupBullet[0] == nullptr || sfxPowerupBullet[1] == nullptr || sfxPowerupBullet[2] == nullptr ||
        sfxPowerupBounce[0] == nullptr || sfxPowerupBounce[1] == nullptr || sfxPowerupBounce[2] == nullptr )
        {
        std::cout << "Could not load powerup sound effects. Exiting." << std::endl;
        Quit(7);
    }

    if (sfxMenu == nullptr || sfxMenuConfirm == nullptr || sfxBulletWall == nullptr || sfxBulletBrick == nullptr ||
        sfxPause == nullptr || sfxUnpause == nullptr || sfxReady == nullptr || sfxNotReady == nullptr) {
            std::cout << "Could not load sound effects. Exiting." << std::endl;
        Quit(6);
    }




    // Initialize joysticks
    gInput = new InputManager();
    #ifdef _DEBUG_BUILD
    std::cout << "Initialized joysticks." << std::endl;
    #endif // _DEBUG_BUILD

    int playerx, playery;
    playerx = (SCREEN_WIDTH / 2);
    playery = (SCREEN_HEIGHT / 2);

    win = SDL_CreateWindow("Tanks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        std::cout << "Error creating window. SDL_Error: " << SDL_GetError() << std::endl;
        Quit(8);
    }

    if (SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
        std::cout << "Error setting fullscreen video mode. SDL_Error: " << SDL_GetError() << std::endl;
    }

    SDL_RestoreWindow(win); // Win7 hack?
    SDL_ShowCursor(0);

    if (argc > 1) {
        if (strcmp(argv[1], "-software") == 0) {
            ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
            software = true;
            if (ren == NULL) {
                std::cout << "Error creating software renderer. Exiting. SDL_Error: " << SDL_GetError() << std::endl;
                Quit(9);
            }
        }
    } else {
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        if (ren == NULL) {
            std::cout << "Error creating accelerated renderer. Falling back to software. SDL_Error: " << SDL_GetError() << std::endl;
            ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
            software = true;
            if (ren == NULL) {
                std::cout << "Error creating software renderer. Exiting. SDL_Error: " << SDL_GetError() << std::endl;
                Quit(9);
            }
       }
    }


    if (!software)
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");  // make the scaled rendering look smoother.

    if (SDL_RenderSetLogicalSize(ren, SCREEN_WIDTH, SCREEN_HEIGHT) < 0) {
        std::cout << "Error setting logical size of renderer. SDL_Error: " << SDL_GetError() << std::endl;
        Quit(10);
    }

    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    #else
        amask = 0xff000000;
        bmask = 0x00ff0000;
        gmask = 0x0000ff00;
        rmask = 0x000000ff;
    #endif // SDL_BIG_ENDIAN

    SDL_Surface* rSurface = SDL_CreateRGBSurface(0, 1, 1, 32, rmask, gmask, bmask, amask);
    uint32_t backgroundColor[MAX_TILESET + 1] = { SDL_MapRGB(rSurface->format, 0x20, 0x20, 0x05),
                                            SDL_MapRGB(rSurface->format, 0x00, 0x00, 0x40),
                                            SDL_MapRGB(rSurface->format, 0x30, 0x30, 0x30)
                                        };
    SDL_FreeSurface(rSurface);

    bool loopGame = true;

    if (Title() == -1) {
            Quit(0);
    }

    #ifndef _DEMO
    if (DisplayControls() == -1) {
        Quit(0);
    }
    #endif

    while (loopGame) {

        #ifdef _DEMO
        if (DisplayControls() == -1) {
            Quit(0);
        }
        #endif


        Mix_FadeInMusic(menuMusic, -1, 1000);

        if (Menu() == -1) {
            loopGame = false;
            break;
        }

        #ifdef _DEMO
        Options* gameOptions = new Options(TIME_MATCH, 0, 0, 120, false);
        #else // _DEMO
        Options* gameOptions = OptionsMenu();
        if (gameOptions == nullptr) {
            loopGame = false;
            break;
        } else if (gameOptions->Back() == true) {
            delete gameOptions;
            continue;
        }
        #endif // _DEMO

        Mix_FadeOutMusic(500);
        SDL_Delay(500);
        uint32_t gameTime = gameOptions->GetTime() * 1000;

        #ifdef _LEVEL_DEBUG
        if (argc > 1)
            mapfilename = argv[1];
        else
            mapfilename = "default.d";
        #endif

        gRndTiles = distTiles(generator);

        Map m(mapfilename, gWallTileNames[gRndTiles], ren);
        if (!m.LoadSuccess()) {
            std::cout << "Level file " << mapfilename << " not a valid map.";
            exit(10);
        }

        Player players[4] = { Player(PLAYER1_TANK, 1, gameOptions->GetStock(), ren),
                                Player(PLAYER2_TANK, 2, gameOptions->GetStock(), ren),
                                Player(PLAYER3_TANK, 3, gameOptions->GetStock(),ren),
                                Player(PLAYER4_TANK, 4, gameOptions->GetStock(),ren) };

        for (int i = 0; i < 4; ++i) {
            if (!playersIn[i]) {
                //players[i].Die();
            }
            Vector2D startPos = m.GetStartPos(i + 1);
            if (startPos.GetX() == 0 && startPos.GetY() == 0) {
                players[i].SetX(playerx);
                players[i].SetY(playery);
            } else {
                players[i].SetX(startPos.GetX());
                players[i].SetY(startPos.GetY());

            }
        }

        for (int i = 0; i < 4; i++) {
            if (players[i].GetTexture() == nullptr) {
                std::cout << "Error loading player sprite: " << SDL_GetError() << std::endl;
                Quit(1);
            }
        }

        std::vector<Player*> vPlayers, vPlayersDelete;
        std::vector<Collider*> vStationary, vStationaryDelete;
        std::map<int, RenderableObject*> vRenderable, vRenderableDelete;
        std::map<int, Bullet*> vBullets, vBulletsDelete;
        std::vector<Explosion*> vExplosions, vExplosionsDelete;
        std::vector<Container*> vContainers, vContainersDelete;
        std::vector<Powerup*> vPowerups, vPowerupsDelete;
        std::vector<DestructibleBlock*> vDestructibleBlocks, vDestructibleBlocksDelete;

        bool winningPlayer[4] = {false, false, false, false};

        vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next++, &m));

        for (int i = 0; i < 4; ++i) {
            if (gInput->Player(i) == nullptr) {
                continue;
            }
            if (playersIn[i]) {
                if (gameOptions->GetMatchType() == STOCK_MATCH) {
                    winningPlayer[i] = true;
                }
                vPlayers.push_back(&players[i]);
                vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next++, &players[i]));
            } else {
                players[i].Die();
            }
        }

        std::vector<Collider>& mapColliders = m.GetColliders();

        for (Collider& c : mapColliders) {
            vStationary.push_back(&c);
        }

        std::vector<DestructibleBlock*> destructibleBlocks = m.GetDestructibleBlocks();
        for (DestructibleBlock* d : destructibleBlocks) {
            vDestructibleBlocks.push_back(d);
            vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next++, d));
        }




        Utility::PlayMusic(gGameMusic[gRndTiles]);

        bool running = true;
        int timeToExit = 0;

        std::uniform_int_distribution<int> containerSpawnDist(5000, 30000);
        int32_t containerSpawnTicks = containerSpawnDist(generator);
        const int maxContainers = 5;
        int containers = 0;

        /* Background texture */

        SDL_Surface* surf = SDL_CreateRGBSurface(0, 320, SCREEN_HEIGHT, 32, rmask, gmask, bmask, amask);
        SDL_FillRect(surf, NULL, backgroundColor[gRndTiles]);
        SDL_Texture* bgTex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);

        /* End of background texture */

        /* Scoreboard power-up textures */

        SDL_Texture* scoreboardBulletsTex = Utility::LoadTexture(ren, SCOREBOARD_BULLETS);
        SDL_Texture* scoreboardBounceTex = Utility::LoadTexture(ren, SCOREBOARD_BOUNCE);
        SDL_Texture* scoreboardSpeedTex = Utility::LoadTexture(ren, SCOREBOARD_SPEED);

        int sbBulletsW, sbBulletsH;
        int sbBounceW, sbBounceH;
        int sbSpeedW, sbSpeedH;

        SDL_QueryTexture(scoreboardBulletsTex, NULL, NULL, &sbBulletsW, &sbBulletsH);
        SDL_QueryTexture(scoreboardBounceTex, NULL, NULL, &sbBounceW, &sbBounceH);
        SDL_QueryTexture(scoreboardSpeedTex, NULL, NULL, &sbSpeedW, &sbSpeedH);

        /* End of scoreboard power-up textures */

        uint32_t ticksSincePause = 500;

        while (running == true) {
            uint32_t new_time = SDL_GetTicks();
            uint32_t frame_time = std::min(new_time - old_time, (uint32_t) 30);
            old_time = new_time;

            if (ticksSincePause > 0) {
                if (frame_time > ticksSincePause) {
                    ticksSincePause = 0;
                } else {
                    ticksSincePause -= frame_time;
                }
            }

            if (timeToExit > 0) {
                timeToExit -= frame_time;
                if (timeToExit <= 0)
                    running = false;
            } else {
                switch (gameOptions->GetMatchType()) {
                    case TIME_MATCH:
                        if (gameTime > frame_time) {
                            gameTime -= frame_time;
                        } else {
                            int highScore = INT_MIN;
                            for (int i = 0; i < 4; ++i) {
                                if (players[i].GetScore() >= highScore && playersIn[i]) {
                                    highScore = players[i].GetScore();
                                }
                            }
                            for (int i = 0; i < 4; ++i) {
                                if (players[i].GetScore() == highScore && playersIn[i]) {
                                    winningPlayer[i] = true;
                                }
                            }
                            timeToExit = GAME_END_TICKS;
                            Mix_FadeOutMusic(GAME_END_TICKS);
                        }
                        break;
                    case SCORE_MATCH:
                        for (int i = 0; i < 4; ++i) {
                            if (players[i].GetScore() >= gameOptions->GetScore() ) {
                                winningPlayer[i] = true;
                            }
                        }
                        for (int i = 0; i < 4; ++i) {
                            if (winningPlayer[i]) {
                                timeToExit = GAME_END_TICKS;
                                Mix_FadeOutMusic(GAME_END_TICKS);
                            }
                        }
                        break;
                    case STOCK_MATCH:
                        int playersAlive = 0;
                        for (int i = 0; i < 4; ++i) {
                            if (players[i].IsDead()) {
                                winningPlayer[i] = false;
                            } else if (playersIn[i]) {
                                playersAlive++;
                            }
                        }
                        if (playersAlive < 2) {
                            timeToExit = GAME_END_TICKS;
                            Mix_FadeOutMusic(GAME_END_TICKS);
                        }


                        break;
                }
            }

            if (running == false) {
                break;
            }



            gInput->CheckInput();

            if (gInput->QuitFlag()) {
                Quit(0);
            }
            if (gInput->EscapeHeld()) {
                if (ticksSincePause == 0) {
                    if (Pause() == -1) {
                        running = false;
                    } else {
                        ticksSincePause = 300;
                    }
                }
            }
            for (int i = 0; i < 4; ++i) {
                if (gInput->Player(i)) {
                    //if (gInput->Player(i)->BackHeld()) {
                    //    running = false;
                    //}
                    if (gInput->Player(i)->StartHeld() && ticksSincePause == 0) {
                        if (Pause() == -1) {
                            running = false;
                        } else {
                            ticksSincePause = 300;
                        }
                    }
                }
            }


            if (containerSpawnTicks > 0) {
                containerSpawnTicks -= frame_time;
            }

            if (containerSpawnTicks <= 0) {
                if (containers < maxContainers) {
                    std::uniform_int_distribution<int> cXdist(1,38);
                    std::uniform_int_distribution<int> cYdist(1,28);
                    int cX = cXdist(generator);
                    int cY = cYdist(generator);
                    if (m.GetTileAt(cY, cX) == 0x00 && m.GetTileAt(cY-1, cX) == 0x00 &&
                        m.GetTileAt(cY, cX-1) == 0x00 && m.GetTileAt(cY-1, cX-1) == 0x00) {
                        bool overlap = false;
                        for (Container* c1 : vContainers) {
                            if ( (cX * 8) >= (c1->GetX() - 16) && (cX * 8) <= (c1->GetX() + 16) &&
                                (cY * 8) >= (c1->GetY() - 16) && (cY * 8) <= (c1->GetY() + 16)) {
                                    overlap = true;
                                    break;
                                }
                        }
                        for (Player* c1 : vPlayers) {
                            if ( (cX * 8) >= (c1->GetX() - 16) && (cX * 8) <= (c1->GetX() + 16) &&
                                (cY * 8) >= (c1->GetY() - 16) && (cY * 8) <= (c1->GetY() + 16)) {
                                    overlap = true;
                                    break;
                                }
                        }
                        for (DestructibleBlock* c1 : vDestructibleBlocks) {
                            if ( (cX * 8) >= (c1->GetX() - 16) && (cX * 8) <= (c1->GetX() + 16) &&
                                (cY * 8) >= (c1->GetY() - 16) && (cY * 8) <= (c1->GetY() + 16)) {
                                    overlap = true;
                                    break;
                                }
                        }

                        if (!overlap) {
                            Container* c = new Container(cX * 8, cY * 8, ren);

                            vContainers.push_back(c);
                            vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next, c));
                            RenderableObject::next++;
                            containers++;
                            containerSpawnTicks = containerSpawnDist(generator) + containerSpawnTicks;
                        }
                    }
                }
            }


            /* Check for players holding fire button */
            for (int i = 0; i < 4; i++) {
                if (players[i].IsDead() ) {
                    continue;
                }
                if (gInput->Player(i) != nullptr) {
                    if (gInput->Player(i)->FireHeld()) {
                        players[i].FireIsHeld(true);
                    } else {
                        players[i].FireIsHeld(false);
                        players[i].FireIsReleased(true);
                    }
                }
                if (players[i].FireHeld() && players[i].FireReady() && players[i].FireReleased()) {
                    Bullet* b = players[i].Fire();
                        if (b != nullptr) {
                            //Utility::FireRumble(gHaptic[i]);
                            vBullets.insert(std::pair<int, Bullet*>(Bullet::next, b));
                            vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next++, b));
                        }
                }
            }

            /* Check for players moving the right stick */
            for (int i = 0; i < 4; i++) {
                if (players[i].IsDead() ) {
                    continue;
                }
                if (gInput->Player(i) != nullptr) {
                    if (gInput->Player(i)->RightStickVector().GetX() != 0 ||
                        gInput->Player(i)->RightStickVector().GetY() != 0) {
                        players[i].SetJoyTurret(true);
                    } else {
                        players[i].SetJoyTurret(false);
                        players[i].SetTurretRotationVel(0);
                    }
                }
                if (players[i].JoyTurret()) {

                    Vector2D joyVec(gInput->Player(i)->RightStickVector().Normalized());

                    float ptAngle = players[i].GetTurretAngle() - 90;
                    while (ptAngle > 360) {
                        ptAngle -= 360;
                    }
                    while (ptAngle < 0) {
                        ptAngle += 360;
                    }
                    if (ptAngle > 180) {
                        ptAngle -= 360;
                    }

                    if (ptAngle < -180) {
                        ptAngle += 360;
                    }

                    ptAngle = ptAngle * M_PI / 180;

                    if (ptAngle > M_PI) {
                        ptAngle = ptAngle - 2 * M_PI;
                    }
                    if (ptAngle < -M_PI) {
                        ptAngle = ptAngle + 2 * M_PI;
                    }

                    float ptX, ptY;
                    ptX = std::cos(ptAngle);
                    ptY = std::sin(ptAngle);

                    float scale;

                    double diff = (joyVec.GetX() * ptY) - (joyVec.GetY() * ptX);
                    scale = (diff > 0 ) - (diff < 0);

                    players[i].SetTurretRotationVel(MAX_ROTATE * scale * joyVec.GetMagnitude());
                }
            }

            /* Check for players moving the left stick */
            for (int i = 0; i < 4; i++) {
                if (players[i].IsDead() ) {
                    continue;
                }
                if (gInput->Player(i) != nullptr) {
                    if (gInput->Player(i)->LeftStickVector().GetMagnitude() != 0) {
                        players[i].SetJoyMove(true);
                    } else {
                        players[i].SetJoyMove(false);
                        players[i].SetRotationVel(0);
                        players[i].SetForwardVel(0);
                    }
                }

                if (players[i].JoyMove()) {
                    players[i].IsMoving(true);
                    Vector2D joyVec(gInput->Player(i)->LeftStickVector().Normalized());

                    float pAngle = players[i].GetAngle() - 90;
                    while (pAngle > 360) {
                        pAngle -= 360;
                    }
                    while (pAngle < 0) {
                        pAngle += 360;
                    }
                    if (pAngle > 180) {
                        pAngle -= 360;
                    }

                    if (pAngle < -180) {
                        pAngle += 360;
                    }

                    pAngle = pAngle * M_PI / 180;

                    if (pAngle > M_PI) {
                        pAngle = pAngle - 2 * M_PI;
                    }
                    if (pAngle < -M_PI) {
                        pAngle = pAngle + 2 * M_PI;
                    }

                    float pX, pY;
                    pX = std::cos(pAngle);
                    pY = std::sin(pAngle);

                    float scale;

                    double diff = (joyVec.GetX() * pY) - (joyVec.GetY() * pX);
                    scale = (diff > 0 ) - (diff < 0);

                    players[i].SetRotationVel(MAX_ROTATE * scale);
                    players[i].SetForwardVel(MAX_MOVE * joyVec.GetMagnitude());
                } else if (players[i].Moving()) {
                    players[i].IsMoving(false);
                }
            }

            for (Player* p : vPlayers) {
                if (p->IsDead()) {
                    continue;
                }
                for (Collider* c : vStationary) {
                    if (!c->GetOwner().IsDead() && !c->Passable()) {
                        p->CheckCollision(*c, frame_time);
                    }
                }
                for (Player* p_other : vPlayers) {
                    if (!p_other->IsDead())
                        p->CheckCollision(p_other->GetCollider(), frame_time);

                }
                for (Container* c : vContainers) {
                    if (!c->IsDead()) {
                        p->CheckCollision(c->GetCollider(), frame_time);
                    } else {
                        bool exists = false;
                            for (Container* c1 : vContainersDelete) {
                                if (c1 == c) {
                                    exists = true;
                                    break;
                                }
                            }
                            if (!exists)
                                vContainersDelete.push_back(c);
                    }
                }
                for (DestructibleBlock* d : vDestructibleBlocks) {
                    if (!d->IsDead()) {
                        p->CheckCollision(d->GetCollider(), frame_time);
                    } else {
                        bool exists = false;
                        for (DestructibleBlock* d1 : vDestructibleBlocksDelete) {
                            if (d == d1) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists)
                            vDestructibleBlocksDelete.push_back(d);
                    }
                }

                for (Powerup* pow : vPowerups) {
                    if (!pow->IsDead()) {
                        CollisionInfo coll = p->GetCollider().CheckCollision(pow->GetCollider(), p->GetVelocity());

                        if (coll.Colliding()) {
                            pow->Apply(*p);
                        }
                    } else {
                        bool exists = false;
                            for (Powerup* pow1 : vPowerupsDelete) {
                                if (pow1 == pow)
                                    exists = true;
                            }
                            if (!exists)
                                vPowerupsDelete.push_back(pow);
                    }

                }
            }

            for (GameObject* g : vPlayers) {
                g->Update(frame_time);
                if (g->GetX() > ARENA_WIDTH || g->GetX() < 0 || g->GetY() < 0 || g->GetY() > ARENA_HEIGHT) {
                    g->SetX(m.GetStartPos( ((Player*) g)->GetID()).GetX());
                    g->SetY(m.GetStartPos( ((Player*) g)->GetID()).GetY());
                }
            }

            for (GameObject* d : vDestructibleBlocks) {
                d->Update(frame_time);
            }

            for (std::pair<int, Bullet*> b : vBullets) {
                if (!b.second->IsDead()) {
                    b.second->Update(frame_time);

                    for (DestructibleBlock* d : vDestructibleBlocks) {
                        if (!d->IsDead()) {
                            CollisionInfo coll = b.second->CheckCollision(*d, frame_time);
                            if (coll.Colliding() || coll.WillCollide()) {
                                Utility::PlaySound(sfxBulletBrick);
                                if (d->Damage() == 1) {
                                    b.second->Die();
                                    b.second->GetOwner().DestroyBullet();
                                    if (d->GetContents() != -1) {
                                        Powerup* pow = new Powerup(d->GetX(), d->GetY(), ren, d->GetContents());
                                        vPowerups.push_back(pow);
                                        vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next++, pow));
                                    }
                                    RenderableObject::next++;
                                    NewExplosion(d->GetX(), d->GetY(), ren, vRenderable, vExplosions);

                                } else if (!d->Bounce() ) {
                                    b.second->Die();
                                    b.second->GetOwner().DestroyBullet();
                                    RenderableObject::next++;
                                    NewExplosion(b.second->GetX(), b.second->GetY(), ren, vRenderable, vExplosions);
                                } else if (b.second->IsDead() ) {
                                    b.second->Die();
                                    b.second->GetOwner().DestroyBullet();
                                    RenderableObject::next++;
                                    NewExplosion(b.second->GetX(), b.second->GetY(), ren, vRenderable, vExplosions);
                                }
                            }
                        }
                    }

                    for (Container* c : vContainers) {
                        if (!c->IsDead()) {
                            CollisionInfo coll = b.second->CheckCollision(*c, frame_time);
                            if (coll.Colliding()) {
                                c->Die();
                                b.second->Die();
                                b.second->GetOwner().DestroyBullet();
                                Powerup* pow = new Powerup(c->GetX(), c->GetY(), ren, c->GetContents());
                                vPowerups.push_back(pow);
                                vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next++, pow));
                                NewExplosion(c->GetX(), c->GetY(), ren, vRenderable, vExplosions);
                                containers--;
                            }
                        } else {
                            bool exists = false;
                            for (Container* c1 : vContainersDelete) {
                                if (c1 == c)
                                    exists = true;
                            }
                            if (!exists)
                                vContainersDelete.push_back(c);
                        }
                    }

                    for (Player* pl : vPlayers) {
                            if (pl->IsDead()) {
                                continue;
                            }
                            if (pl->IsInvincible()) {
                                continue;
                            }
                            if (pl->GetID() == b.second->GetOwner().GetID() && b.second->GetBounce() == 0) {
                                continue;
                            }
                            CollisionInfo coll = b.second->CheckCollision(*pl, frame_time);
                            if (coll.Colliding()) {
                                if (&(b.second->GetOwner()) == pl) {
                                    pl->AddScore(-1);
                                } else {
                                    b.second->GetOwner().AddScore(1);
                                }

                                NewExplosion(pl->GetX(), pl->GetY(), ren, vRenderable, vExplosions);

                                Utility::PlaySound(sfxDie);
                                //Utility::DieRumble(gHaptic[pl->GetID()]);
                                if (gameOptions->GetMatchType() == STOCK_MATCH && pl->GetLives() == 1) {
                                    pl->Die();
                                } else {
                                    pl->LoseLife();
                                    pl->SetX(m.GetStartPos(pl->GetID()).GetX());
                                    pl->SetY(m.GetStartPos(pl->GetID()).GetY());
                                    pl->Invincible();
                                }
                                b.second->GetOwner().DestroyBullet();
                                b.second->Die();


                            }
                    }

                    if (b.second->IsDead()) {
                        break;
                    }

                    for (std::pair<int, Bullet*> other : vBullets) {
                        if (b.second != other.second && !other.second->IsDead() && b.second->GetDirection() != other.second->GetDirection()) {
                            CollisionInfo coll = b.second->CheckCollision(*other.second, frame_time);
                            if ((coll.Colliding() || coll.WillCollide())) {
                                b.second->GetOwner().DestroyBullet();
                                other.second->GetOwner().DestroyBullet();
                                NewExplosion(b.second->GetX(), b.second->GetY(), ren, vRenderable, vExplosions);
                                b.second->Die();
                                other.second->Die();
                                break;
                            }
                        }

                    }

                    if (b.second->IsDead()) {
                        break;
                    }

                    for (Collider* c : vStationary) {
                        if (!c->StopsShots())
                            continue;
                        CollisionInfo coll = b.second->CheckCollision(*c, frame_time);
                        if (b.second->IsDead() && (coll.Colliding() || coll.WillCollide())) {
                            Utility::PlaySound(sfxBulletWall);
                            b.second->GetOwner().DestroyBullet();
                            NewExplosion(b.second->GetX(), b.second->GetY(), ren, vRenderable, vExplosions);
                            b.second->Die();
                            break;
                        }
                    }
                    if (b.second->IsDead()) {
                        break;
                    }
                }
                if (b.second->IsDead() ) {
                    vBulletsDelete.insert(b);
                }
            }

            for (Explosion* e : vExplosions) {
                if (!e->IsDead()) {
                    e->Update(frame_time);
                } else {
                    vExplosionsDelete.push_back(e);
                }
            }

            while (!vDestructibleBlocksDelete.empty()) {
                std::vector<DestructibleBlock*>::iterator it = vDestructibleBlocksDelete.begin();
                DestructibleBlock* d = *it;
                for (std::vector<DestructibleBlock*>::iterator dIt = vDestructibleBlocks.begin(); dIt != vDestructibleBlocks.end(); dIt++) {
                    if (d == *dIt) {
                        vDestructibleBlocks.erase(dIt);
                        break;
                    }
                }
                for (std::map<int, RenderableObject*>::iterator rIt = vRenderable.begin(); rIt != vRenderable.end(); rIt++) {
                    if (rIt->second == d) {
                        vRenderable.erase(rIt->first);
                        break;
                    }
                }
                vDestructibleBlocksDelete.erase(it);

                delete d;

            }

            while (!vExplosionsDelete.empty()) {
                std::vector<Explosion*>::iterator it = vExplosionsDelete.begin();
                Explosion* e = *it;
                for (std::vector<Explosion*>::iterator eIt = vExplosions.begin(); eIt != vExplosions.end(); eIt++) {
                    if (e == *eIt) {
                        vExplosions.erase(eIt);
                        break;
                    }
                }
                for (std::map<int, RenderableObject*>::iterator rIt = vRenderable.begin(); rIt != vRenderable.end(); rIt++) {
                    if (rIt->second == e) {
                        vRenderable.erase(rIt->first);
                        break;
                    }
                }
                vExplosionsDelete.erase(it);
                delete e;

            }

            while (!vContainersDelete.empty()) {
                std::vector<Container*>::iterator it = vContainersDelete.begin();
                Container* c = *it;
                for (std::vector<Container*>::iterator cIt = vContainers.begin(); cIt != vContainers.end(); cIt++) {
                    if (c == *cIt) {
                        vContainers.erase(cIt);
                        break;
                    }
                }
                for (std::map<int, RenderableObject*>::iterator rIt = vRenderable.begin(); rIt != vRenderable.end(); rIt++) {
                    if (rIt->second == c) {
                        vRenderable.erase(rIt->first);
                        delete c;
                        break;
                    }
                }
                vContainersDelete.erase(it);


            }

            while (!vPowerupsDelete.empty()) {
                std::vector<Powerup*>::iterator it = vPowerupsDelete.begin();
                Powerup* c = *it;
                for (std::vector<Powerup*>::iterator cIt = vPowerups.begin(); cIt != vPowerups.end(); cIt++) {
                    if (c == *cIt) {
                        vPowerups.erase(cIt);
                        break;
                    }
                }
                for (std::map<int, RenderableObject*>::iterator rIt = vRenderable.begin(); rIt != vRenderable.end(); rIt++) {
                    if (rIt->second == c) {
                        vRenderable.erase(rIt->first);
                        delete c;
                        break;
                    }
                }
                vPowerupsDelete.erase(it);


            }


            while (!vBulletsDelete.empty()) {
                std::map<int, Bullet*>::iterator it = vBulletsDelete.begin();
                Bullet* b = it->second;
                vBullets.erase(it->first);
                for (std::map<int, RenderableObject*>::iterator rIt = vRenderable.begin(); rIt != vRenderable.end(); rIt++) {
                    if (rIt->second == b) {
                        vRenderable.erase(rIt->first);
                        break;
                    }
                }
                vBulletsDelete.erase(it);
                delete b;

            }


            /* Render Loop */
            if (SDL_TICKS_PASSED(new_time - ticks, RENDER_INTERVAL)) {



                /* Clear render target */
                SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
                SDL_RenderClear(ren);

                /* Copy background texture */
                SDL_Rect bgDestRect;
                bgDestRect.x = 0;
                bgDestRect.y = 0;
                bgDestRect.w = 320;
                bgDestRect.h = 240;
                SDL_RenderCopy(ren, bgTex, NULL, &bgDestRect);

                /* Render all renderable objects */
                for (std::pair<int, RenderableObject*> r : vRenderable) {
                    if (!r.second->IsDead()) {
                        r.second->Render();
                    }
                }

                #ifdef _FPS_DEBUG
                /* FPS Texture */
                SDL_Color c = {255, 255, 255, 255};
                uint32_t fps = 1000 / (new_time + (SDL_GetTicks() - new_time) - ticks);
                //if (frame_time > 0)
                //    fps = 1000 / frame_time;
                std::stringstream strFPS;
                strFPS << "FPS: " << fps;

                SDL_Texture* fps_tex = Utility::RenderText(strFPS.str(), GAME_FONT, c, 10, ren);

                int fps_w, fps_h;

                SDL_QueryTexture(fps_tex, NULL, NULL, &fps_w, &fps_h);
                SDL_Rect srcRect, dstRect;
                srcRect.x = 0;
                srcRect.y = 0;
                srcRect.h = fps_h;
                srcRect.w = fps_w;

                dstRect.x = 8;
                dstRect.y = 8;
                dstRect.h = fps_h;
                dstRect.w = fps_w;

                SDL_RenderCopy(ren, fps_tex, &srcRect, &dstRect);

                /* End of FPS texture */
                #endif // _FPS_DEBUG

                /* HUD */
                int verticalOffset = 0;
                {
                    SDL_Color c = { 0xFF, 0xFF, 0xFF, 0xFF };
                    {
                        SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);

                        std::stringstream gtypeString, gvalueString;
                        gtypeString << "Match: ";
                        switch (gameOptions->GetMatchType()) {
                            case SCORE_MATCH:
                                gtypeString << "Score";
                                gvalueString << "Max score: " << gameOptions->GetScore();
                                break;
                            case STOCK_MATCH:
                                gtypeString << "Stock";
                                gvalueString << "Max lives: " << gameOptions->GetStock();
                                break;
                            case TIME_MATCH:
                                gtypeString << "Time";
                                gvalueString << "Time: " << gameTime / 1000;
                                break;
                        }

                        SDL_Texture* gtypeTexture = Utility::RenderText(gtypeString.str(), GAME_FONT, c, 12, ren);
                        SDL_Texture* gvalueTexture = Utility::RenderText(gvalueString.str(), GAME_FONT, c, 12, ren);

                        int gtypeW, gtypeH, gvalueW, gvalueH;

                        SDL_QueryTexture(gtypeTexture, NULL, NULL, &gtypeW, &gtypeH);
                        SDL_QueryTexture(gvalueTexture, NULL, NULL, &gvalueW, &gvalueH);

                        SDL_Rect frameRect, typeRect, valueRect;

                        frameRect.x = 320;
                        frameRect.y = 0;
                        frameRect.h = verticalOffset = gtypeH + gvalueH + 2 + 2 + 2;
                        frameRect.w = 106;

                        typeRect.x = 320 + 2;
                        typeRect.y = 0 + 2;
                        typeRect.w = gtypeW;
                        typeRect.h = gtypeH;

                        valueRect.x = 320 + 2;
                        valueRect.y = 2 + gtypeH + 2;
                        valueRect.w = gvalueW;
                        valueRect.h = gvalueH;

                        SDL_RenderDrawRect(ren, &frameRect);
                        SDL_RenderCopy(ren, gtypeTexture, NULL, &typeRect);
                        SDL_RenderCopy(ren, gvalueTexture, NULL, &valueRect);

                        SDL_DestroyTexture(gtypeTexture);
                        SDL_DestroyTexture(gvalueTexture);
                    }
                    SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
                    for (int i = 0; i < 4; ++i) {
                        if (!playersIn[i])
                            continue;
                        int rWidth, rHeight;
                        rWidth = 106;
                        rHeight = (240 - verticalOffset) / 4;

                        SDL_Rect srcRect, dstRect;
                        dstRect.x = 320;
                        dstRect.y = verticalOffset + i * rHeight;
                        dstRect.w = rWidth;
                        dstRect.h = rHeight;

                        SDL_RenderDrawRect(ren, &dstRect);



                        std::stringstream scoreString;

                        scoreString << "Player " << i + 1;
                        SDL_Texture* plTex = Utility::RenderText(scoreString.str(), GAME_FONT, c, 12, ren);
                        int plWidth, plHeight;
                        SDL_QueryTexture(plTex, NULL, NULL, &plWidth, &plHeight);
                        scoreString.str("");

                        switch (gameOptions->GetMatchType() ) {
                            case STOCK_MATCH:
                                scoreString << "Lives: " << players[i].GetLives();
                                break;
                            case SCORE_MATCH:
                            case TIME_MATCH:
                                scoreString << "Score: " << players[i].GetScore();
                                break;
                        }
                        int scoreWidth, scoreHeight;
                        SDL_Texture* scoreTex = Utility::RenderText(scoreString.str(), GAME_FONT, c, 12, ren);
                        SDL_QueryTexture(scoreTex, NULL, NULL, &scoreWidth, &scoreHeight);
                        scoreString.str("");

                        scoreString << "- " << players[i].GetMaxBullets();
                        SDL_Texture* bulletsTex = Utility::RenderText(scoreString.str(), GAME_FONT, c, 12, ren);
                        int bulletsWidth, bulletsHeight;
                        SDL_QueryTexture(bulletsTex, NULL, NULL, &bulletsWidth, &bulletsHeight);
                        scoreString.str("");

                        scoreString << "- " << players[i].GetMaxBounce();
                        SDL_Texture* bounceTex = Utility::RenderText(scoreString.str(), GAME_FONT, c, 12, ren);
                        int bounceWidth, bounceHeight;
                        SDL_QueryTexture(bounceTex, NULL, NULL, &bounceWidth, &bounceHeight);
                        scoreString.str("");

                        scoreString << "- " << players[i].GetMaxSpeed();
                        SDL_Texture* speedTex = Utility::RenderText(scoreString.str(), GAME_FONT, c, 12, ren);
                        int speedWidth, speedHeight;
                        SDL_QueryTexture(speedTex, NULL, NULL, &speedWidth, &speedHeight);
                        scoreString.str("");

                        int horizOffset = 320 + 2;

                        srcRect.x = 0;
                        srcRect.y = 0;
                        srcRect.w = plWidth;
                        srcRect.h = plHeight;

                        dstRect.x = 320 + 2;
                        dstRect.y = verticalOffset + (i * rHeight + 2);
                        dstRect.w = plWidth;
                        dstRect.h = plHeight;

                        SDL_RenderCopy(ren, plTex, &srcRect, &dstRect);

                        srcRect.x = 0;
                        srcRect.y = 0;
                        srcRect.w = scoreWidth;
                        srcRect.h = scoreHeight;

                        dstRect.x = 320 + 2;
                        dstRect.y = verticalOffset + (i * rHeight + plHeight + 4);
                        dstRect.w = scoreWidth;
                        dstRect.h = scoreHeight;

                        SDL_RenderCopy(ren, scoreTex, &srcRect, &dstRect);

                        /* Bullets powerup stats */

                        SDL_Rect bulletsRect;

                        bulletsRect.x = horizOffset;
                        bulletsRect.y = verticalOffset + (i * rHeight + plHeight + scoreHeight + 6);
                        bulletsRect.h = sbBulletsH;
                        bulletsRect.w = sbBulletsW;

                        SDL_RenderCopy(ren, scoreboardBulletsTex, NULL, &bulletsRect);

                        horizOffset += sbBulletsW + 2;

                        srcRect.x = 0;
                        srcRect.y = 0;
                        srcRect.w = bulletsWidth;
                        srcRect.h = bulletsHeight;

                        dstRect.x = horizOffset;
                        dstRect.y = verticalOffset + (i * rHeight + plHeight + scoreHeight + 6);
                        dstRect.w = bulletsWidth;
                        dstRect.h = bulletsHeight;

                        horizOffset += bulletsWidth + 4;

                        SDL_RenderCopy(ren, bulletsTex, &srcRect, &dstRect);

                        /* Bounce power-up stats */

                        SDL_Rect bounceRect;

                        bounceRect.x = horizOffset;
                        bounceRect.y = verticalOffset + (i * rHeight + plHeight + scoreHeight + 6);
                        bounceRect.h = sbBounceH;
                        bounceRect.w = sbBounceW;

                        SDL_RenderCopy(ren, scoreboardBounceTex, NULL, &bounceRect);

                        horizOffset += sbBounceW + 2;

                        srcRect.x = 0;
                        srcRect.y = 0;
                        srcRect.w = bounceWidth;
                        srcRect.h = bounceHeight;

                        dstRect.x = horizOffset;
                        dstRect.y = verticalOffset + (i * rHeight + plHeight + scoreHeight + 6);
                        dstRect.w = bounceWidth;
                        dstRect.h = bounceHeight;

                        horizOffset += bounceWidth + 4;

                        SDL_RenderCopy(ren, bounceTex, &srcRect, &dstRect);

                        /* Speed power-up stats */

                        SDL_Rect speedRect;

                        speedRect.x = horizOffset;
                        speedRect.y = verticalOffset + (i * rHeight + plHeight + scoreHeight + 6);
                        speedRect.h = sbSpeedH;
                        speedRect.w = sbSpeedW;

                        SDL_RenderCopy(ren, scoreboardSpeedTex, NULL, &speedRect);

                        horizOffset += sbSpeedW + 2;

                        srcRect.x = 0;
                        srcRect.y = 0;
                        srcRect.w = speedWidth;
                        srcRect.h = speedHeight;

                        dstRect.x = horizOffset;
                        dstRect.y = verticalOffset + (i * rHeight + plHeight + scoreHeight + 6);
                        dstRect.w = speedWidth;
                        dstRect.h = speedHeight;

                        SDL_RenderCopy(ren, speedTex, &srcRect, &dstRect);

                        SDL_DestroyTexture(plTex);
                        SDL_DestroyTexture(scoreTex);
                        SDL_DestroyTexture(bulletsTex);
                        SDL_DestroyTexture(bounceTex);
                        SDL_DestroyTexture(speedTex);



                    }
                }
                /* End of HUD */


                SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
                SDL_RenderPresent(ren);
                ticks = SDL_GetTicks();
            }
            /* End of Render Loop */


        }

        /* Delete scoreboard texture */
        SDL_DestroyTexture(scoreboardBulletsTex);
        SDL_DestroyTexture(scoreboardBounceTex);
        SDL_DestroyTexture(scoreboardSpeedTex);

        /* Delete background texture */
        SDL_DestroyTexture(bgTex);

        delete gameOptions;

        if (loopGame) {
            Mix_FadeOutMusic(200);
            if (WinScreen(winningPlayer, players) == -1) {
                loopGame = false;
            }
        } else {
            Mix_HaltMusic();
        }
    }

    Quit(0);

    return 0;
}


void NewExplosion(const float x, const float y, SDL_Renderer* ren, std::map<int, RenderableObject*>& vRenderable, std::vector<Explosion*>& vExplosions) {
    Explosion* newExpl = new Explosion(x, y, ren);
    std::pair<int, RenderableObject*> newPair(RenderableObject::next, newExpl);
    RenderableObject::next++;
    vRenderable.insert(newPair);
    vExplosions.push_back(newExpl);
}


int Menu() {
    bool menuRunning = true;

    #ifdef _DEMO
    playersIn[0] = false;
    playersIn[1] = false;
    playersIn[2] = false;
    playersIn[3] = false;
    #endif // _DEMO
    std::vector<std::string> levelFiles;

    dirent* de = NULL;
    DIR* dp = NULL;

    std::string mapsPath = basePath + MAPS_PATH;

    dp = opendir(mapsPath.c_str());
    if (dp) {
        while (true) {
            errno = 0;

            de = readdir(dp);
            if (de == NULL)
                break;
            std::string filename = de->d_name;
            if (filename.at(filename.length() - 1) == 'd' && filename.at(filename.length() - 2) == '.') {
                std::string fullPath = mapsPath + filename;
                std::ifstream inFile(fullPath);
                if (inFile.good() ) {
                    char c;
                    char DE = 0xDE;
                    char AD = 0xAD;
                    char BE = 0xBE;
                    char EF = 0xEF;

                    inFile.get(c);
                    if (c != DE)
                        continue;

                    inFile.get(c);
                    if (c != AD)
                        continue;

                    inFile.get(c);
                    if (c != BE)
                        continue;

                    inFile.get(c);
                    if (c != EF)
                        continue;

                    levelFiles.push_back(filename);

                    inFile.close();
                }

            }

        }
    }

    free(dp);
    if (de)
        free(de);

    const int cursorRepeat = MENU_REPEAT_VERT_TICKS;
    int ticksSinceMove[4] = {200, 200, 200, 200};
    bool mapSelect = false;

    int mapCount = levelFiles.size();
    int mapSelected = 0;

    uint32_t time = SDL_GetTicks();

    uint32_t ticksSinceStart = 1000;
    uint32_t ticksSincePause = 1000;

    int topLevel = 0;
    int bottomLevel = 17;
    bool quit = false;
    while (menuRunning) {

        /*
        if (SDL_NumJoysticks() > 0 && SDL_NumJoysticks() != numJoysticks) {
            numJoysticks = SDL_NumJoysticks();
            for (int i = 0; i < 4; ++i) {
                playersIn[i] = false;
            }
            CheckJoysticks();
        }
        */
        for (int i = 0; i < 4; ++i) {
            if (gInput->Player(i) == nullptr) {
                playersIn[i] = false;
            }
        }

        uint32_t frameTime = SDL_GetTicks() - time;
        time = SDL_GetTicks();

        if (ticksSinceStart > 0) {
            if (frameTime > ticksSinceStart)
                ticksSinceStart = 0;
            else
                ticksSinceStart -= frameTime;
        }

        if (ticksSincePause > 0) {
            if (frameTime > ticksSincePause)
                ticksSincePause = 0;
            else
                ticksSincePause -= frameTime;
        }

        gInput->CheckInput();

        if (gInput->QuitFlag()) {
            menuRunning = false;
            quit = true;
        }

        if (gInput->EscapeHeld() && ticksSinceStart == 0 && ticksSincePause == 0) {
            if (ConfirmQuit() == -1) {
                menuRunning = false;
                quit = true;
                break;
            } else {
                ticksSincePause = 2000;
            }
        }

        for (int i = 0; i < 4; ++i) {
            if (gInput->Player(i) == nullptr)
                continue;

            if (gInput->Player(i)->SelectHeld() ) {
                if (!playersIn[i]) {
                    playersIn[i] = true;
                    Utility::PlaySound(sfxReady);
                }
            } else if (gInput->Player(i)->CancelHeld() ) {
                if (playersIn[i]) {
                    playersIn[i] = false;
                    Utility::PlaySound(sfxNotReady);
                }
            }
            if (ticksSinceMove[i] > 0) {
                ticksSinceMove[i] = std::max((uint32_t) 0, ticksSinceMove[i] - frameTime);
            } else  {
                if (gInput->Player(i)->StartHeld() && ticksSinceStart == 0) {
                    mapSelect = true;
                }
                if (gInput->Player(i)->BackHeld() && ticksSinceStart == 0 && ticksSincePause == 0) {
                    if (ConfirmQuit() == -1) {
                        menuRunning = false;
                        quit = true;
                    } else {
                        ticksSincePause = 2000;
            }
                }
                if (gInput->Player(i)->UpHeld()) {
                    if (mapSelected > 0) {
                            mapSelected--;
                            if (mapSelected < topLevel) {
                                topLevel--;
                                bottomLevel--;
                            }

                            ticksSinceMove[i] = cursorRepeat;
                            Utility::PlaySound(sfxMenu);
                    }
                } else if (gInput->Player(i)->DownHeld()) {
                    if (mapSelected < mapCount - 1) {
                            mapSelected++;
                            if (mapSelected > bottomLevel) {
                                bottomLevel++;
                                topLevel++;
                            }
                            ticksSinceMove[i] = cursorRepeat;
                            Utility::PlaySound(sfxMenu);
                    }
                }
            }
        }

        if (gInput->EnterHeld() && ticksSinceStart == 0) {
            mapSelect = true;
        }

        int tempPlayersIn = 0;
        for (int i = 0; i < 4; ++i) {
            if (playersIn[i]) {
                tempPlayersIn++;
            }
        }
        playersInCount = tempPlayersIn;

        #ifdef _DEBUG_BUILD
        if (mapSelect && playersInCount >= 0) {
        #else
        if (mapSelect && playersInCount >= 1) {
        #endif
            mapfilename = levelFiles[mapSelected];
            return 0;
        } else {
            mapSelect = false;
        }

        SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);

        SDL_Rect frameRect;

        SDL_Color white = {0xFF, 0xFF, 0xFF, 0xFF};

        if (gInput->Player(0) != nullptr) {
            SDL_Texture* p1Text = Utility::RenderText("Player 1", GAME_FONT, white, 12, ren);
            SDL_Texture* p1Button = NULL;

            if (playersIn[0] == false) {
                if (gInput->Player(0)->ControllerType() == PS4_CONTROLLER) {
                    p1Button = Utility::RenderText("Press X", GAME_FONT, white, 10, ren);
                } else {
                    p1Button = Utility::RenderText("Press A", GAME_FONT, white, 10, ren);
                }

            } else {
                p1Button = Utility::RenderText("Ready!", GAME_FONT, white, 10, ren);
            }
            int p1TextWidth, p1TextHeight;
            SDL_QueryTexture(p1Text, NULL, NULL, &p1TextWidth, &p1TextHeight);
            int p1ButtonWidth, p1ButtonHeight;
            SDL_QueryTexture(p1Button, NULL, NULL, &p1ButtonWidth, &p1ButtonHeight);

            SDL_Rect p1DestRect;

            p1DestRect.x = 320 / 4 - (p1TextWidth / 2);
            p1DestRect.y = 240 / 4 - (p1TextHeight) - 2;
            p1DestRect.w = p1TextWidth;
            p1DestRect.h = p1TextHeight;
            SDL_RenderCopy(ren, p1Text, NULL, &p1DestRect);

            p1DestRect.x = 320 / 4 - (p1ButtonWidth / 2);
            p1DestRect.y = 240 / 4 + 2;
            p1DestRect.w = p1ButtonWidth;
            p1DestRect.h = p1ButtonHeight;
            SDL_RenderCopy(ren, p1Button, NULL, &p1DestRect);

            frameRect.x = 0;
            frameRect.y = 0;
            frameRect.h = SCREEN_HEIGHT / 2;
            frameRect.w = 320 / 2;


            SDL_RenderDrawRect(ren, &frameRect);

            SDL_DestroyTexture(p1Text);
            SDL_DestroyTexture(p1Button);
        }

        if (gInput->Player(1) != nullptr) {

            SDL_Texture* p2Text = Utility::RenderText("Player 2", GAME_FONT, white, 12, ren);
            SDL_Texture* p2Button = NULL;

            if (playersIn[1] == false) {
                if (gInput->Player(1)->ControllerType() == PS4_CONTROLLER) {
                    p2Button = Utility::RenderText("Press X", GAME_FONT, white, 10, ren);
                } else {
                    p2Button = Utility::RenderText("Press A", GAME_FONT, white, 10, ren);
                }
            } else {
                p2Button = Utility::RenderText("Ready!", GAME_FONT, white, 10, ren);
            }
            int p2TextWidth, p2TextHeight;
            SDL_QueryTexture(p2Text, NULL, NULL, &p2TextWidth, &p2TextHeight);
            int p2ButtonWidth, p2ButtonHeight;
            SDL_QueryTexture(p2Button, NULL, NULL, &p2ButtonWidth, &p2ButtonHeight);

            SDL_Rect p2DestRect;

            p2DestRect.x = 3 * 320 / 4 - (p2TextWidth / 2);
            p2DestRect.y = 240 / 4 - (p2TextHeight) - 2;
            p2DestRect.w = p2TextWidth;
            p2DestRect.h = p2TextHeight;
            SDL_RenderCopy(ren, p2Text, NULL, &p2DestRect);

            p2DestRect.x = 3 * 320 / 4 - (p2ButtonWidth / 2);
            p2DestRect.y = 240 / 4 + 2;
            p2DestRect.w = p2ButtonWidth;
            p2DestRect.h = p2ButtonHeight;
            SDL_RenderCopy(ren, p2Button, NULL, &p2DestRect);

            frameRect.x = 320 / 2;
            frameRect.y = 0;
            frameRect.h = SCREEN_HEIGHT / 2;
            frameRect.w = 320 / 2;
            SDL_RenderDrawRect(ren, &frameRect);

            SDL_DestroyTexture(p2Text);
            SDL_DestroyTexture(p2Button);
        }

        if (gInput->Player(2) != nullptr) {

            SDL_Texture* p3Text = Utility::RenderText("Player 3", GAME_FONT, white, 12, ren);
            SDL_Texture* p3Button = NULL;

            if (playersIn[2] == false) {
                if (gInput->Player(2)->ControllerType() == PS4_CONTROLLER) {
                    p3Button = Utility::RenderText("Press X", GAME_FONT, white, 10, ren);
                } else {
                    p3Button = Utility::RenderText("Press A", GAME_FONT, white, 10, ren);
                }
            } else {
                p3Button = Utility::RenderText("Ready!", GAME_FONT, white, 10, ren);
            }
            int p3TextWidth, p3TextHeight;
            SDL_QueryTexture(p3Text, NULL, NULL, &p3TextWidth, &p3TextHeight);
            int p3ButtonWidth, p3ButtonHeight;
            SDL_QueryTexture(p3Button, NULL, NULL, &p3ButtonWidth, &p3ButtonHeight);

            SDL_Rect p3DestRect;

            p3DestRect.x = 320 / 4 - (p3TextWidth / 2);
            p3DestRect.y = 3 * 240 / 4 - (p3TextHeight) - 2;
            p3DestRect.w = p3TextWidth;
            p3DestRect.h = p3TextHeight;
            SDL_RenderCopy(ren, p3Text, NULL, &p3DestRect);

            p3DestRect.x = 320 / 4 - (p3ButtonWidth / 2);
            p3DestRect.y = 3 * 240 / 4 + 2;
            p3DestRect.w = p3ButtonWidth;
            p3DestRect.h = p3ButtonHeight;
            SDL_RenderCopy(ren, p3Button, NULL, &p3DestRect);

            frameRect.x = 0;
            frameRect.y = 240 / 2;
            frameRect.h = SCREEN_HEIGHT / 2;
            frameRect.w = 320 / 2;
            SDL_RenderDrawRect(ren, &frameRect);

            SDL_DestroyTexture(p3Text);
            SDL_DestroyTexture(p3Button);
        }

        if (gInput->Player(3) != nullptr) {

            SDL_Texture* p4Text = Utility::RenderText("Player 4", GAME_FONT, white, 12, ren);
            SDL_Texture* p4Button = NULL;

            if (playersIn[3] == false) {
                if (gInput->Player(3)->ControllerType() == PS4_CONTROLLER) {
                    p4Button = Utility::RenderText("Press X", GAME_FONT, white, 10, ren);
                } else {
                    p4Button = Utility::RenderText("Press A", GAME_FONT, white, 10, ren);
                }
            } else {
                p4Button = Utility::RenderText("Ready!", GAME_FONT, white, 10, ren);
            }
            int p4TextWidth, p4TextHeight;
            SDL_QueryTexture(p4Text, NULL, NULL, &p4TextWidth, &p4TextHeight);
            int p4ButtonWidth, p4ButtonHeight;
            SDL_QueryTexture(p4Button, NULL, NULL, &p4ButtonWidth, &p4ButtonHeight);

            SDL_Rect p4DestRect;

            p4DestRect.x = 3 * 320 / 4 - (p4TextWidth / 2);
            p4DestRect.y = 3 * 240 / 4 - (p4TextHeight) - 2;
            p4DestRect.w = p4TextWidth;
            p4DestRect.h = p4TextHeight;
            SDL_RenderCopy(ren, p4Text, NULL, &p4DestRect);
            SDL_DestroyTexture(p4Text);

            p4DestRect.x = 3 * 320 / 4 - (p4ButtonWidth / 2);
            p4DestRect.y = 3 * 240 / 4 + 2;
            p4DestRect.w = p4ButtonWidth;
            p4DestRect.h = p4ButtonHeight;
            SDL_RenderCopy(ren, p4Button, NULL, &p4DestRect);
            SDL_DestroyTexture(p4Button);

            frameRect.x = 320 / 2;
            frameRect.y = 240 / 2;
            frameRect.h = SCREEN_HEIGHT / 2;
            frameRect.w = 320 / 2;
            SDL_RenderDrawRect(ren, &frameRect);



        }


        // Level Select Frame
        frameRect.x = 320;
        frameRect.y = 0;
        frameRect.h = 240;
        frameRect.w = 106;

        {
            SDL_Color grey = { 0x60, 0x60, 0x60, 0xFF };
            int i = 0;
            int j = 0;
            for (std::string filename : levelFiles) {
                if (j < topLevel || j > bottomLevel) {
                    j++;
                    continue;
                }

                filename.resize(filename.size() - 2);
                if (filename.size() > 8) {
                    filename.resize(8);
                }


                SDL_Texture* fileTex = NULL;

                if (mapSelected == j) {
                    fileTex = Utility::RenderText(filename, GAME_FONT, white, 10, ren);
                } else {
                    fileTex = Utility::RenderText(filename, GAME_FONT, grey, 10, ren);
                }

                int fileHeight, fileWidth;
                SDL_QueryTexture(fileTex, NULL, NULL, &fileWidth, &fileHeight);

                SDL_Rect fileRect;
                fileRect.x = 320 + 2;
                fileRect.y = i * fileHeight + i * 2;
                fileRect.w = fileWidth;
                fileRect.h = fileHeight;

                SDL_RenderCopy(ren, fileTex, NULL, &fileRect);
                i++;
                j++;
                SDL_DestroyTexture(fileTex);
            }
        }
        SDL_RenderDrawRect(ren, &frameRect);



        SDL_RenderPresent(ren);
    }
    if (quit)
        return -1;
    else {
        Utility::PlaySound(sfxMenuConfirm);
        return 0;
    }
}


Options* OptionsMenu() {
    bool optionsMenuRunning = true;

    int gameType = SCORE_MATCH;

    int score = std::min(5, MAX_SCORE);
    int time = std::min(120, MAX_TIME);
    int stock = std::min(3, MAX_STOCK);

    const int cursorRepeatV = MENU_REPEAT_VERT_TICKS;
    const int cursorRepeatH = MENU_REPEAT_HORIZ_TICKS;
    int ticksSinceMove[4] = {200, 200, 200, 200};

    uint32_t nowTime, renderTime;
    int ticksSinceStart = 1000;

    renderTime = nowTime = SDL_GetTicks();

    while (optionsMenuRunning) {

        uint32_t frameTime = SDL_GetTicks() - nowTime;
        nowTime = SDL_GetTicks();
        ticksSinceStart = std::max(0, ticksSinceStart - (int) frameTime);

        gInput->CheckInput();

        if (gInput->QuitFlag() ) {
            return nullptr;
        }

        if (gInput->EscapeHeld() && ticksSinceStart == 0 ) {
            return new Options(gameType, score, stock, time, true);
        }

        if (gInput->EnterHeld() && ticksSinceStart == 0) {
            optionsMenuRunning = false;
        }

        for (int i = 0; i < 4; ++i) {
            if (gInput->Player(i) == nullptr)
                continue;
            if ((gInput->Player(i)->BackHeld() || gInput->Player(i)->CancelHeld()) && ticksSinceStart == 0) {
                return new Options(gameType, score, stock, time, true);
            }
            if ((gInput->Player(i)->SelectHeld() || gInput->Player(i)->StartHeld()) && ticksSinceStart == 0) {
                optionsMenuRunning = false;
            }
        }

        for (int i = 0; i < 4; ++i) {
            if (gInput->Player(i) == nullptr) {
                continue;
            }

            if (ticksSinceMove[i] > 0) {
                ticksSinceMove[i] = std::max((uint32_t) 0, ticksSinceMove[i] - frameTime);
            } else  {
                if (gInput->Player(i)->UpHeld()) {
                    if (gameType > STOCK_MATCH)
                            gameType--;
                    ticksSinceMove[i] = cursorRepeatV;
                    Utility::PlaySound(sfxMenu);
                } else if (gInput->Player(i)->DownHeld()) {
                    if (gameType < TIME_MATCH)
                            gameType++;
                    ticksSinceMove[i] = cursorRepeatV;
                    Utility::PlaySound(sfxMenu);
                } else if (gInput->Player(i)->LeftHeld()) {
                    switch (gameType) {
                        case SCORE_MATCH:
                            if (score > MIN_SCORE)
                                score--;
                            break;
                        case STOCK_MATCH:
                            if (stock > MIN_STOCK)
                                stock--;
                            break;
                        case TIME_MATCH:
                            if (time > MIN_TIME)
                                time--;
                            break;
                    } // switch gameType
                    ticksSinceMove[i] = cursorRepeatH;
                    Utility::PlaySound(sfxMenu);
                } else if (gInput->Player(i)->RightHeld()) {
                    switch (gameType) {
                            case SCORE_MATCH:
                                if (score < MAX_SCORE)
                                    score++;
                                break;
                            case STOCK_MATCH:
                                if (stock < MAX_STOCK)
                                    stock++;
                                break;
                            case TIME_MATCH:
                                if (time < MAX_TIME)
                                    time++;
                                break;
                        } // switch gameType
                    ticksSinceMove[i] = cursorRepeatH;
                    Utility::PlaySound(sfxMenu);
                }
            }
        }

        if (SDL_TICKS_PASSED(nowTime - renderTime, RENDER_INTERVAL) ) {
            SDL_Texture* gameTypeTexture = nullptr;
            int gameTypeWidth, gameTypeHeight;

            SDL_Texture* gameValueTexture = nullptr;
            int gameValueWidth, gameValueHeight;

            SDL_Texture* arrowTexture = Utility::LoadTexture(ren, "Arrows.png");
            int arrowWidth, arrowHeight;
            SDL_QueryTexture(arrowTexture, NULL, NULL, &arrowWidth, &arrowHeight);

            std::string gameTypeString = "Game Type: ";
            std::stringstream gameValueString;

            switch (gameType) {
                case SCORE_MATCH:
                    gameTypeString += "Score";
                    gameValueString << "Score: " << score;
                    break;
                case STOCK_MATCH:
                    gameTypeString += "Stock";
                    gameValueString << "Stock: " << stock;
                    break;
                case TIME_MATCH:
                    gameTypeString += "Time";
                    gameValueString << "Time: " << time;
                    break;
            } // switch gameType

            SDL_Color white = { 0xFF, 0xFF, 0xFF, 0xFF };

            gameTypeTexture = Utility::RenderText(gameTypeString, GAME_FONT, white, 12, ren);
            gameValueTexture = Utility::RenderText(gameValueString.str(), GAME_FONT, white, 12, ren);

            SDL_QueryTexture(gameTypeTexture, NULL, NULL, &gameTypeWidth, &gameTypeHeight);
            SDL_QueryTexture(gameValueTexture, NULL, NULL, &gameValueWidth, &gameValueHeight);

            SDL_Rect gameTypeRect, gameValueRect;

            SDL_Rect lArrowSrcRect, rArrowSrcRect;
            SDL_Rect uArrowSrcRect, dArrowSrcRect;

            lArrowSrcRect.x = 0;
            lArrowSrcRect.y = 0;
            lArrowSrcRect.h = arrowHeight;
            lArrowSrcRect.w = arrowWidth / 4;

            rArrowSrcRect.x = 8;
            rArrowSrcRect.y = 0;
            rArrowSrcRect.h = arrowHeight;
            rArrowSrcRect.w = arrowWidth / 4;

            uArrowSrcRect.x = 16;
            uArrowSrcRect.y = 0;
            uArrowSrcRect.h = arrowHeight;
            uArrowSrcRect.w = arrowWidth / 4;

            dArrowSrcRect.x = 24;
            dArrowSrcRect.y = 0;
            dArrowSrcRect.h = arrowHeight;
            dArrowSrcRect.w = arrowWidth / 4;


            SDL_Rect lArrowDestRect, rArrowDestRect;
            SDL_Rect uArrowDestRect, dArrowDestRect;



            gameTypeRect.x = (320 / 2) - (gameTypeWidth / 2);
            gameTypeRect.y = (240 / 2) - (gameTypeHeight / 2) - 2;
            gameTypeRect.w = gameTypeWidth;
            gameTypeRect.h = gameTypeHeight;

            gameValueRect.x = (320 / 2) - (gameValueWidth / 2);
            gameValueRect.y = (240 / 2) + (gameValueHeight / 2) + 2;
            gameValueRect.w = gameValueWidth;
            gameValueRect.h = gameValueHeight;

            uArrowDestRect.x = (320 / 2) - (arrowWidth / 4 / 2);
            uArrowDestRect.y = (240 / 2) - (gameTypeHeight / 2) - 2 - (arrowHeight);
            uArrowDestRect.h = arrowHeight;
            uArrowDestRect.w = arrowWidth / 4;

            dArrowDestRect.x = (320 / 2) - (arrowWidth / 4 / 2);
            dArrowDestRect.y = (240 / 2) + (gameTypeHeight / 2 ) + (gameValueHeight) + 1;
            dArrowDestRect.h = arrowHeight;
            dArrowDestRect.w = arrowWidth / 4;

            lArrowDestRect.x = gameValueRect.x - 2 - (arrowWidth / 4);
            lArrowDestRect.y = gameValueRect.y + 2;
            lArrowDestRect.h  = arrowHeight;
            lArrowDestRect.w = arrowWidth / 4;

            rArrowDestRect.x = gameValueRect.x + gameValueWidth + 2;
            rArrowDestRect.y = gameValueRect.y + 2;
            rArrowDestRect.h  = arrowHeight;
            rArrowDestRect.w = arrowWidth / 4;

            SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
            SDL_RenderClear(ren);



            SDL_RenderCopy(ren, gameTypeTexture, NULL, &gameTypeRect);
            SDL_RenderCopy(ren, gameValueTexture, NULL, &gameValueRect);
            SDL_RenderCopy(ren, arrowTexture, &uArrowSrcRect, &uArrowDestRect);
            SDL_RenderCopy(ren, arrowTexture, &dArrowSrcRect, &dArrowDestRect);
            SDL_RenderCopy(ren, arrowTexture, &lArrowSrcRect, &lArrowDestRect);
            SDL_RenderCopy(ren, arrowTexture, &rArrowSrcRect, &rArrowDestRect);

            SDL_DestroyTexture(gameTypeTexture);
            SDL_DestroyTexture(gameValueTexture);
            SDL_DestroyTexture(arrowTexture);

            SDL_RenderPresent(ren);
        }
    } // while optionsMenuRunning
    Utility::PlaySound(sfxMenuConfirm);

    return new Options(gameType, score, stock, time, false);
}

int WinScreen(bool (&winningPlayer)[4], Player (&players)[4]) {
    bool WinScreenRunning = true;

    int ticksSinceStart = 1000;
    uint32_t nowTime = SDL_GetTicks();

    std::vector<int> vWinners;

    for (int i = 0; i < 4; ++i) {
        if (winningPlayer[i])
            vWinners.push_back(i + 1);
    }

    while (WinScreenRunning) {
        int frameTime = SDL_GetTicks() - nowTime;
        nowTime = SDL_GetTicks();

        if (ticksSinceStart > 0) {
            if (frameTime > ticksSinceStart)
                ticksSinceStart = 0;
            else
                ticksSinceStart -= frameTime;
        }

        gInput->CheckInput();

        if (gInput->QuitFlag() ) {
            return -1;
        }

        if (ticksSinceStart == 0) {
            if (gInput->EscapeHeld() || gInput->EnterHeld()) {
                WinScreenRunning = false;
            }

            for (int i = 0; i < 4; ++i) {
                if (gInput->Player(i) != nullptr)
                    if (gInput->Player(i)->StartHeld() || gInput->Player(i)->SelectHeld() || gInput->Player(i)->BackHeld())
                        WinScreenRunning = false;
            }
        }

        std::stringstream WinnerString;
        std::string scoreString = "Score";
        std::stringstream p1scoreString, p2scoreString, p3scoreString, p4scoreString;

        switch (vWinners.size()) {
        case 0:
            WinnerString << "No Contest!";
            break;
        case 1:
            WinnerString << "Player " << vWinners.at(0) << " wins!";
            break;
        case 2:
            WinnerString << "Players " << vWinners.at(0) << " & " << vWinners.at(1) << " tied!";
            break;
        case 3:
            WinnerString << "Players " << vWinners.at(0) << ", " << vWinners.at(1) << " & " << vWinners.at(2) << " tied!";
            break;
        case 4:
            WinnerString << "Tie game!";
            break;
        }

        p1scoreString << "Player 1: " << players[0].GetScore();
        p2scoreString << "Player 2: " << players[1].GetScore();
        p3scoreString << "Player 3: " << players[2].GetScore();
        p4scoreString << "Player 4: " << players[3].GetScore();

        int winnersW, winnersH;
        int scoreW, scoreH;
        int p1W, p1H;
        int p2W, p2H;
        int p3W, p3H;
        int p4W, p4H;

        SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(ren);

        SDL_Color c = { 0xFF, 0xFF, 0xFF, 0xFF };

        SDL_Texture* winnersTex = Utility::RenderText(WinnerString.str(), GAME_FONT, c, 18, ren);
        SDL_Texture* scoreTex = Utility::RenderText(scoreString, GAME_FONT, c, 16, ren);
        SDL_Texture* p1Tex = Utility::RenderText(p1scoreString.str(), GAME_FONT, c, 14, ren);
        SDL_Texture* p2Tex = Utility::RenderText(p2scoreString.str(), GAME_FONT, c, 14, ren);
        SDL_Texture* p3Tex = Utility::RenderText(p3scoreString.str(), GAME_FONT, c, 14, ren);
        SDL_Texture* p4Tex = Utility::RenderText(p4scoreString.str(), GAME_FONT, c, 14, ren);

        SDL_QueryTexture(winnersTex, NULL, NULL, &winnersW, &winnersH);
        SDL_QueryTexture(scoreTex, NULL, NULL, &scoreW, &scoreH);
        SDL_QueryTexture(p1Tex, NULL, NULL, &p1W, &p1H);
        SDL_QueryTexture(p2Tex, NULL, NULL, &p2W, &p2H);
        SDL_QueryTexture(p3Tex, NULL, NULL, &p3W, &p3H);
        SDL_QueryTexture(p4Tex, NULL, NULL, &p4W, &p4H);

        SDL_Rect winnersRect, scoreRect, p1Rect, p2Rect, p3Rect, p4Rect;

        winnersRect.x = (SCREEN_WIDTH / 2) - (winnersW / 2);
        winnersRect.y = (SCREEN_HEIGHT / 2) - (winnersH / 2);
        winnersRect.h = winnersH;
        winnersRect.w = winnersW;

        scoreRect.x = (SCREEN_WIDTH / 2) - (scoreW / 2);
        scoreRect.y = (SCREEN_HEIGHT / 2) + (winnersH / 2) + 4;
        scoreRect.w = scoreW;
        scoreRect.h = scoreH;

        int verticalOffset = (SCREEN_HEIGHT / 2) + (winnersH / 2) + 4 + scoreH + 2;

        if (playersIn[0]) {
            p1Rect.x = (SCREEN_WIDTH / 2) - (p1W / 2);
            p1Rect.y = verticalOffset;
            p1Rect.h = p1H;
            p1Rect.w = p1W;
            verticalOffset += p1H + 2;
            SDL_RenderCopy(ren, p1Tex, NULL, &p1Rect);
        }

        if (playersIn[1]) {
            p2Rect.x = (SCREEN_WIDTH / 2) - (p2W / 2);
            p2Rect.y = verticalOffset;
            p2Rect.h = p2H;
            p2Rect.w = p2W;
            verticalOffset += p2H + 2;
            SDL_RenderCopy(ren, p2Tex, NULL, &p2Rect);
        }

        if (playersIn[2]) {
            p3Rect.x = (SCREEN_WIDTH / 2) - (p3W / 2);
            p3Rect.y = verticalOffset;
            p3Rect.h = p3H;
            p3Rect.w = p3W;
            verticalOffset += p3H + 2;
            SDL_RenderCopy(ren, p3Tex, NULL, &p3Rect);
        }

        if (playersIn[3]) {
            p4Rect.x = (SCREEN_WIDTH / 2) - (p4W / 2);
            p4Rect.y = verticalOffset;
            p4Rect.h = p4H;
            p4Rect.w = p4W;
            verticalOffset += p4H + 2;
            SDL_RenderCopy(ren, p4Tex, NULL, &p4Rect);
        }

        SDL_RenderCopy(ren, winnersTex, NULL, &winnersRect);
        SDL_RenderCopy(ren, scoreTex, NULL, &scoreRect);

        SDL_RenderPresent(ren);

        SDL_DestroyTexture(winnersTex);
        SDL_DestroyTexture(scoreTex);
        SDL_DestroyTexture(p1Tex);
        SDL_DestroyTexture(p2Tex);
        SDL_DestroyTexture(p3Tex);
        SDL_DestroyTexture(p4Tex);
    }
    Utility::PlaySound(sfxMenuConfirm);
    return 0;
}

void Quit(int status) {
    /*
    for (int i = 0; i < 4; ++i) {
        SDL_JoystickClose(gController[i]);
        //SDL_HapticClose(gHaptic[i]);
        gController[i] = NULL;
        //gHaptic[i] = NULL;
    }
    */

    delete gInput;

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);


    Mix_FreeChunk(sfxFire);
    Mix_FreeChunk(sfxDie);
    for (int i = 0; i < 3; ++i)
        Mix_FreeChunk(sfxBounce[i]);
    for (int i = 0; i < MAX_TILESET + 1; ++i)
        Mix_FreeChunk(sfxPowerupBounce[i]);
    for (int i = 0; i < MAX_TILESET + 1; ++i)
        Mix_FreeChunk(sfxPowerupBullet[i]);
    for (int i = 0; i < MAX_TILESET + 1; ++i)
        Mix_FreeChunk(sfxPowerupSpeed[i]);
    for (int i = 0; i < MAX_TILESET + 1; ++i)
        Mix_FreeMusic(gGameMusic[i]);
    for (int i = 0; i < MAX_TILESET + 1; ++i)
        Mix_FreeMusic(introMusic[i]);

    Mix_Quit();
    #ifdef _DEBUG_BUILD
    std::cout << "Mix_Quit() successful" << std::endl;
    #endif

    TTF_Quit();
    #ifdef _DEBUG_BUILD
    std::cout << "TTF_Quit() successful" << std::endl;
    #endif // _DEBUG_BUILD

    IMG_Quit();
    #ifdef _DEBUG_BUILD
    std::cout << "IMG_Quit() successful" << std::endl;
    #endif // _DEBUG_BUILD

    SDL_Quit();
    #ifdef _DEBUG_BUILD
    std::cout << "SDL_Quit() successful" << std::endl;
    #endif // _DEBUG_BUILD
    char c;
    std::cin >> c;

    exit(status);

}

int Title() {

    bool titleRunning = true;
    SDL_Texture* titleTexture = Utility::LoadTexture(ren, TITLE_IMG);

    int ticksElapsed = 0;

    int time = SDL_GetTicks();
    int renderTime = SDL_GetTicks();

    const int fadeInTicks = 2000;
    const int flashTicks = 500;
    bool fadeInDone = false;
    bool flash = true;

    SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(ren);
    SDL_RenderPresent(ren);
    SDL_Delay(500);

    bool quit = false;

    uint32_t ticksSincePause = 0;

    while (titleRunning) {

        uint32_t frameTime = SDL_GetTicks() - time;
        time = SDL_GetTicks();

        gInput->CheckInput();

        if (ticksSincePause > 0) {
            if (frameTime > ticksSincePause) {
                ticksSincePause = 0;
            } else {
                ticksSincePause -= frameTime;
            }
        }

        if (gInput->QuitFlag() ) {
            quit = true;
            titleRunning = false;
        }

        if (fadeInDone) {
            if (gInput->EscapeHeld() && fadeInDone && ticksSincePause == 0) {
                if (ConfirmQuit() == -1) {
                    quit = true;
                    titleRunning = false;
                } else {
                    ticksSincePause = 2000;
                    gInput->CheckInput();
                }
            }

            if (gInput->EnterHeld() && fadeInDone) {
                titleRunning = false;
            }

            for (int i = 0; i < 4; ++i) {
                if (gInput->Player(i) == nullptr) {
                    continue;
                }
                if (gInput->Player(i)->StartHeld()) {
                    titleRunning = false;
                }
                if (gInput->Player(i)->BackHeld() && ticksSincePause == 0) {
                    if (ConfirmQuit() == -1) {
                        titleRunning = false;
                        quit = true;
                    } else {
                        ticksSincePause = 2000;
                        gInput->CheckInput();
                    }
                }
            }
        }

        if (!fadeInDone) {
            ticksElapsed += frameTime;
            if (ticksElapsed >= fadeInTicks) {
                fadeInDone = true;
                ticksElapsed = 0;
            }
        } else {
            ticksElapsed += frameTime;
            if (ticksElapsed > flashTicks) {
                ticksElapsed = 0;
                flash = !flash;
            }
        }

        int alpha = 0;
        if (!fadeInDone) {
            alpha = 255 - ((float)ticksElapsed / (fadeInTicks) * 255);
        }

        if (SDL_TICKS_PASSED(time - renderTime, RENDER_INTERVAL) ) {

            renderTime = SDL_GetTicks();


            SDL_Surface* surf = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, rmask, gmask, bmask, amask);
            SDL_FillRect(surf, NULL, SDL_MapRGBA(surf->format, 0x00, 0x00, 0x00, alpha));
            SDL_Texture* fadeTex = SDL_CreateTextureFromSurface(ren, surf);
            SDL_FreeSurface(surf);

            SDL_Color white = {0xFF, 0xFF, 0xFF, 0xFF};
            SDL_Texture* startTexture = Utility::RenderText("Press Start / Options", GAME_FONT, white, 16, ren);
            int startW, startH;
            SDL_QueryTexture(startTexture, NULL, NULL, &startW, &startH);

            SDL_Rect startRect;

            startRect.x = (SCREEN_WIDTH / 2) - (startW / 2);
            startRect.y = (SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / 4);
            startRect.w = startW;
            startRect.h = startH;

            SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, titleTexture, NULL, NULL);
            SDL_RenderCopy(ren, fadeTex, NULL, NULL);

            if (fadeInDone & flash) {
                SDL_RenderCopy(ren, startTexture, NULL, &startRect);
            }

            SDL_RenderPresent(ren);
            SDL_DestroyTexture(fadeTex);
            SDL_DestroyTexture(startTexture);
        }

    }
    SDL_DestroyTexture(titleTexture);

    if (quit)
        return -1;
    else {
        Utility::PlaySound(sfxMenuConfirm);
        return 0;
    }
}

int DisplayControls() {
    bool controlsDisplayRunning = true;

    const int cursorRepeatV = MENU_REPEAT_VERT_TICKS;
    const int cursorRepeatH = MENU_REPEAT_HORIZ_TICKS;
    int ticksSinceMove[4] = {0, 0, 0, 0};

    SDL_Texture* controlsTex = NULL;

    controlsTex = Utility::LoadTexture(ren, CONTROLS_IMG);

    uint32_t nowTime, renderTime;

    uint32_t ticksSinceStart = 1000;
    uint32_t ticksSincePause = 0;

    renderTime = nowTime = SDL_GetTicks();

    bool quit = false;

    while (controlsDisplayRunning) {

        uint32_t frameTime = SDL_GetTicks() - nowTime;
        nowTime = SDL_GetTicks();

        if (ticksSinceStart > 0) {
            if (frameTime > ticksSinceStart)
                ticksSinceStart = 0;
            else
                ticksSinceStart -= frameTime;
        }

        if (ticksSincePause > 0) {
            if (frameTime > ticksSincePause)
                ticksSincePause = 0;
            else
                ticksSincePause -= frameTime;
        }

        gInput->CheckInput();

        if (gInput->QuitFlag()) {
            quit = true;
            controlsDisplayRunning = false;
        }

        if (gInput->EscapeHeld() && ticksSinceStart == 0 && ticksSincePause == 0) {
            if (ConfirmQuit() == -1) {
                quit = true;
                controlsDisplayRunning = false;
            } else {
                ticksSincePause = 2000;
                gInput->CheckInput();
            }
        }

        if (gInput->EnterHeld() && ticksSinceStart == 0) {
            controlsDisplayRunning = false;
        }

        for (int i = 0; i < 4; ++i) {
            if (gInput->Player(i) == nullptr) {
                continue;
            }
            if (gInput->Player(i)->StartHeld() && ticksSinceStart == 0)
                controlsDisplayRunning = false;
            if (gInput->Player(i)->BackHeld() && ticksSinceStart == 0 && ticksSincePause == 0) {
                if (ConfirmQuit() == -1) {
                    quit = true;
                    controlsDisplayRunning = false;
                } else {
                    ticksSincePause = 2000;
                    gInput->CheckInput();
                }
            }
            if (ticksSinceMove[i] > 0) {
                ticksSinceMove[i] = std::max((uint32_t) 0, ticksSinceMove[i] - frameTime);
            } else  {
                if (gInput->Player(i)->UpHeld()) {
                    ticksSinceMove[i] = cursorRepeatV;
                } else if (gInput->Player(i)->DownHeld()) {
                    ticksSinceMove[i] = cursorRepeatV;
                } else if (gInput->Player(i)->LeftHeld()) {
                    ticksSinceMove[i] = cursorRepeatH;
                } else if (gInput->Player(i)->RightHeld()) {
                    ticksSinceMove[i] = cursorRepeatH;
                }
            }
        }

        if (SDL_TICKS_PASSED(nowTime - renderTime, RENDER_INTERVAL) ) {

            SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
            SDL_RenderClear(ren);

            SDL_RenderCopy(ren, controlsTex, NULL, NULL);

            SDL_RenderPresent(ren);
        }
    }

    SDL_DestroyTexture(controlsTex);

    if (quit)
        return -1;
    else {
        Utility::PlaySound(sfxMenuConfirm);
        return 0;
    }
}

int Pause() {

    bool pauseRunning = true;
    bool quit = false;

    uint32_t ticksSinceStart = 500;

    uint32_t nowTime, renderTime;
    nowTime = renderTime = SDL_GetTicks();

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Texture* pausedTex = Utility::RenderText("Paused", GAME_FONT, white, 16, ren);
    SDL_Texture* resumeTex = Utility::RenderText("Press Start to Resume", GAME_FONT, white, 12, ren);
    SDL_Texture* quitTex = Utility::RenderText("Press Back to Quit", GAME_FONT, white, 12, ren);

    int pausedW, pausedH;
    int resumeW, resumeH;
    int quitW, quitH;

    SDL_QueryTexture(pausedTex, NULL, NULL, &pausedW, &pausedH);
    SDL_QueryTexture(resumeTex, NULL, NULL, &resumeW, &resumeH);
    SDL_QueryTexture(quitTex, NULL, NULL, &quitW, &quitH);

    SDL_Rect bgRect, pausedRect, resumeRect, quitRect;

    pausedRect.x = (320 / 2) - (pausedW / 2);
    resumeRect.x = (320 / 2) - (resumeW / 2);
    quitRect.x = (320 / 2) - (quitW / 2);

    pausedRect.y = (SCREEN_HEIGHT / 2) - ((pausedH + 2 + resumeH + 2 + quitH) / 2);
    resumeRect.y = pausedRect.y + pausedH + 2;
    quitRect.y = resumeRect.y + resumeH + 2;

    pausedRect.h = pausedH;
    pausedRect.w = pausedW;

    resumeRect.h = resumeH;
    resumeRect.w = resumeW;

    quitRect.h = quitH;
    quitRect.w = quitW;

    int baseWidth = std::max(pausedW, std::max(resumeW, quitW));
    int baseHeight = pausedH + 2 + resumeH + 2 + quitH;

    bgRect.x = (320 / 2) - (baseWidth / 2) - 2;
    bgRect.y = (SCREEN_HEIGHT / 2) - (baseHeight / 2) - 2;
    bgRect.h = baseHeight + 4;
    bgRect.w = baseWidth + 4;

    SDL_Surface* bgSurface = SDL_CreateRGBSurface(0, baseWidth, baseHeight, 32, rmask, gmask, bmask, amask);
    SDL_FillRect(bgSurface, NULL, SDL_MapRGB(bgSurface->format, 0, 0, 0));
    SDL_Texture* bgTexture = SDL_CreateTextureFromSurface(ren, bgSurface);
    SDL_FreeSurface(bgSurface);

    SDL_RenderCopy(ren, bgTexture, NULL, &bgRect);

    SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderDrawRect(ren, &bgRect);

    SDL_RenderCopy(ren, pausedTex, NULL, &pausedRect);
    SDL_RenderCopy(ren, resumeTex, NULL, &resumeRect);
    SDL_RenderCopy(ren, quitTex, NULL, &quitRect);


    SDL_RenderPresent(ren);



    Mix_PauseMusic();
    Utility::PlaySound(sfxPause);

    while (pauseRunning) {

        uint32_t frame_time = SDL_GetTicks() - nowTime;
        nowTime = SDL_GetTicks();

        if (ticksSinceStart > 0) {
            if (frame_time > ticksSinceStart)
                ticksSinceStart = 0;
            else
                ticksSinceStart -= frame_time;
        }

        gInput->CheckInput();

        if (gInput->QuitFlag()) {
            pauseRunning = false;
            quit = true;
        }

        if (gInput->EnterHeld() && ticksSinceStart == 0) {
            pauseRunning = false;
        } else if (gInput->EscapeHeld() && ticksSinceStart == 0) {
            pauseRunning = false;
            quit = true;
        }

        for (int i = 0; i < 4; ++i) {
            if (gInput->Player(i) != nullptr) {
                if (gInput->Player(i)->StartHeld() && ticksSinceStart == 0) {
                    pauseRunning = false;
                } else if (gInput->Player(i)->BackHeld() && ticksSinceStart == 0) {
                    pauseRunning = false;
                    quit = true;
                }
            }
        }

        if (SDL_TICKS_PASSED(nowTime - renderTime, RENDER_INTERVAL)) {
            renderTime = nowTime;

            SDL_RenderCopy(ren, bgTexture, NULL, &bgRect);

            SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderDrawRect(ren, &bgRect);

            SDL_RenderCopy(ren, pausedTex, NULL, &pausedRect);
            SDL_RenderCopy(ren, resumeTex, NULL, &resumeRect);
            SDL_RenderCopy(ren, quitTex, NULL, &quitRect);

            SDL_RenderPresent(ren);
        }
    }

    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(pausedTex);
    SDL_DestroyTexture(resumeTex);
    SDL_DestroyTexture(quitTex);

    if (quit) {
        Utility::PlaySound(sfxMenuConfirm);
        Mix_ResumeMusic();
        return -1;
    } else {
        Utility::PlaySound(sfxUnpause);
        Mix_ResumeMusic();
        return 0;
    }
}


int ConfirmQuit() {

    bool pauseRunning = true;
    bool quit = false;

    uint32_t ticksSinceStart = 500;

    uint32_t nowTime, renderTime;
    nowTime = renderTime = SDL_GetTicks();

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Texture* pausedTex = Utility::RenderText("Really Quit?", GAME_FONT, white, 16, ren);
    SDL_Texture* resumeTex = Utility::RenderText("Press Start to Confirm", GAME_FONT, white, 12, ren);
    SDL_Texture* quitTex = Utility::RenderText("Press Back to Cancel", GAME_FONT, white, 12, ren);

    int pausedW, pausedH;
    int resumeW, resumeH;
    int quitW, quitH;

    SDL_QueryTexture(pausedTex, NULL, NULL, &pausedW, &pausedH);
    SDL_QueryTexture(resumeTex, NULL, NULL, &resumeW, &resumeH);
    SDL_QueryTexture(quitTex, NULL, NULL, &quitW, &quitH);

    SDL_Rect bgRect, pausedRect, resumeRect, quitRect;

    pausedRect.x = (SCREEN_WIDTH / 2) - (pausedW / 2);
    resumeRect.x = (SCREEN_WIDTH / 2) - (resumeW / 2);
    quitRect.x = (SCREEN_WIDTH / 2) - (quitW / 2);

    pausedRect.y = (SCREEN_HEIGHT / 2) - ((pausedH + 2 + resumeH + 2 + quitH) / 2);
    resumeRect.y = pausedRect.y + pausedH + 2;
    quitRect.y = resumeRect.y + resumeH + 2;

    pausedRect.h = pausedH;
    pausedRect.w = pausedW;

    resumeRect.h = resumeH;
    resumeRect.w = resumeW;

    quitRect.h = quitH;
    quitRect.w = quitW;

    int baseWidth = std::max(pausedW, std::max(resumeW, quitW));
    int baseHeight = pausedH + 2 + resumeH + 2 + quitH;

    bgRect.x = (SCREEN_WIDTH / 2) - (baseWidth / 2) - 2;
    bgRect.y = (SCREEN_HEIGHT / 2) - (baseHeight / 2) - 2;
    bgRect.h = baseHeight + 4;
    bgRect.w = baseWidth + 4;

    SDL_Surface* bgSurface = SDL_CreateRGBSurface(0, baseWidth, baseHeight, 32, rmask, gmask, bmask, amask);
    SDL_FillRect(bgSurface, NULL, SDL_MapRGB(bgSurface->format, 0, 0, 0));
    SDL_Texture* bgTexture = SDL_CreateTextureFromSurface(ren, bgSurface);
    SDL_FreeSurface(bgSurface);

    SDL_RenderCopy(ren, bgTexture, NULL, &bgRect);

    SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderDrawRect(ren, &bgRect);

    SDL_RenderCopy(ren, pausedTex, NULL, &pausedRect);
    SDL_RenderCopy(ren, resumeTex, NULL, &resumeRect);
    SDL_RenderCopy(ren, quitTex, NULL, &quitRect);


    SDL_RenderPresent(ren);



    Utility::PlaySound(sfxMenu);

    while (pauseRunning) {

        uint32_t frame_time = SDL_GetTicks() - nowTime;
        nowTime = SDL_GetTicks();

        if (ticksSinceStart > 0) {
            if (frame_time > ticksSinceStart)
                ticksSinceStart = 0;
            else
                ticksSinceStart -= frame_time;
        }

        gInput->CheckInput();

        if (gInput->QuitFlag()) {
            pauseRunning = false;
            quit = true;
        }

        if (gInput->EnterHeld() && ticksSinceStart == 0) {
            pauseRunning = false;
            quit = true;
        } else if (gInput->EscapeHeld() && ticksSinceStart == 0) {
            pauseRunning = false;
        }

        for (int i = 0; i < 4; ++i) {
            if (gInput->Player(i) != nullptr) {
                if (gInput->Player(i)->StartHeld() && ticksSinceStart == 0) {
                    pauseRunning = false;
                    quit = true;
                } else if (gInput->Player(i)->BackHeld() && ticksSinceStart == 0) {
                    pauseRunning = false;
                }
            }
        }

        if (SDL_TICKS_PASSED(nowTime - renderTime, RENDER_INTERVAL)) {
            renderTime = nowTime;

            SDL_RenderCopy(ren, bgTexture, NULL, &bgRect);

            SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderDrawRect(ren, &bgRect);

            SDL_RenderCopy(ren, pausedTex, NULL, &pausedRect);
            SDL_RenderCopy(ren, resumeTex, NULL, &resumeRect);
            SDL_RenderCopy(ren, quitTex, NULL, &quitRect);

            SDL_RenderPresent(ren);
        }
    }

    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(pausedTex);
    SDL_DestroyTexture(resumeTex);
    SDL_DestroyTexture(quitTex);

    Utility::PlaySound(sfxMenuConfirm);
    if (quit)
        return -1;
    else {
        return 0;
    }

}

