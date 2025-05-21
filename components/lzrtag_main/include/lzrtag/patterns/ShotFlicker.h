/*
 * ShotFlicker.h
 *
 *  Created on: 5 Sep 2019
 *      Author: xasin
 */

#ifndef MAIN_FX_PATTERNS_SHOTFLICKER_H_
#define MAIN_FX_PATTERNS_SHOTFLICKER_H_

#include "BasePattern.h"
#include "lzrtag/ManeAnimator.h"
#include "lzrtag/weapon/handler.h"

namespace LZR {
struct ColorSet;
namespace FX {

class ShotFlicker: public BasePattern {
public:
	ShotFlicker(float length, int points, LZRTag::Weapon::Handler* handler);
	void set_buffered_colors(const LZR::ColorSet* colors) { bufferedColors = colors; }
	void tick();
	void apply_color_at(Xasin::NeoController::Color &tgt, float pos);

private:
	ManeAnimator anim;
	LZRTag::Weapon::Handler* gunHandler; // Store pointer to handler
	const LZR::ColorSet* bufferedColors; // Pointer to color set
	const float maxLen;
	const int   pointCount;
};

} /* namespace FX */
} /* namespace LZR */

#endif /* MAIN_FX_PATTERNS_SHOTFLICKER_H_ */
