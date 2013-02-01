/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file magic.c
 *     magic support functions.
 * @par Purpose:
 *     Functions to magic.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     11 Mar 2010 - 12 May 2010
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "magic.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_memory.h"
#include "bflib_math.h"
#include "bflib_sound.h"

#include "player_data.h"
#include "player_instances.h"
#include "player_utils.h"
#include "dungeon_data.h"
#include "thing_list.h"
#include "game_merge.h"
#include "power_specials.h"
#include "power_hand.h"
#include "thing_objects.h"
#include "thing_effects.h"
#include "thing_stats.h"
#include "thing_physics.h"
#include "thing_shots.h"
#include "creature_control.h"
#include "creature_states.h"
#include "creature_states_lair.h"
#include "config_creature.h"
#include "config_terrain.h"
#include "config_magic.h"
#include "gui_soundmsgs.h"
#include "room_jobs.h"
#include "sounds.h"
#include "game_legacy.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
const long power_sight_close_instance_time[] = {4, 4, 5, 5, 6, 6, 7, 7, 8};

/******************************************************************************/
DLLIMPORT void _DK_magic_use_power_chicken(unsigned char plyr_idx, struct Thing *thing, long pwmodel, long stl_y, long splevel);
DLLIMPORT void _DK_magic_use_power_disease(unsigned char plyr_idx, struct Thing *thing, long pwmodel, long stl_y, long splevel);
DLLIMPORT void _DK_magic_use_power_destroy_walls(unsigned char plyr_idx, long a2, long pwmodel, long stl_y);
DLLIMPORT short _DK_magic_use_power_imp(unsigned short plyr_idx, unsigned short a2, unsigned short pwmodel);
DLLIMPORT void _DK_magic_use_power_heal(unsigned char plyr_idx, struct Thing *thing, long pwmodel, long stl_y, long splevel);
DLLIMPORT void _DK_magic_use_power_conceal(unsigned char plyr_idx, struct Thing *thing, long pwmodel, long stl_y, long splevel);
DLLIMPORT void _DK_magic_use_power_armour(unsigned char plyr_idx, struct Thing *thing, long pwmodel, long stl_y, long splevel);
DLLIMPORT void _DK_magic_use_power_speed(unsigned char plyr_idx, struct Thing *thing, long pwmodel, long stl_y, long splevel);
DLLIMPORT void _DK_magic_use_power_lightning(unsigned char plyr_idx, long a2, long pwmodel, long stl_y);
DLLIMPORT long _DK_magic_use_power_sight(unsigned char plyr_idx, long a2, long pwmodel, long stl_y);
DLLIMPORT void _DK_magic_use_power_cave_in(unsigned char plyr_idx, long a2, long pwmodel, long stl_y);
DLLIMPORT long _DK_magic_use_power_call_to_arms(unsigned char plyr_idx, long a2, long pwmodel, long stl_y, long splevel);
DLLIMPORT short _DK_magic_use_power_hand(unsigned short plyr_idx, unsigned short a2, unsigned short pwmodel, unsigned short stl_y);
DLLIMPORT short _DK_magic_use_power_slap(unsigned short plyr_idx, unsigned short a2, unsigned short pwmodel);
DLLIMPORT short _DK_magic_use_power_obey(unsigned short plridx);
DLLIMPORT long _DK_magic_use_power_armageddon(unsigned char val);
DLLIMPORT void _DK_magic_use_power_hold_audience(unsigned char idx);

DLLIMPORT long _DK_power_sight_explored(long stl_x, long stl_y, unsigned char plyr_idx);
DLLIMPORT void _DK_update_power_sight_explored(struct PlayerInfo *player);
DLLIMPORT unsigned char _DK_can_cast_spell_at_xy(unsigned char plyr_idx, unsigned char a2, unsigned char pwmodel, unsigned char stl_y, long splevel);
DLLIMPORT long _DK_can_cast_spell_on_creature(long plyr_idx, struct Thing *thing, long pwmodel);
/******************************************************************************/
/**
 * Returns if a spell can be casted on specific thing.
 * Originally was can_cast_spell_on_creature().
 * @param plyr_idx
 * @param thing
 * @param pwmodel
 * @return
 */
