/*
 * ShotFlicker.cpp
 *
 *  Created on: 5 Sep 2019
 *      Author: xasin
 */


#include "lzrtag/animatorThread.h"
#include "lzrtag/patterns/ShotFlicker.h"

namespace LZR {
namespace FX {

ShotFlicker::ShotFlicker(float length, int points, LZRTag::Weapon::Handler* handler)
    : BasePattern(),
      anim(points),
      gunHandler(handler),
      bufferedColors(nullptr), // initialize pointer in the same order as class
      maxLen(length),
      pointCount(points)
{
    anim.baseTug   = 0.002;
    anim.basePoint = 0.0;
    anim.dampening = 0.94;
    anim.ptpTug    = 0.02;
    anim.wrap      = true;
}

void ShotFlicker::tick() {
    if(this->gunHandler && this->gunHandler->was_shot_tick())
        anim.points[0].pos = 1;
    anim.tick();
}

// Explicitly qualify member variable for clarity
void ShotFlicker::apply_color_at(Xasin::NeoController::Color &tgt, float index) {
    if (!(this->bufferedColors)) return;
    Xasin::NeoController::Color shotColor = this->bufferedColors->vestShotEnergy;

    if(index < 0)
        index = 0;
    if(index > maxLen)
        index = maxLen;

    shotColor.bMod(anim.scalarPoints[(index/maxLen) * pointCount]);
    tgt.merge_add(shotColor);
}

} /* namespace FX */
} /* namespace LZR */
