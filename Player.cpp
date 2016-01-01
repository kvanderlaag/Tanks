#include "Player.hpp"

//#define _COLLISION_DEBUG

Player::Player(const std::string& filename, int id, SDL_Renderer* ren) :
    RenderableObject(filename, PLAYER_SIZE, PLAYER_SIZE, 0, 0, 0, ren),
    mID(id),
    score(0),
    mVelocity(Vector2D(0, 0)),
    mForwardVel(0),
    mRotationVel(0),
    mTurretAngle(0),
    mTurretRotationVel(0),
    mCollider(PLAYER_SIZE, PLAYER_SIZE, 0, 0, 0, this),
    mMaxBullets(1),
    mBullets(0),
    mMaxBounce(1),
    mFireHeld(false),
    mJoyMove(false),
    mJoyRotate(false),
    mJoyTurret(false),
    mInvincible(0),
    mFlashTicks(0),
    mInvisible(false)
{
    SDL_QueryTexture(mTexture, NULL, NULL, &width, &height);
    width = width / (width / 16);
}

Player::~Player() {

}



const float Player::GetXVel() const {
    return mVelocity.GetX();
}

const float Player::GetYVel() const {
    return mVelocity.GetY();
}

void Player::SetXVel(float newvel) {
    mVelocity.SetX(newvel);
}

void Player::SetYVel(float newvel) {
    mVelocity.SetY(newvel);
}

void Player::SetX(float newx) {
    x = newx;
    mCollider.Move(newx, y);
}

void Player::SetY(float newy) {
    y = newy;
    mCollider.Move(x, newy);
}

void Player::SetForwardVel(float newvel) {
    mForwardVel = newvel;
}

const float Player::GetForwardVel() const {
    return mForwardVel;
}

const float Player::GetRotationVel() const {
    return mRotationVel;
}

void Player::SetRotationVel(float newvel) {
    mRotationVel = newvel;
}

const float Player::GetTurretRotationVel() const {
    return mTurretRotationVel;
}

void Player::SetTurretRotationVel(float newvel) {
    mTurretRotationVel = newvel;
}

void Player::Render() {
    if (mInvisible) {
        return;
    }

    SDL_Rect srcRect, dstRect;

    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.h = height;
    srcRect.w = width;

    dstRect.x = x - (width / 2);
    dstRect.y = y - (height / 2);
    dstRect.h = height;
    dstRect.w = width;
    SDL_RenderCopyEx(mRenderer, mTexture, &srcRect, &dstRect, angle, NULL, SDL_FLIP_NONE);

    srcRect.x = 16;
    srcRect.y = 0;
    srcRect.h = height;
    srcRect.w = width;
    SDL_RenderCopyEx(mRenderer, mTexture, &srcRect, &dstRect, mTurretAngle, NULL, SDL_FLIP_NONE);

    /* Collider rendering
    std::vector<Point> colliderPoints = mCollider.GetPoints();
    SDL_SetRenderDrawColor(mRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

    SDL_RenderDrawLine(mRenderer, colliderPoints[0].x, colliderPoints[0].y, colliderPoints[1].x, colliderPoints[1].y);
    SDL_RenderDrawLine(mRenderer, colliderPoints[1].x, colliderPoints[1].y, colliderPoints[2].x, colliderPoints[2].y);
    SDL_RenderDrawLine(mRenderer, colliderPoints[2].x, colliderPoints[2].y, colliderPoints[3].x, colliderPoints[3].y);
    SDL_RenderDrawLine(mRenderer, colliderPoints[3].x, colliderPoints[3].y, colliderPoints[0].x, colliderPoints[0].y);
    /* End of Collider Rendering */
}

const Collider& Player::GetCollider() const {
    return mCollider;
}

void Player::CheckCollision(const Collider& other, uint32_t ticks) {
    if (&mCollider != &other) {
        Vector2D velocity(mVelocity.GetX() * ticks / 100, mVelocity.GetY() * ticks / 100);
        CollisionInfo coll = mCollider.CheckCollision(other, velocity);
        Vector2D mt = coll.MinimumTranslation();
        x += mt.GetX();
        y += mt.GetY();
    }
}