TbBool can_cast_spell_on_thing(PlayerNumber plyr_idx, struct Thing *thing, PowerKind pwmodel)
{
    //return _DK_can_cast_spell_on_creature(plyr_idx, thing, pwmodel);
    // Picked up things are immune to spells
    if (thing_is_picked_up(thing))
        return false;
    struct SpellData *pwrdata;
    pwrdata = get_power_data(pwmodel);
    if (power_data_is_invalid(pwrdata))
        return false;
    if (thing_is_object(thing))
    {
        if ((pwrdata->can_cast_flags & PwCast_OwnedFood) != 0)
        {
            if (thing->owner == plyr_idx) {
                if (object_is_mature_food(thing))  {
                    return true;
                }
            }
        }
        if ((pwrdata->can_cast_flags & PwCast_OwnedGold) != 0)
        {
            if (thing->owner == plyr_idx) {
                if (object_is_gold_pile(thing) || object_is_gold_hoard(thing)) {
                return true;
                }
            }
        }
        if ((pwrdata->can_cast_flags & PwCast_OwnedSpell) != 0)
        {
            if (thing->owner == plyr_idx) {
                if (thing_is_spellbook(thing))  {
                    return true;
                }
            }
        }
    }
    if (thing_is_creature(thing))
    {
        if ((pwrdata->can_cast_flags & PwCast_AllCrtrs) == PwCast_AllCrtrs)
        {
            return true;
        }
        if ((pwrdata->can_cast_flags & PwCast_OwnedCrtrs) != 0)
        {
            if (thing->owner == plyr_idx) {
                if (!creature_is_kept_in_custody_by_enemy(thing)) {
                    return true;
                }
            }
        }
        if ((pwrdata->can_cast_flags & PwCast_AlliedCrtrs) != 0)
        {
            if (players_are_mutual_allies(plyr_idx, thing->owner)) {
                if (!creature_is_kept_in_custody_by_enemy(thing)) {
                    return true;
                }
            }
        }
        if ((pwrdata->can_cast_flags & PwCast_CustodyCrtrs) != 0)
        {
            if (creature_is_kept_in_custody_by_enemy(thing)) {
                return true;
            }
        }
        if ((pwrdata->can_cast_flags & PwCast_EnemyCrtrs) != 0)
        {
            if (players_are_enemies(plyr_idx, thing->owner)) {
                return true;
            }
        }
    }
    return false;
}

void update_power_sight_explored(struct PlayerInfo *player)
{
  SYNCDBG(16,"Starting");
  _DK_update_power_sight_explored(player);
}

long power_sight_explored(long stl_x, long stl_y, unsigned char plyr_idx)
{
  return _DK_power_sight_explored(stl_x, stl_y, plyr_idx);
}

void slap_creature(struct PlayerInfo *player, struct Thing *thing)
{
  struct CreatureStats *crstat;
  struct CreatureControl *cctrl;
  struct MagicStats *magstat;
  long i;
  crstat = creature_stats_get_from_thing(thing);
  cctrl = creature_control_get_from_thing(thing);

  anger_apply_anger_to_creature(thing, crstat->annoy_slapped, 4, 1);
  if (crstat->slaps_to_kill > 0)
  {
    i = compute_creature_max_health(crstat->health,cctrl->explevel) / crstat->slaps_to_kill;
    if (i > 0)
    {
      apply_damage_to_thing(thing, i, player->id_number);
      thing->creature.health_bar_turns = 8;
    }
  }
  magstat = &game.magic_stats[PwrK_SLAP];
  i = cctrl->field_21;
  cctrl->field_21 = magstat->time;
  if (i == 0)
    cctrl->max_speed = calculate_correct_creature_maxspeed(thing);
  if (thing->active_state != CrSt_CreatureSlapCowers)
  {
      clear_creature_instance(thing);
      cctrl->active_state_bkp = thing->active_state;
      cctrl->continue_state_bkp = thing->continue_state;
      if (creature_is_sleeping(thing))
          anger_apply_anger_to_creature(thing, crstat->annoy_woken_up, 4, 1);
      external_set_thing_state(thing, CrSt_CreatureSlapCowers);
  }
  cctrl->field_B1 = 6;
  cctrl->field_27F = 18;
  play_creature_sound(thing, CrSnd_SlappedOuch, 3, 0);
}

