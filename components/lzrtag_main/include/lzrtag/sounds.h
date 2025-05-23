/*
 * sounds.h
 *
 *  Created on: 30 Mar 2019
 *      Author: xasin
 */

#ifndef MAIN_FX_SOUNDS_H_
#define MAIN_FX_SOUNDS_H_

#include <string>
#include "driver/dac.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xasin/mqtt/Handler.h"

namespace LZR {
namespace Sounds {


class SoundManager {
  private:
    Xasin::MQTT::Handler* comm_handler_;
  public:
    SoundManager(Xasin::MQTT::Handler* commHandler);
    void play_audio(const std::string& aName);
    void init();
};

// Add audio player function declarations
esp_err_t audio_player_init(dac_channel_t dac_channel);
esp_err_t audio_player_play_file(const char* file_path, uint32_t sample_rate);
esp_err_t audio_player_stop(void);
bool audio_player_is_playing(void);
void audio_player_task(void *pvParameters);

}
}

#endif /* MAIN_FX_SOUNDS_H_ */
