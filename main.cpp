#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <climits>
#include <map>

#include "Player.hpp"
#include "Tile.hpp"
#include "Map.hpp"
#include "Rectangle.hpp"
#include "Vector2D.hpp"
#include "Point.hpp"
#include "Collider.hpp"
#include "Explosion.hpp"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define MAX_MOVE 1
#define MAX_ROTATE 2

#define RENDER_INTERVAL 16

const std::string basePath = SDL_GetBasePath();
void NewExplosion(const float x, const float y, SDL_Renderer* ren, std::map<int, RenderableObject*>& vRenderable, std::vector<Explosion*>& vExplosions);

int main(int argc, char** argv) {

    freopen("CON", "w", stdout);

    uint32_t ticks = SDL_GetTicks();
    uint32_t old_time = SDL_GetTicks();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER |SDL_INIT_JOYSTICK) < 0) {
        std::cout << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) < 0) {
        std::cout << "Error initializing SDL_IMG: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 2;
    }

    if (TTF_Init() != 0) {
        std::cout << "Error initializing SDL_TTF: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 3;
    }




    // Initialize joysticks
    bool joystick = true;
    const int JOYSTICK_DEADZONE = 8000;
    SDL_Joystick* gController = NULL;

    if (SDL_NumJoysticks() < 1) {
        joystick = false;
    } else {
        gController = SDL_JoystickOpen(0);
        if (gController == NULL) {
            joystick = false;
        }
    }

    int playerx, playery;
    playerx = (SCREEN_WIDTH / 2);
    playery = (SCREEN_HEIGHT / 2);

    SDL_Window* win = SDL_CreateWindow("Balls", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, SDL_WINDOW_SHOWN);
    //SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);
    SDL_ShowCursor(0);

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(ren, 320, 240);

    Map m("test.d", "WallTiles.png", ren);

    Player players[4] = { Player("Tank.png", 1, ren), Player("Tank.png", 2, ren), Player("Tank.png", 3, ren), Player("Tank.png", 4, ren) };
    Player& p = players[0];
    Vector2D startPos = m.GetStartPos(1);
    if (startPos.GetX() == 0 && startPos.GetY() == 0) {
        p.SetX(playerx);
        p.SetY(playery);
    } else {
        p.SetX(startPos.GetX());
        p.SetY(startPos.GetY());

    }

    if (p.GetTexture() == nullptr) {
        std::cout << "Error loading player sprite: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win);
        SDL_DestroyRenderer(ren);
        SDL_Quit();
        return 1;
    }

    uint32_t rmask, gmask, bmask, amask;
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

    std::vector<Player*> vMoving;
    std::vector<Collider*> vStationary;

    vMoving.push_back(&p);

    std::vector<Collider>& mapColliders = m.GetColliders();

    for (Collider& c : mapColliders) {
        vStationary.push_back(&c);
    }

    std::map<int, RenderableObject*> vRenderable;
    std::map<int, Bullet*> vBullets;
    std::vector<Explosion*> vExplosions;

    vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next, &p));
    RenderableObject::next++;
    vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next, &m));
    RenderableObject::next++;


    bool running = true;

    while (running == true) {
        uint32_t new_time = SDL_GetTicks();
        uint32_t frame_time = new_time - old_time;
        old_time = new_time;

        bool joymove = false;
        bool joyrotate = false;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_JOYAXISMOTION) {
                //std::cout << "Joystick " << e.jaxis.which << " - Axis " << e.jaxis.axis << ": " << e.jaxis.value << std::endl;
                if (e.jaxis.which == 0) {
                    if (e.jaxis.axis == 0x00) {
                        // X-axis
                        if (e.jaxis.value < -JOYSTICK_DEADZONE || e.jaxis.value > JOYSTICK_DEADZONE) {
                            joyrotate = true;

                            float scale = (e.jaxis.value > 0) - (e.jaxis.value < 0);
                            std::cout << "Rotating " << scale << std::endl;
                            p.SetRotationVel(MAX_ROTATE * scale);
                        } else {
                                joyrotate = false;
                                p.SetRotationVel(0);
                        }
                    } else if (e.jaxis.axis == 0x01) {
                        // Y-axis
                        if (e.jaxis.value < -JOYSTICK_DEADZONE || e.jaxis.value > JOYSTICK_DEADZONE) {
                            joymove = true;

                            float scale = (e.jaxis.value > 0) - (e.jaxis.value < 0);
                            std::cout << "Moving " << scale << std::endl;
                            p.SetForwardVel(MAX_MOVE * -scale);
                        } else {
                            joymove = false;
                            p.SetForwardVel(0);

                        }
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_PAUSE:
                case SDLK_END:
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_LEFT:
                    if (!joyrotate)
                        p.SetRotationVel(-MAX_ROTATE);
                    break;
                case SDLK_RIGHT:
                    if (!joyrotate)
                        p.SetRotationVel(MAX_ROTATE);
                    break;
                case SDLK_a:
                case SDLK_TAB:
                    p.SetTurretRotationVel(-MAX_ROTATE);
                    break;
                case SDLK_d:
                case SDLK_BACKSPACE:
                    p.SetTurretRotationVel(MAX_ROTATE);
                    break;
                case SDLK_UP:
                    if (!joymove)
                        p.SetForwardVel(MAX_MOVE);
                    break;
                case SDLK_DOWN:
                    if (!joymove)
                        p.SetForwardVel(-MAX_MOVE);
                    break;
                case SDLK_SPACE:
                    Bullet* b = p.Fire();
                    if (b != nullptr) {
                        int n = Bullet::next;
                        vBullets.insert(std::pair<int, Bullet*>(Bullet::next, b));
                        vRenderable.insert(std::pair<int, RenderableObject*>(RenderableObject::next, b));
                        RenderableObject::next++;
                    }
                    break;
                }

            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {

                case SDLK_LEFT:
                    if (p.GetRotationVel() < 0 && !joyrotate)
                        p.SetRotationVel(0);
                    break;
                case SDLK_RIGHT:
                    if (p.GetRotationVel() > 0 && !joyrotate)
                        p.SetRotationVel(0);
                    break;
                case SDLK_TAB:
                case SDLK_a:
                    if (p.GetTurretRotationVel() < 0)
                        p.SetTurretRotationVel(0);
                    break;
                case SDLK_BACKSPACE:
                case SDLK_d:
                    if (p.GetTurretRotationVel() > 0)
                        p.SetTurretRotationVel(0);
                    break;
                case SDLK_UP:
                    if (p.GetForwardVel() > 0 && !joymove)
                        p.SetForwardVel(0);
                    break;
                case SDLK_DOWN:
                    if (p.GetForwardVel() < 0 && !joymove)
                        p.SetForwardVel(0);
                    break;

                }

            }
        }

        for (Player* p : vMoving) {
            for (Collider* c : vStationary) {
                p->CheckCollision(*c, frame_time);
            }
        }

        for (GameObject* g : vMoving) {
            g->Update(frame_time);
        }

        for (std::pair<int, Bullet*> b : vBullets) {
            if (!b.second->IsDead()) {
                b.second->Update(frame_time);
                for (Player* pl : vMoving) {
                        CollisionInfo coll = b.second->CheckCollision(*pl, frame_time);
                        if (coll.Colliding() && b.second->GetBounce() > 0) {
                            if (&(b.second->GetOwner()) == pl) {
                                pl->AddScore(-1);
                            } else {
                                b.second->GetOwner().AddScore(1);
                            }

                            NewExplosion(pl->GetX(), pl->GetY(), ren, vRenderable, vExplosions);

                            pl->SetX(m.GetStartPos(pl->GetID()).GetX());
                            pl->SetY(m.GetStartPos(pl->GetID()).GetY());
                            b.second->GetOwner().DestroyBullet();
                            b.second->Die();


                        }
                }

                if (b.second->IsDead()) {
                    break;
                }

                for (Collider* c : vStationary) {
                    CollisionInfo coll = b.second->CheckCollision(*c, frame_time);
                    if (b.second->IsDead() && (coll.Colliding() || coll.WillCollide()) ) {
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
        }

        for (Explosion* e : vExplosions) {
            if (!e->IsDead()) {
                e->Update(frame_time);
            }
        }

        if (SDL_TICKS_PASSED(new_time - ticks, RENDER_INTERVAL)) {


            SDL_Surface* surf = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, rmask, gmask, bmask, amask);

            SDL_FillRect(surf, NULL, SDL_MapRGB(surf->format, 0x20, 0x20, 0x05));

            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);

            SDL_FreeSurface(surf);




            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, tex, NULL, NULL);

            for (std::pair<int, RenderableObject*> r : vRenderable) {
                if (!r.second->IsDead()) {
                    r.second->Render();
                }
            }

            /*for (Explosion* e : vExplosions) {
                e->Render();
            }*/

            /* Test rectangle
            SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderDrawPoint(ren, p.GetX(), p.GetY());


            r.Move(p.GetX(), p.GetY());
            r.SetAngle(p.GetAngle());
            std::vector<Point> RectPoints = r.GetPoints();
            Vector2D upNormal = r.GetUpNormal();
            Vector2D leftNormal = r.GetLeftNormal();
            Point center = r.GetCenter();

            SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderDrawLine(ren, RectPoints[0].x, RectPoints[0].y, RectPoints[1].x, RectPoints[1].y);
            SDL_RenderDrawLine(ren, RectPoints[1].x, RectPoints[1].y, RectPoints[2].x, RectPoints[2].y);
            SDL_RenderDrawLine(ren, RectPoints[2].x, RectPoints[2].y, RectPoints[3].x, RectPoints[3].y);
            SDL_RenderDrawLine(ren, RectPoints[3].x, RectPoints[3].y, RectPoints[0].x, RectPoints[0].y);

            SDL_RenderDrawLine(ren, center.x, center.y - 8, center.x + upNormal.GetX() * 5, center.y - 8 + upNormal.GetY() * 5);
            SDL_RenderDrawLine(ren, center.x - 8, center.y, center.x - 8 + leftNormal.GetX() * 5, center.y + leftNormal.GetY() * 5);
            /* end of test rectangle */

            /* FPS Texture
            SDL_Color c = {255, 255, 255, 255};
            std::string basePath = SDL_GetBasePath();
            uint32_t fps = 1000 / (new_time + (SDL_GetTicks() - new_time) - ticks);
            //if (frame_time > 0)
            //    fps = 1000 / frame_time;
            std::stringstream strFPS;
            strFPS << "FPS: " << fps;

            SDL_Texture* fps_tex = Utility::RenderText(strFPS.str(), basePath + "sample.ttf", c, 10, ren);

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

            /* Angle text

            std::stringstream strAngle;
            strAngle << "Ticks: " << frame_time;
            //SDL_Color c = {255, 255, 255, 255};
            SDL_Texture* angle_tex = Utility::RenderText(strAngle.str(), basePath + "sample.ttf", c, 10, ren);

            int angle_w, angle_h;
            SDL_QueryTexture(angle_tex, NULL, NULL, &angle_w, &angle_h);

            //SDL_Rect srcRect, dstRect;
            srcRect.x = 0;
            srcRect.y = 0;
            srcRect.h = angle_h;
            srcRect.w = angle_w;

            dstRect.x = 8;
            dstRect.y = 16;
            dstRect.h = angle_h;
            dstRect.w = angle_w;

            SDL_RenderCopy(ren, angle_tex, &srcRect, &dstRect);

            /* End of angle text */



            SDL_RenderPresent(ren);
            ticks = SDL_GetTicks();
        }


    }

    SDL_JoystickClose(gController);
    gController = NULL;
    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(ren);

    IMG_Quit();
    SDL_Quit();

    return 0;
}


void NewExplosion(const float x, const float y, SDL_Renderer* ren, std::map<int, RenderableObject*>& vRenderable, std::vector<Explosion*>& vExplosions) {
    Explosion* newExpl = new Explosion(x, y, ren);
    std::pair<int, RenderableObject*> newPair(RenderableObject::next, newExpl);
    RenderableObject::next++;
    vRenderable.insert(newPair);
    vExplosions.push_back(newExpl);
}