TbBool can_cast_spell_at_xy(PlayerNumber plyr_idx, PowerKind pwmodel,
    MapSubtlCoord stl_x, MapSubtlCoord stl_y, unsigned long allow_flags)
{
    struct Map *mapblk;
    struct SlabMap *slb;
    unsigned long can_cast;
    mapblk = get_map_block_at(stl_x, stl_y);
    slb = get_slabmap_for_subtile(stl_x, stl_y);
    struct SpellData *pwrdata;
    pwrdata = get_power_data(pwmodel);
    if (power_data_is_invalid(pwrdata))
        return false;
    can_cast = pwrdata->can_cast_flags | allow_flags;
    // Allow casting only on revealed tiles (unless the spell overrides this)
    if ((can_cast & PwCast_Unrevealed) == 0)
    {
        if (!map_block_revealed(mapblk, plyr_idx))
        {
            // If it's not revealed, we may still accept revealing by SOE spell
            if ((can_cast & PwCast_RevealedTemp) == 0) {
                return false;
            } else
            if (!power_sight_explored(stl_x, stl_y, plyr_idx)) {
                return false;
            }
        }
    }
    if ((can_cast & PwCast_Anywhere) != 0)
    {
        // If allowed casting anywhere, we're done
        return true;
    }
    if ((can_cast & PwCast_NeedsDelay) != 0)
    {
        struct PlayerInfo *player;
        player = get_player(plyr_idx);
        if (game.play_gameturn <= player->field_4E3+20) {
            return false;
        }
    }
    PlayerNumber slb_owner;
    slb_owner = slabmap_owner(slb);
    if ((mapblk->flags & MapFlg_IsTall) != 0)
    {
        if ((can_cast & PwCast_Claimable) != 0)
        {
            if (((mapblk->flags & (MapFlg_IsDoor|MapFlg_IsRoom|MapFlg_Unkn01)) != 0) || (slb->kind == SlbT_ROCK))
            {
                  return false;
            }
        }
        if ((can_cast & PwCast_AllTall) == PwCast_AllTall)
        {
            // If allowed all tall slab, we're good
            return true;
        }
        if ((can_cast & PwCast_NeutrlTall) != 0)
        {
            if (slb_owner == game.neutral_player_num) {
                return true;
            }
        }
        if ((can_cast & PwCast_OwnedTall) != 0)
        {
            if (slb_owner == plyr_idx) {
                return true;
            }
        }
        if ((can_cast & PwCast_AlliedTall) != 0)
        {
            if ((slb_owner != plyr_idx) && players_are_mutual_allies(plyr_idx,slb_owner)) {
                return true;
            }
        }
        if ((can_cast & PwCast_EnemyTall) != 0)
        {
            if (players_are_enemies(plyr_idx,slb_owner)) {
                return true;
            }
        }
    } else
    {
        if ((can_cast & PwCast_Claimable) != 0)
        {
            if (slab_kind_is_liquid(slb->kind))
            {
                  return false;
            }
        }
        if ((can_cast & PwCast_AllGround) == PwCast_AllGround)
        {
            // If allowed all ground slab, we're good
            return true;
        }
        if ((can_cast & PwCast_NeutrlGround) != 0)
        {
            if (slb_owner == game.neutral_player_num) {
                return true;
            }
        }
        if ((can_cast & PwCast_OwnedGround) != 0)
        {
            if (slb_owner == plyr_idx) {
                return true;
            }
        }
        if ((can_cast & PwCast_AlliedGround) != 0)
        {
            if ((slb_owner != plyr_idx) && players_are_mutual_allies(plyr_idx,slb_owner)) {
                return true;
            }
        }
        if ((can_cast & PwCast_EnemyGround) != 0)
        {
            if (players_are_enemies(plyr_idx,slb_owner)) {
                return true;
            }
        }
    }
    return false;
}

TbBool pay_for_spell(PlayerNumber plyr_idx, PowerKind spkind, long splevel)
{
    struct Dungeon *dungeon;
    struct MagicStats *magstat;
    long price;
    long i;
    unsigned long k;
    if ((spkind < 0) || (spkind >= POWER_TYPES_COUNT))
        return false;
    if (splevel >= MAGIC_OVERCHARGE_LEVELS)
        splevel = MAGIC_OVERCHARGE_LEVELS;
    if (splevel < 0)
        splevel = 0;
    magstat = &game.magic_stats[spkind];
    switch (spkind)
    {
    case PwrK_MKDIGGER: // Special price algorithm for "create imp" spell
        dungeon = get_players_num_dungeon(plyr_idx);
        // Reduce price by count of sacrificed diggers
        k = get_players_special_digger_breed(plyr_idx);
        i = dungeon->num_active_diggers - dungeon->creature_sacrifice[k] + 1;
        if (i < 1)
          i = 1;
        price = magstat->cost[splevel]*i/2;
        break;
    default:
        price = magstat->cost[splevel];
        break;
    }
    // Try to take money
    if (take_money_from_dungeon(plyr_idx, price, 1) >= 0)
    {
        return true;
    }
    // If failed, say "you do not have enough gold"
    if (is_my_player_number(plyr_idx))
        output_message(SMsg_GoldNotEnough, 0, true);
    return false;
}

TbResult magic_use_power_armageddon(PlayerNumber plyr_idx)
{
    SYNCDBG(6,"Starting");
    _DK_magic_use_power_armageddon(plyr_idx);
    return Lb_SUCCESS;
}

TbResult magic_use_power_obey(PlayerNumber plyr_idx)
{
    struct Dungeon *dungeon;
    //return _DK_magic_use_power_obey(plyr_idx);
    dungeon = get_players_num_dungeon(plyr_idx);
    if (dungeon->must_obey_turn != 0) {
        dungeon->must_obey_turn = 0;
    } else {
        dungeon->must_obey_turn = game.play_gameturn;
        play_non_3d_sample(58);
    }
    update_speed_of_player_creatures_of_model(plyr_idx, 0);
    return Lb_SUCCESS;
}

void turn_off_sight_of_evil(long plyr_idx)
{
    struct Dungeon *dungeon;
  struct MagicStats *mgstat;
  long spl_lev,cit;
  long i,imax,k,n;
  //_DK_turn_off_sight_of_evil(plyr_idx);
  dungeon = get_players_num_dungeon(plyr_idx);
  mgstat = &(game.magic_stats[PwrK_SIGHT]);
  spl_lev = dungeon->field_5DA;
  if (spl_lev > SPELL_MAX_LEVEL)
      spl_lev = SPELL_MAX_LEVEL;
  i = game.play_gameturn - dungeon->field_5D4;
  imax = abs(mgstat->power[spl_lev]/4) >> 2;
  if (i > imax)
      i = imax;
  if (i < 0)
      i = 0;
  n = game.play_gameturn - mgstat->power[spl_lev];
  cit = power_sight_close_instance_time[spl_lev];
  k = imax / cit;
  if (k < 1) k = 1;
  dungeon->field_5D4 = n + i/k - cit;
}