void Player::Update(uint32_t ticks) {

    if (mInvincible > 0) {
        mInvincible -= ticks;
        mFlashTicks += ticks;
        if (mFlashTicks >= TICKS_PER_FLASH) {
            mInvisible = !mInvisible;
            mFlashTicks = 0;
        }

        if (mInvincible <= 0) {
            mFlashTicks = 0;
            mInvincible = 0;
            mInvisible = false;
        }
    } else if (mInvisible) {
        mInvisible = false;
    }

    float forwardPerFrame = mForwardVel * ((float)ticks / 1000) * MOVE_SPEED ;
    float rotationPerFrame = mRotationVel * ((float)ticks / 1000) * ROTATE_SPEED ;
    float turretRotationPerFrame = mTurretRotationVel * ((float) ticks / 1000) * ROTATE_SPEED ;

    angle += rotationPerFrame;
    if (angle < 0) {
        angle += 360;
    }
    if (angle > 360) {
        angle -= 360;
    }

    mCollider.Rotate(rotationPerFrame);

    mTurretAngle += (rotationPerFrame + turretRotationPerFrame);
    if (mTurretAngle < 0) {
        mTurretAngle = 360 + mTurretAngle;
    }
    if (mTurretAngle > 360) {
        mTurretAngle = mTurretAngle - 360;
    }


    float a = angle * M_PI / 180;
    mVelocity.SetX( std::sin(a) * forwardPerFrame );
    mVelocity.SetY( std::cos(a) * -forwardPerFrame );
    if (mVelocity.GetX() != 0) {
        if (mVelocity.GetX() < 0 && x > (width / 2)) {
            x += mVelocity.GetX();
        }
        else if (mVelocity.GetX() > 0 && x + (width / 2) < maxX) {
            x += mVelocity.GetX();
        }
    }
    if (mVelocity.GetY() != 0) {
        if (mVelocity.GetY() < 0 && y > (height / 2)) {
            y += mVelocity.GetY();
        }
        else if (mVelocity.GetY() > 0 && y + (height / 2) < maxY) {
            y += mVelocity.GetY();
        }
    }

    mCollider.Move(x, y);
}

Bullet* Player::Fire() {
    if (mBullets >= mMaxBullets) {
        return nullptr;
    }
    mBullets++;
    float a = mTurretAngle * M_PI / 180;
    Vector2D direction(std::sin(a), -std::cos(a));
    float bx = 0;
    float by = -8;

    // now apply rotation
    float rotatedX = bx*std::cos(a) - by*std::sin(a);
    float rotatedY = bx*std::sin(a) + by*std::cos(a);

    bx = rotatedX + x;
    by = rotatedY + y;


    Bullet* b = new Bullet(bx, by, mTurretAngle, direction.Normalized(), *this, mRenderer);

    return b;
}

const int Player::GetMaxBullets() const {
    return mMaxBullets;
}

const int Player::GetBullets() const {
    return mBullets;
}

const int Player::GetMaxBounce() const {
    return mMaxBounce;
}

void Player::DestroyBullet() {
    if (mBullets > 0) {
        --mBullets;
    }
}

const int Player::GetID() const {
    return mID;
}

const int Player::GetScore() const {
    return score;
}

void Player::AddScore(const int mod) {
    score += mod;
}

const bool Player::FireHeld() const {
    return mFireHeld;
}

void Player::FireIsHeld(const bool val) {
    mFireHeld = val;
}

const bool Player::JoyMove() const {
    return mJoyMove;
}

void Player::SetJoyMove(const bool val) {
    mJoyMove = val;
}

const bool Player::JoyRotate() const {
    return mJoyRotate;
}

void Player::SetJoyRotate(const bool val) {
    mJoyRotate = val;
}

const bool Player::JoyTurret() const {
    return mJoyTurret;
}

void Player::SetJoyTurret(const bool val) {
    mJoyTurret = val;
}

const bool Player::IsInvincible() const {
    return mInvincible > 0;
}

void Player::Invincible() {
    mInvincible = INVINCIBLE_TICKS;
    std::cout << "Player " << mID << " is invincible." << std::endl;
}
