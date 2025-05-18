

#ifndef __LZRTAG_WEAPON_BASE_H__
#define __LZRTAG_WEAPON_BASE_H__

#include <stdint.h>

#include "handler.h"

namespace LZRTag {
namespace Weapon {

class Handler;

class BaseWeapon {
protected:
    friend Handler;
    Handler &handler;

    int32_t equip_duration;

    //! Tells the handler that this weapon wants to 
    //  reload, initiating a reload tick
    bool wants_to_reload;
    int32_t reload_duration;

    virtual bool can_shoot();
    virtual bool can_reload();

    virtual void reload_start();
    virtual void reload_tick();

    void bump_shot_tick();

    /*! @brief Run the shot process
     *  @details This function must be overwritten by the overloading class
     *      to flesh out the weapon's behaviour. The code here MUST always
     *      be blocking, as otherwise the weapon handler's main thread will
     *      loop without pause.
     *      This shot process MUST exit (after teardown procedures) once the 
     *      handler's "can_shoot()" function returns false, in order to let
     *      it handle reloading and/or weapon switching. Aside from that,
     *      the weapon code is free to block and wait for signals.
     */
    virtual void shot_process();
public:
    BaseWeapon(Handler &handler);

    virtual int32_t get_clip_ammo();
    virtual int32_t get_max_clip_ammo();
    virtual int32_t get_total_ammo();

    ammo_info_t get_ammo();

    /*! Offer this weapon to reload.
     *  Calling this function will offer this weapon to reload.
     *  Most weapons will initiate a reload cycle from this, though
     *  they can ignore the request (i.e. they are full, there is
     *  no ammo left, or they do not use ammo, such as a beam weapon).
     */
    virtual void tempt_reload();

    virtual void apply_vibration(float &vibr);
};

}
}

#endif