TbResult magic_use_power_hold_audience(PlayerNumber plyr_idx)
{
    _DK_magic_use_power_hold_audience(plyr_idx);
    return Lb_SUCCESS;
}

TbResult magic_use_power_chicken(PlayerNumber plyr_idx, struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    //_DK_magic_use_power_chicken(plyr_idx, thing, stl_x, stl_y, splevel);
    // If this spell is already casted at that creature, do nothing
    if (thing_affected_by_spell(thing, SplK_Chicken)) {
        return Lb_OK;
    }
    // If we can't afford the spell, fail
    if (!pay_for_spell(plyr_idx, PwrK_CHICKEN, splevel)) {
        return Lb_FAIL;
    }
    // Check if the creature kind isn't affected by that spell
    if ((get_creature_model_flags(thing) & MF_NeverChickens) != 0)
    {
        return Lb_SUCCESS;
    }
    apply_spell_effect_to_thing(thing, SplK_Chicken, splevel);
    thing_play_sample(thing, 109, 100, 0, 3, 0, 2, 256);
    return Lb_SUCCESS;
}

TbResult magic_use_power_disease(PlayerNumber plyr_idx, struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    //_DK_magic_use_power_disease(a1, thing, a3, a4, a5); return Lb_OK;
    // If this spell is already casted at that creature, do nothing
    if ( thing_affected_by_spell(thing, SplK_Disease) ) {
        return Lb_OK;
    }
    // If we can't afford the spell, fail
    if (!pay_for_spell(plyr_idx, PwrK_DISEASE, splevel)) {
        return Lb_FAIL;
    }
    apply_spell_effect_to_thing(thing, SplK_Disease, splevel);
    {
        struct CreatureControl *cctrl;
        cctrl = creature_control_get_from_thing(thing);
        cctrl->field_B6 = plyr_idx;
    }
    thing_play_sample(thing, 59, 100, 0, 3, 0, 3, 256);
    return Lb_SUCCESS;
}

TbResult magic_use_power_destroy_walls(PlayerNumber plyr_idx, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    _DK_magic_use_power_destroy_walls(plyr_idx, stl_x, stl_y, splevel);

    return Lb_SUCCESS;
}

TbResult magic_use_power_time_bomb(PlayerNumber plyr_idx, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    //TODO SPELL TIMEBOMB write the spell support

    return Lb_SUCCESS;
}

TbResult magic_use_power_imp(PlayerNumber plyr_idx, MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    struct Dungeon *dungeon;
    struct Thing *thing;
    struct Thing *dnheart;
    struct Coord3d pos;
    //return _DK_magic_use_power_imp(plyr_idx, x, y);
    if (!can_cast_spell_at_xy(plyr_idx, PwrK_MKDIGGER, stl_x, stl_y, 0)
     || !i_can_allocate_free_control_structure()
     || !i_can_allocate_free_thing_structure(FTAF_FreeEffectIfNoSlots))
    {
        if (is_my_player_number(plyr_idx))
            play_non_3d_sample(119);
        return Lb_OK;
    }
    if (!pay_for_spell(plyr_idx, PwrK_MKDIGGER, 0))
    {
        return Lb_FAIL;
    }
    dungeon = get_players_num_dungeon(plyr_idx);
    dnheart = thing_get(dungeon->dnheart_idx);
    pos.x.val = get_subtile_center_pos(stl_x);
    pos.y.val = get_subtile_center_pos(stl_y);
    pos.z.val = get_floor_height_at(&pos) + (dnheart->field_58 >> 1);
    thing = create_creature(&pos, get_players_special_digger_breed(plyr_idx), plyr_idx);
    if (thing_is_invalid(thing))
    {
        ERRORLOG("There was place to create new creature, but creation failed");
        return Lb_SUCCESS;
    }
    thing->acceleration.x.val += ACTION_RANDOM(161) - 80;
    thing->acceleration.y.val += ACTION_RANDOM(161) - 80;
    thing->acceleration.z.val += 160;
    thing->field_1 |= TF1_PushdByAccel;
    thing->field_52 = 0;
    initialise_thing_state(thing, CrSt_ImpBirth);
    play_creature_sound(thing, 3, 2, 0);
    return Lb_SUCCESS;
}

TbResult magic_use_power_heal(PlayerNumber plyr_idx, struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    //_DK_magic_use_power_heal(plyr_idx, thing, stl_x, stl_y, splevel);return Lb_OK;
    // If the creature has full health, do nothing
    if (!thing_is_creature(thing)) {
        ERRORLOG("Tried to apply spell to invalid creature.");
        return Lb_FAIL;
    }
    if (get_creature_health_permil(thing) >= 1000) {
        return Lb_OK;
    }
    // If we can't afford the spell, fail
    if (!pay_for_spell(plyr_idx, PwrK_HEALCRTR, splevel)) {
        return Lb_FAIL;
    }
    // Apply spell effect
    thing_play_sample(thing, 37, 100, 0, 3, 0, 2, 256);
    apply_spell_effect_to_thing(thing, SplK_Heal, splevel);
    return Lb_SUCCESS;
}

