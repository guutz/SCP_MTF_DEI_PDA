/*
 * sounds.h
 *
 *  Created on: 30 Mar 2019
 *      Author: xasin
 */

#ifndef MAIN_FX_SOUNDS_H_
#define MAIN_FX_SOUNDS_H_

#include <string>
#include "core_defs.h"

namespace LZR {
namespace Sounds {


class SoundManager {
  private:
    Xasin::Audio::TX& audioManager_;
    Xasin::Communication::CommHandler* comm_handler_;
  public:
    SoundManager(Xasin::Audio::TX& audioManager, Xasin::Communication::CommHandler* commHandler);
    void play_audio(const std::string& aName);
    void init();
};

}
}

#endif /* MAIN_FX_SOUNDS_H_ */
