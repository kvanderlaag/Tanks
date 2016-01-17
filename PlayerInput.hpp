#ifndef _PLAYERINPUT_H
#define _PLAYERINPUT_H

#include <string>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_haptic.h>

#include <iostream>

#include "Defines.hpp"
#include "Vector2D.hpp"

class PlayerInput {
public:

    PlayerInput(const int id);
    ~PlayerInput();
    void CheckInput();

    const int ID() const;

    const std::string& JoystickName() const;

    const bool UpHeld() const;
    const bool DownHeld() const;
    const bool LeftHeld() const;
    const bool RightHeld() const;

    const bool FireHeld() const;
    const bool SelectHeld() const;
    const bool CancelHeld() const;

    const int LeftStickX() const;
    const int LeftStickY() const;

    const int RightStickX() const;
    const int RightStickY() const;

    const Vector2D LeftStickVector() const;
    const Vector2D RightStickVector() const;

    void FireRumble() const;
    void DieRumble() const;

private:

    const int JOYLSTICK_DEADZONE = 12000;
    const int JOYRSTICK_DEADZONE = 12000;
    const int JOYLTRIGGER_DEADZONE = 0;
    const int JOYRTRIGGER_DEADZONE = 0;

    const int JHAT_DPADCENTER    = 0;
    const int JHAT_DPADUP        = 1;
    const int JHAT_DPADRIGHT     = 2;
    const int JHAT_DPADDOWN      = 4;
    const int JHAT_DPADLEFT      = 8;

    const int JHAT_DPADUPRIGHT   = JHAT_DPADUP | JHAT_DPADRIGHT;
    const int JHAT_DPADDOWNRIGHT = JHAT_DPADDOWN | JHAT_DPADRIGHT;
    const int JHAT_DPADDOWNLEFT  = JHAT_DPADDOWN | JHAT_DPADLEFT;
    const int JHAT_DPADUPLEFT    = JHAT_DPADUP | JHAT_DPADLEFT;

    const int XBOX_JBUTTON_START         = 7;
    const int XBOX_JBUTTON_BACK          = 6;
    const int XBOX_JBUTTON_RBUMPER         = 5;
    const int XBOX_JBUTTON_A             = 0;
    const int XBOX_JBUTTON_B             = 1;
    const int XBOX_JAXIS_LTRIGGER        = 0x02;
    const int XBOX_JAXIS_RTRIGGER            = 0x05;
    const int XBOX_JAXIS_RSTICKX         = 0x03;
    const int XBOX_JAXIS_RSTICKY         = 0x04;
    const int XBOX_JAXIS_LSTICKX           = 0x00;
    const int XBOX_JAXIS_LSTICKY           = 0x01;

    const int PS4_JBUTTON_OPTIONS         = 9;
    const int PS4_JBUTTON_SHARE          = 8;
    const int PS4_JBUTTON_R1          = 5;
    const int PS4_JBUTTON_X             = 1;
    const int PS4_JBUTTON_CIRCLE             = 2;
    const int PS4_JAXIS_L2        = 0x03;
    const int PS4_JAXIS_R2            = 0x04;
    const int PS4_JAXIS_RSTICKX         = 0x02;
    const int PS4_JAXIS_RSTICKY         = 0x05;
    const int PS4_JAXIS_LSTICKX           = 0x00;
    const int PS4_JAXIS_LSTICKY           = 0x01;


    int mPlayerID;
    int mControllerType;

    SDL_Joystick* mJoystick;
    SDL_Haptic* mHaptic;

    int mButtonFire;
    int mAxisFire;
    int mAxisLeftStickX, mAxisLeftStickY;
    int mAxisRightStickX, mAxisRightStickY;

    int mButtonA, mButtonB;
    int mButtonBack, mButtonStart;

    bool mUpHeld, mDownHeld, mRightHeld, mLeftHeld;

    int mLeftStickX, mLeftStickY;
    int mRightStickX, mRightStickY;

    bool mFireHeld;
    bool mCancelHeld;
    bool mSelectHeld;

    bool mStartHeld;
    bool mBackHeld;

    std::string mJoystickName;
};
#endif // _PLAYERINPUT_H