TbResult magic_use_power_conceal(PlayerNumber plyr_idx, struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    //_DK_magic_use_power_conceal(a1, thing, a3, a4, a5);
    if (!thing_is_creature(thing)) {
        ERRORLOG("Tried to apply spell to invalid creature.");
        return Lb_FAIL;
    }
    // If this spell is already casted at that creature, do nothing
    if (thing_affected_by_spell(thing, SplK_Invisibility)) {
        return Lb_OK;
    }
    // If we can't afford the spell, fail
    if (!pay_for_spell(plyr_idx, PwrK_CONCEAL, splevel)) {
        return Lb_FAIL;
    }
    thing_play_sample(thing, 154, 100, 0, 3, 0, 2, 256);
    apply_spell_effect_to_thing(thing, SplK_Invisibility, splevel);
    return Lb_SUCCESS;
}

TbResult magic_use_power_armour(PlayerNumber plyr_idx, struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    //_DK_magic_use_power_armour(plyr_idx, thing, a3, a4, splevel);
    if (!thing_is_creature(thing)) {
        ERRORLOG("Tried to apply spell to invalid creature.");
        return Lb_FAIL;
    }
    // If this spell is already casted at that creature, do nothing
    if (thing_affected_by_spell(thing, SplK_Armour)) {
        return Lb_OK;
    }
    // If we can't afford the spell, fail
    if (!pay_for_spell(plyr_idx, PwrK_PROTECT, splevel)) {
        return Lb_FAIL;
    }
    thing_play_sample(thing, 153, 100, 0, 3, 0, 2, 256);
    apply_spell_effect_to_thing(thing, SplK_Armour, splevel);
    return Lb_SUCCESS;
}

long thing_affected_by_spell(struct Thing *thing, long spkind)
{
    struct CreatureControl *cctrl;
    struct CastedSpellData *cspell;
    long i;
    cctrl = creature_control_get_from_thing(thing);
    if (creature_control_invalid(cctrl))
    {
        ERRORLOG("Invalid creature control for thing %d",(int)thing->index);
        return 0;
    }
    for (i=0; i < CREATURE_MAX_SPELLS_CASTED_AT; i++)
    {
        cspell = &cctrl->casted_spells[i];
        if (cspell->spkind == spkind)
        {
            return cspell->field_1;
        }
    }
    return 0;
}

TbResult magic_use_power_speed(PlayerNumber plyr_idx, struct Thing *thing, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    //_DK_magic_use_power_speed(plyr_idx, thing, a3, a4, splevel);
    if (!thing_is_creature(thing)) {
        ERRORLOG("Tried to apply spell to invalid creature.");
        return Lb_FAIL;
    }
    if (thing_affected_by_spell(thing, SplK_Speed)) {
        return Lb_OK;
    }
    if (!pay_for_spell(plyr_idx, PwrK_SPEEDCRTR, splevel)) {
        return Lb_FAIL;
    }
    thing_play_sample(thing, 38, 100, 0, 3, 0, 2, 256);
    apply_spell_effect_to_thing(thing, SplK_Speed, splevel);
    return Lb_SUCCESS;
}

TbResult magic_use_power_lightning(PlayerNumber plyr_idx, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    struct PlayerInfo *player;
    struct Dungeon *dungeon;
    struct MagicStats *magstat;
    struct ShotConfigStats *shotst;
    struct Thing *shtng;
    struct Thing *obtng;
    struct Thing *efftng;
    struct Coord3d pos;
    long range,max_damage;
    long i;
    //_DK_magic_use_power_lightning(plyr_idx, stl_x, stl_y, splevel);
    player = get_player(plyr_idx);
    dungeon = get_dungeon(player->id_number);
    pos.x.val = get_subtile_center_pos(stl_x);
    pos.y.val = get_subtile_center_pos(stl_y);
    pos.z.val = 0;
    // make sure the spell level is correct
    if (splevel >= MAGIC_OVERCHARGE_LEVELS)
        splevel = MAGIC_OVERCHARGE_LEVELS-1;
    if (splevel < 0)
        splevel = 0;
    // Check if we can cast that spell
    if (!can_cast_spell_at_xy(plyr_idx, PwrK_LIGHTNING, stl_x, stl_y, 0))
    {
        // Make a rejection sound
        if (is_my_player_number(plyr_idx))
          play_non_3d_sample(119);
        return Lb_OK;
    }
    // Pay for it
    if (!pay_for_spell(plyr_idx, PwrK_LIGHTNING, splevel))
    {
        return Lb_FAIL;
    }
    // And cast it
    shtng = create_shot(&pos, 16, plyr_idx);
    if (!thing_is_invalid(shtng))
    {
        shtng->mappos.z.val = get_thing_height_at(shtng, &shtng->mappos) + 128;
        shtng->byte_16 = 2;
        shtng->field_19 = splevel;
    }
    magstat = &game.magic_stats[PwrK_LIGHTNING];
    shotst = get_shot_model_stats(16);
    dungeon->camera_deviate_jump = 256;
    i = magstat->power[splevel];
    max_damage = i * shotst->old->damage;
    range = (i << 8) / 2;
    if (power_sight_explored(stl_x, stl_y, plyr_idx))
        max_damage /= 4;
    obtng = create_object(&pos, 124, plyr_idx, -1);
    if (!thing_is_invalid(obtng))
    {
        obtng->byte_13 = splevel;
        obtng->field_4F |= 0x01;
    }
    i = electricity_affecting_area(&pos, plyr_idx, range, max_damage);
    SYNCDBG(9,"Affected %ld targets within range %ld, damage %ld",i,range,max_damage);
    if (!thing_is_invalid(shtng))
    {
        efftng = create_effect(&shtng->mappos, TngEff_Unknown49, shtng->owner);
        if (!thing_is_invalid(efftng))
        {
            thing_play_sample(efftng, 55, 100, 0, 3, 0, 2, 256);
        }
    }
    player->field_4E3 = game.play_gameturn;
    return Lb_SUCCESS;
}

TbResult magic_use_power_sight(PlayerNumber plyr_idx, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    struct MagicStats *magstat;
    struct Dungeon *dungeon;
    struct Thing *thing;
    struct Coord3d pos;
    long cit,cdt,cgt,cdlimit;
    long i;
    //return _DK_magic_use_power_sight(plyr_idx, stl_x, stl_y, splevel);
    dungeon = get_dungeon(plyr_idx);
    magstat = &game.magic_stats[PwrK_SIGHT];
    if ( dungeon->keeper_sight_thing_idx )
    {
        cdt = game.play_gameturn - dungeon->field_5D4;
        cdlimit = magstat->power[dungeon->field_5DA] >> 4;
        if (cdt < 0) {
            cdt = 0;
        } else
        if (cdt > cdlimit) {
            cdt = cdlimit;
        }
        cit = power_sight_close_instance_time[dungeon->field_5DA];
        cgt = game.play_gameturn - magstat->power[dungeon->field_5DA];
        i = cdlimit / cit;
        if (i > 0) {
            dungeon->field_5D4 = cgt + cdt/i - cit;
        } else {
            dungeon->field_5D4 = cgt;
        }
        thing = thing_get(dungeon->keeper_sight_thing_idx);
        if (cgt < (long)thing->creation_turn)
        {
            dungeon->computer_enabled |= 0x04;
            dungeon->sight_casted_stl_x = stl_x;
            dungeon->sight_casted_stl_y = stl_y;
        }
        return Lb_OK;
    }

    if (take_money_from_dungeon(plyr_idx, magstat->cost[splevel], 1) < 0)
    {
        if (is_my_player_number(plyr_idx))
            output_message(SMsg_GoldNotEnough, 0, true);
        return Lb_FAIL;
    }
    pos.x.val = (stl_x << 8) + 128;
    pos.y.val = (stl_y << 8) + 128;
    pos.z.val = (5 << 8) + 128;
    thing = create_object(&pos, 123, plyr_idx, -1);
    if (!thing_is_invalid(thing))
    {
        dungeon->field_5D4 = game.play_gameturn;
        thing->health = 2;
        dungeon->field_5DA = splevel;
        dungeon->keeper_sight_thing_idx = thing->index;
        LbMemorySet(dungeon->soe_explored_flags, 0, sizeof(dungeon->soe_explored_flags));
        thing->field_4F |= 0x01;
        thing_play_sample(thing, 51, 100, -1, 3, 0, 3, 256);
    }
    return Lb_SUCCESS;
}

TbResult magic_use_power_cave_in(PlayerNumber plyr_idx, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel)
{
    _DK_magic_use_power_cave_in(plyr_idx, stl_x, stl_y, splevel);
    return Lb_SUCCESS;
}

TbResult magic_use_power_call_to_arms(PlayerNumber plyr_idx, MapSubtlCoord stl_x, MapSubtlCoord stl_y, long splevel, unsigned long allow_flags)
{
    return _DK_magic_use_power_call_to_arms(plyr_idx, stl_x, stl_y, splevel, allow_flags);
}

TbResult magic_use_power_slap_thing(PlayerNumber plyr_idx, struct Thing *thing)
{
    struct PlayerInfo *player;
    struct Dungeon *dungeon;
    if (!thing_exists(thing)) {
        return Lb_FAIL;
    }
    player = get_player(plyr_idx);
    dungeon = get_dungeon(player->id_number);
    if ((player->instance_num == PI_Whip) || (game.play_gameturn - dungeon->field_14AE <= 10)) {
        return Lb_OK;
    }
    player->influenced_thing_idx = thing->index;
    player->influenced_thing_creation = thing->creation_turn;
    set_player_instance(player, PI_Whip, 0);
    dungeon->lvstats.num_slaps++;
    return Lb_SUCCESS;
}

TbResult magic_use_power_slap(PlayerNumber plyr_idx, MapSubtlCoord stl_x, MapSubtlCoord stl_y)
{
    struct Thing *thing;
    //return _DK_magic_use_power_slap(plyr_idx, stl_x, stl_y);
    thing = get_nearest_thing_for_slap(plyr_idx, get_subtile_center_pos(stl_x), get_subtile_center_pos(stl_y));
    return magic_use_power_slap_thing(plyr_idx, thing);
}

TbResult magic_use_power_possess_thing(PlayerNumber plyr_idx, struct Thing *thing)
{
    struct PlayerInfo *player;
    if (!thing_exists(thing)) {
        return Lb_FAIL;
    }
    player = get_player(plyr_idx);
    player->influenced_thing_idx = thing->index;
    set_player_instance(player, 5, 0);
    return Lb_SUCCESS;
}

/**
 * Unified function for using powers which are castable on things.
 *
 * @param plyr_idx The casting player.
 * @param spl_idx Power kind to be casted.
 * @param splevel Power overcharge level.
 * @param thing The target thing.
 * @param stl_x The casting subtile, X coord.
 * @param stl_y The casting subtile, Y coord.
 * @return
 */
TbResult magic_use_available_power_on_thing(PlayerNumber plyr_idx, PowerKind pwmodel,
    unsigned short splevel, MapSubtlCoord stl_x, MapSubtlCoord stl_y, struct Thing *thing)
{
    TbResult ret;
    ret = Lb_OK;
    if (!is_power_available(plyr_idx, pwmodel)) {
        // It shouldn't be possible to select unavailable spell
        WARNLOG("Player %d tried to cast %s which is unavailable",(int)plyr_idx,power_code_name(pwmodel));
        ret = Lb_FAIL;
    }
    if (!thing_exists(thing)) {
        WARNLOG("Player %d tried to cast %s on non-existing thing",(int)plyr_idx,power_code_name(pwmodel));
        ret = Lb_FAIL;
    }
    if (ret == Lb_OK)
    {
        TbBool cast_at_xy,cast_on_tng;
        cast_at_xy = can_cast_spell_at_xy(plyr_idx, pwmodel, stl_x, stl_y, 0);
        cast_on_tng = can_cast_spell_on_thing(plyr_idx, thing, pwmodel);
        struct SpellData *pwrdata;
        pwrdata = get_power_data(pwmodel);
        if ((pwrdata->can_cast_flags & PwCast_ThingOrMap) != 0)
        {
            // Fail only if both functions have failed - one is enough
            if (!cast_at_xy && !cast_on_tng) {
                WARNLOG("Player %d tried to cast %s on %s which can't be targeted",(int)plyr_idx,power_code_name(pwmodel),cast_on_tng?"a thing":cast_at_xy?"a subtile":"thing or subtile");
                ret = Lb_FAIL;
            }
        } else
        {
            // Fail if any of the functions has failed - we need both
            if (!cast_at_xy || !cast_on_tng) {
                WARNLOG("Player %d tried to cast %s on %s which can't be targeted",(int)plyr_idx,power_code_name(pwmodel),cast_on_tng?"a thing":cast_at_xy?"a subtile":"thing or subtile");
                ret = Lb_FAIL;
            }
        }
    }
    if (ret == Lb_OK)
    {
        if (splevel > MAGIC_OVERCHARGE_LEVELS) {
            WARNLOG("Overcharge level %d out of range, adjusting",(int)splevel);
            splevel = MAGIC_OVERCHARGE_LEVELS;
        }
    }
    if (ret == Lb_OK)
    {
        switch (pwmodel)
        {
        case PwrK_HAND:
            //TODO check if we should use magic_use_power_hand()
            if (place_thing_in_power_hand(thing, plyr_idx))
                ret = Lb_SUCCESS;
            else
                ret = Lb_FAIL;
            break;
        case PwrK_HEALCRTR:
            ret = magic_use_power_heal(plyr_idx, thing, stl_x, stl_y, splevel);
            break;
        case PwrK_SPEEDCRTR:
            ret = magic_use_power_speed(plyr_idx, thing, stl_x, stl_y, splevel);
            break;
        case PwrK_PROTECT:
            ret = magic_use_power_armour(plyr_idx, thing, stl_x, stl_y, splevel);
            break;
        case PwrK_CONCEAL:
            ret = magic_use_power_conceal(plyr_idx, thing, stl_x, stl_y, splevel);
            break;
        case PwrK_DISEASE:
            ret = magic_use_power_disease(plyr_idx, thing, stl_x, stl_y, splevel);
            break;
        case PwrK_CHICKEN:
            ret = magic_use_power_chicken(plyr_idx, thing, stl_x, stl_y, splevel);
            break;
        case PwrK_SLAP:
            ret = magic_use_power_slap_thing(plyr_idx, thing);
            break;
        case PwrK_POSSESS:
            ret = magic_use_power_possess_thing(plyr_idx, thing);
            break;
        case PwrK_CALL2ARMS:
            ret = magic_use_power_call_to_arms(plyr_idx, stl_x, stl_y, splevel, PwCast_None);
            break;
        case PwrK_LIGHTNING:
            ret = magic_use_power_lightning(plyr_idx, stl_x, stl_y, splevel);
            break;
        default:
            ERRORLOG("Power not supported here: %d", (int)pwmodel);
            ret = Lb_FAIL;
            break;
        }
    }
    if (ret == Lb_FAIL) {
        // Make a rejection sound
        if (is_my_player_number(plyr_idx))
            play_non_3d_sample(119);
    }
    return ret;
}

/**
 * Unified function for using powers which are castable on map subtile.
 *
 * @param plyr_idx The casting player.
 * @param spl_idx Power kind to be casted.
 * @param splevel Power overcharge level.
 * @param stl_x The target subtile, X coord.
 * @param stl_y The target subtile, Y coord.
 * @return
 */
TbResult magic_use_available_power_on_subtile(PlayerNumber plyr_idx, PowerKind spl_idx,
    unsigned short splevel, MapSubtlCoord stl_x, MapSubtlCoord stl_y, unsigned long allow_flags)
{
    if (!is_power_available(plyr_idx, spl_idx)) {
        // It shouldn't be possible to select unavailable spell
        WARNLOG("Player %d tried to cast unavailable spell %d",(int)plyr_idx,(int)spl_idx);
        return Lb_FAIL;
    }
    if (splevel > MAGIC_OVERCHARGE_LEVELS) {
        splevel = MAGIC_OVERCHARGE_LEVELS;
    }
    switch (spl_idx)
    {
    case PwrK_MKDIGGER:
        return magic_use_power_imp(plyr_idx, stl_x, stl_y);
    case PwrK_SLAP:
        return magic_use_power_slap(plyr_idx, stl_x, stl_y);
    case PwrK_SIGHT:
        return magic_use_power_sight(plyr_idx, stl_x, stl_y, splevel);
    case PwrK_CALL2ARMS:
        return magic_use_power_call_to_arms(plyr_idx, stl_x, stl_y, splevel, allow_flags);
    case PwrK_CAVEIN:
        return magic_use_power_cave_in(plyr_idx, stl_x, stl_y, splevel);
    case PwrK_LIGHTNING:
        return magic_use_power_lightning(plyr_idx, stl_x, stl_y, splevel);
    case PwrK_DESTRWALLS:
        return magic_use_power_destroy_walls(plyr_idx, stl_x, stl_y, splevel);
    case PwrK_TIMEBOMB:
        return magic_use_power_time_bomb(plyr_idx, stl_x, stl_y, splevel);
    default:
        ERRORLOG("Power not supported here: %d", (int)spl_idx);
        break;
    }
    return Lb_FAIL;
}

/**
 * Unified function for using powers which are castable without any particular target.
 *
 * @param plyr_idx The casting player.
 * @param spl_idx Power kind to be casted.
 * @param splevel Power overcharge level.
 * @return
 */
TbResult magic_use_available_power_on_level(PlayerNumber plyr_idx, PowerKind spl_idx,
    unsigned short splevel)
{
    if (!is_power_available(plyr_idx, spl_idx)) {
        // It shouldn't be possible to select unavailable spell
        WARNLOG("Player %d tried to cast unavailable spell %d",(int)plyr_idx,(int)spl_idx);
        return Lb_FAIL;
    }
    if (splevel > MAGIC_OVERCHARGE_LEVELS) {
        splevel = MAGIC_OVERCHARGE_LEVELS;
    }
    switch (spl_idx)
    {
    case PwrK_OBEY:
        return magic_use_power_obey(plyr_idx);
    case PwrK_HOLDAUDNC:
        return magic_use_power_hold_audience(plyr_idx);
    case PwrK_ARMAGEDDON:
        return magic_use_power_armageddon(plyr_idx);
    default:
        ERRORLOG("Power not supported here: %d", (int)spl_idx);
        break;
    }
    return Lb_FAIL;
}

void directly_cast_spell_on_thing(PlayerNumber plyr_idx, PowerKind spl_idx, ThingIndex thing_idx, long splevel)
{
    struct Thing *thing;
    //_DK_directly_cast_spell_on_thing(plyr_idx, spl_idx, thing_idx, splevel);
    thing = thing_get(thing_idx);
    magic_use_available_power_on_thing(plyr_idx, spl_idx, splevel,
        thing->mappos.x.stl.num, thing->mappos.y.stl.num, thing);
}

int get_power_overcharge_level(struct PlayerInfo *player)
{
    int i;
    i = (player->field_4D2 >> 2);
    if (i > SPELL_MAX_LEVEL)
        return SPELL_MAX_LEVEL;
    return i;
}

TbBool update_power_overcharge(struct PlayerInfo *player, int spl_idx)
{
  struct Dungeon *dungeon;
  struct MagicStats *mgstat;
  int i;
  if ((spl_idx < 0) || (spl_idx >= POWER_TYPES_COUNT))
      return false;
  dungeon = get_dungeon(player->id_number);
  mgstat = &(game.magic_stats[spl_idx]);
  i = (player->field_4D2+1) >> 2;
  if (i > SPELL_MAX_LEVEL)
    i = SPELL_MAX_LEVEL;
  if (mgstat->cost[i] <= dungeon->total_money_owned)
  {
    // If we have more money, increase overcharge
    player->field_4D2++;
  } else
  {
    // If we don't have money, decrease the charge
    while (mgstat->cost[i] > dungeon->total_money_owned)
    {
      i--;
      if (i < 0) break;
    }
    if (i >= 0)
      player->field_4D2 = (i << 2) + 1;
    else
      player->field_4D2 = 0;
  }
  return (i < SPELL_MAX_LEVEL);
}
/******************************************************************************/
#ifdef __cplusplus
}
#endif