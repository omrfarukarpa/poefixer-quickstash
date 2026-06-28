// File: POEFixer/plugin_sdk/PluginAbi.h
//
// Pure-C plugin ABI v6. Crossed across the host-DLL boundary; POD types only.
// Plugin authors should use PluginSDK.h (the header-only C++ wrapper) rather
// than this file directly.

#ifndef POEFIXER_PLUGIN_ABI_H
#define POEFIXER_PLUGIN_ABI_H

#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <Windows.h>  // DWORD, HWND

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PLUGIN_SDK_VERSION
#undef PLUGIN_SDK_VERSION
#endif
#define PLUGIN_SDK_VERSION 6

typedef enum {
    PSDK_ENTITY_TYPE_UNIDENTIFIED      = 0,
    PSDK_ENTITY_TYPE_CHEST             = 1,
    PSDK_ENTITY_TYPE_NPC               = 2,
    PSDK_ENTITY_TYPE_PLAYER            = 3,
    PSDK_ENTITY_TYPE_SHRINE            = 4,
    PSDK_ENTITY_TYPE_MONSTER           = 5,
    PSDK_ENTITY_TYPE_DELIRIUM_BOMB     = 6,
    PSDK_ENTITY_TYPE_DELIRIUM_SPAWNER  = 7,
    PSDK_ENTITY_TYPE_OTHER_IMPORTANT   = 8,
    PSDK_ENTITY_TYPE_ITEM              = 9,
    PSDK_ENTITY_TYPE_RENDERABLE        = 10,
    PSDK_ENTITY_TYPE_AREA_TRANSITION   = 11,
    PSDK_ENTITY_TYPE_EXPEDITION_MARKER = 12,
    PSDK_ENTITY_TYPE_EXPEDITION_REMNANT = 13,
} PsdkEntityType;

typedef enum {
    PSDK_ENTITY_SUBTYPE_UNIDENTIFIED         = 0,
    PSDK_ENTITY_SUBTYPE_NONE                 = 1,
    PSDK_ENTITY_SUBTYPE_PLAYER_SELF          = 2,
    PSDK_ENTITY_SUBTYPE_PLAYER_OTHER         = 3,
    PSDK_ENTITY_SUBTYPE_CHEST_MAGIC          = 4,
    PSDK_ENTITY_SUBTYPE_CHEST_RARE           = 5,
    PSDK_ENTITY_SUBTYPE_EXPEDITION_CHEST     = 6,
    PSDK_ENTITY_SUBTYPE_BREACH_CHEST         = 7,
    PSDK_ENTITY_SUBTYPE_STRONGBOX            = 8,
    PSDK_ENTITY_SUBTYPE_JEWELLER_STRONGBOX   = 9,
    PSDK_ENTITY_SUBTYPE_RESEARCHER_STRONGBOX = 10,
    PSDK_ENTITY_SUBTYPE_LARGE_STRONGBOX      = 11,
    PSDK_ENTITY_SUBTYPE_OMEN_CHEST           = 12,
    PSDK_ENTITY_SUBTYPE_SPECIAL_NPC          = 13,
    PSDK_ENTITY_SUBTYPE_POI_MONSTER          = 14,
    PSDK_ENTITY_SUBTYPE_PINNACLE_BOSS        = 15,
    PSDK_ENTITY_SUBTYPE_WORLD_ITEM           = 16,
    PSDK_ENTITY_SUBTYPE_INVENTORY_ITEM       = 17,
} PsdkEntitySubtype;

typedef enum {
    PSDK_ENTITY_STATE_NONE                 = 0,
    PSDK_ENTITY_STATE_USELESS              = 1,
    PSDK_ENTITY_STATE_PLAYER_LEADER        = 2,
    PSDK_ENTITY_STATE_MONSTER_FRIENDLY     = 3,
    PSDK_ENTITY_STATE_PINNACLE_BOSS_HIDDEN = 4,
} PsdkEntityState;

typedef enum {
    PSDK_NEARBY_NONE         = 0,
    PSDK_NEARBY_INNER_CIRCLE = 1,  // ~60 grid units
    PSDK_NEARBY_OUTER_CIRCLE = 2,  // ~120 grid units
    PSDK_NEARBY_FAR          = 3,
} PsdkNearbyZone;

typedef enum {
    PSDK_GAME_STATE_AREA_LOADING        = 0,
    PSDK_GAME_STATE_CHANGE_PASSWORD     = 1,
    PSDK_GAME_STATE_CREDITS             = 2,
    PSDK_GAME_STATE_ESCAPE              = 3,
    PSDK_GAME_STATE_IN_GAME             = 4,
    PSDK_GAME_STATE_PRE_GAME            = 5,
    PSDK_GAME_STATE_LOGIN               = 6,
    PSDK_GAME_STATE_WAITING             = 7,
    PSDK_GAME_STATE_CREATE_CHARACTER    = 8,
    PSDK_GAME_STATE_SELECT_CHARACTER    = 9,
    PSDK_GAME_STATE_DELETE_CHARACTER    = 10,
    PSDK_GAME_STATE_LOADING             = 11,
    PSDK_GAME_STATE_NOT_LOADED          = 12,
} PsdkGameState;

typedef enum {
    PSDK_EVENT_AREA_CHANGE   = 0,
    PSDK_EVENT_FRAME         = 1,
    PSDK_EVENT_GAME_ATTACHED = 2,
    PSDK_EVENT_GAME_DETACHED = 3,
} PsdkEventKind;

typedef enum {
    PSDK_MOD_KIND_IMPLICIT  = 0,
    PSDK_MOD_KIND_EXPLICIT  = 1,
    PSDK_MOD_KIND_ENCHANT   = 2,
    PSDK_MOD_KIND_HELLSCAPE = 3,
    PSDK_MOD_KIND_CRUCIBLE  = 4,
} PsdkModKind;

typedef enum {
    PSDK_STAT_SOURCE_ITEMS = 0,
    PSDK_STAT_SOURCE_BUFFS = 1,
} PsdkStatSource;

typedef struct {
    int32_t valid;
    int32_t current;
    int32_t total;
    int32_t reserved_flat;
    int32_t reserved_percent;
    float   regeneration;
} VitalAbi;

typedef struct {
    int32_t valid;
    int32_t _pad;
    VitalAbi health;
    VitalAbi mana;
    VitalAbi energy_shield;
    uintptr_t address;
    uintptr_t owner_address;
} LifeAbi;

typedef struct {
    int32_t valid;
    float   world_x, world_y, world_z;
    float   model_bounds_x, model_bounds_y, model_bounds_z;
    float   terrain_height;
    uintptr_t address;
    uintptr_t owner_address;
} RenderAbi;

typedef struct {
    int32_t valid;
    int32_t reaction;
    int32_t is_friendly;
    uintptr_t address;
    uintptr_t owner_address;
} PositionedAbi;

typedef struct {
    int32_t valid;
    int32_t is_targetable;
    int32_t is_highlightable;
    int32_t is_targeted_by_player;
    int32_t hidden_from_player;
    int32_t meets_quest_state;
    int32_t meets_item_requirements;
} TargetableAbi;

typedef struct {
    int32_t valid;
    int32_t is_opened;
    int32_t is_label_visible;
} ChestAbi;

typedef struct {
    int32_t valid;
    int32_t is_used;
} ShrineAbi;

typedef struct {
    int32_t valid;
    int32_t current_size;
    int32_t max_size;
} StackAbi;

typedef struct {
    int32_t valid;
    int32_t current;
    int32_t per_use_charges;
} ChargesAbi;

typedef struct {
    int32_t  valid;
    uint32_t xp;
    uint8_t  level;
    uint8_t  _pad0[3];
    uint32_t _pad1;
    uintptr_t name_addr;  // pass to MemoryServiceAbi::read_string
} PlayerAbi;

typedef struct {
    int32_t  valid;
    uint32_t id;
    uintptr_t path_addr;
} AnimatedAbi;

typedef struct {
    int32_t valid;
    int16_t current_state;
    int16_t _pad;
} TransitionableAbi;

typedef struct {
    int32_t valid;
    int32_t is_closed;
    int32_t is_blocked;
} TriggerableBlockageAbi;

typedef struct {
    int32_t valid;
    int32_t _pad;
    uintptr_t dat_row_addr;  // pass to read_string
} MinimapIconAbi;

typedef struct {
    int32_t valid;
    int32_t states_count;
    uintptr_t states_ptr;
} StateMachineAbi;

typedef struct {
    int32_t valid;
    uint8_t width;
    uint8_t height;
    uint8_t _pad[2];
    uintptr_t base_type_name_addr;
} BaseAbi;

typedef struct {
    int32_t valid;
    int32_t is_identified;
    int32_t is_corrupted;
    int32_t is_split;
    int32_t is_mirrored;
    int32_t is_relic;
    int32_t is_synthesised;
    int32_t rarity;
    int32_t item_level;
    int32_t required_level;
    int32_t crafted_mod_count;
    // Mods list walked via ComponentsServiceAbi::enumerate_item_mods.
} ModsAbi;

// Per-entity item-mod aggregate summary. Filled by
// InventoryServiceAbi::read_item_mods_summary; per-kind mod lists walked
// separately via enumerate_item_mods_by_entity.
typedef struct {
    int32_t rarity;
    int32_t item_level;
    int32_t required_level;
    int32_t crafted_mod_count;
    int32_t is_identified;
    int32_t is_corrupted;
    int32_t is_split;
    int32_t is_mirrored;
    int32_t is_relic;
    int32_t is_synthesised;
    int32_t valid;
} ItemModsSummaryAbi;

typedef struct {
    int32_t valid;
    int32_t current_weapon_index;
    int32_t is_shapeshifted;
    // Stats walked via ComponentsServiceAbi::enumerate_stats.
} StatsAbi;

typedef struct {
    int32_t valid;
    int32_t animation_id;
    uintptr_t animation_name_addr;
    // Active skills walked via ComponentsServiceAbi::enumerate_active_skills.
} ActorAbi;

typedef struct {
    int32_t valid;
    int32_t _pad;
    uintptr_t entity_owner_addr;
} NpcAbi;

typedef struct {
    int32_t valid;
    int32_t _pad;
    uintptr_t entity_owner_addr;
} DiesAfterTimeAbi;

typedef struct {
    int32_t valid;
    // Buffs walked via ComponentsServiceAbi::enumerate_buffs.
} BuffsAbi;

typedef struct {
    // === existing v6 fields — DO NOT REORDER ===
    int32_t  current_size;
    int32_t  total_uses;
    int32_t  use_stage;
    int32_t  cast_type;
    int32_t  total_cooldown_ms;
    int32_t  can_be_used;          // semantics fix lands with Bridge_Components.cpp rewrite
    uintptr_t name_addr;
    // === ActiveSkillAbi APPEND-ONLY extensions (added 2026-05-23) ===
    // SAFE because host allocates this struct and writes it; older plugin DLLs
    // compiled against the pre-extension layout just read the first 7 fields
    // and ignore the trailing bytes. Never reorder; never insert above this
    // marker. New fields go below, before the END marker.
    int32_t  max_uses;                                   // 0 if not cooldown-bound
    int32_t  total_active_cooldowns;                     // currently-running CD slots
    uint32_t equipment_info_packed;                      // raw skill.UnknownIdAndEquipmentInfo @0x10
    int32_t  _pad;                                       // align to 8
    uintptr_t granted_effects_per_level_addr;            // @0x18 of skill
    uintptr_t active_skills_dat_addr;                    // @0x20 of skill
    uintptr_t granted_effect_stat_sets_per_level_addr;   // @0x30 of skill
    uintptr_t skill_details_addr;                        // raw 0x100-byte ActiveSkillDetails base
    // === END ActiveSkillAbi extensions ===
} ActiveSkillAbi;

typedef struct {
    float    total_time;
    float    time_left;
    int16_t  charges;
    int16_t  flask_slot;
    int16_t  effectiveness;
    int16_t  _pad;
    uint32_t source_entity_id;
    uint32_t _pad1;
    uintptr_t name_addr;
} BuffAbi;

// Per-entity table of component addresses. Zero means "this entity does not
// carry that component". world_item and area_transition are entity-type
// markers rather than real components.
typedef struct {
    uintptr_t render, positioned, life, targetable;
    uintptr_t chest, shrine, player, npc;
    uintptr_t buffs;
    uintptr_t world_item, area_transition, minimap_icon;
    uintptr_t stats, actor, animated, base;
    uintptr_t charges, mods, stack, transitionable;
    uintptr_t state_machine, dies_after_time;
    uintptr_t triggerable_blockage, omp;
} ComponentAddressesAbi;

typedef struct {
    uint32_t  id;
    int32_t   _pad0;
    uintptr_t address;
    uintptr_t entity_details_address;
    uintptr_t render_component_address;
    int32_t   entity_type;       // PsdkEntityType
    int32_t   entity_subtype;    // PsdkEntitySubtype
    int32_t   entity_state;      // PsdkEntityState
    int32_t   rarity;
    int32_t   reaction;
    int32_t   zone;              // PsdkNearbyZone
    float     grid_x, grid_y;
    float     terrain_height;
    float     world_x, world_y, world_z;
    float     model_bounds_z;
    int32_t   hp_current, hp_max;
    int32_t   es_current, es_max;
    int32_t   is_valid;
    int32_t   is_sleeping;
    int32_t   is_chest_opened;
    uintptr_t path_addr;          // pass to read_wstring
    uintptr_t player_name_addr;   // pass to read_wstring
    uintptr_t tgt_path_addr;      // pass to read_string
} EntityInfoAbi;

typedef struct {
    float center_x, center_y;
    float size_x, size_y;
    float shift_x, shift_y;
    float default_shift_x, default_shift_y;
    float zoom;
    float scale;
    int32_t is_visible;
} MapDataAbi;

typedef struct {
    int32_t  current_hp, max_hp, hp_percent;
    int32_t  current_es, max_es, es_percent;
    int32_t  current_mp, max_mp, mp_percent;
    int32_t  is_town_or_hideout;
    int32_t  is_paused;
    int32_t  is_valid;
} VitalsAbi;

typedef struct {
    int32_t game_state;          // PsdkGameState
    int32_t current_area_level;
    int32_t is_town;
    int32_t is_hideout;
    int32_t is_paused;
    int32_t is_skill_tree_visible;
    int32_t is_attached;
    int32_t is_window_valid;
    int32_t game_window_foreground;
    int32_t screen_width;
    int32_t screen_height;
    float   world_to_grid_convertor;
    uint64_t last_update_time;
    uint64_t area_change_counter;
    DWORD   process_id;
    uint32_t _pad_dword;
    HWND    game_window;
    uintptr_t area_name_addr;     // pass to read_string
    uintptr_t area_hash_addr;     // pass to read_string
    EntityInfoAbi player;
    ComponentAddressesAbi player_components;
    MapDataAbi large_map;
    MapDataAbi mini_map;
    VitalsAbi  vitals;
    // Entities/inventories/buffs delivered via EntitiesService/InventoryService
    // /ComponentsService enumeration callbacks (not packed inline).
    float world_to_screen_matrix[16];  // row-major XMFLOAT4X4
} SnapshotAbi;

typedef struct {
    uintptr_t address;
    int32_t   slot_x;
    int32_t   slot_y;
    int32_t   width;
    int32_t   height;
    int32_t   stack_count;
    int32_t   rarity;
    int32_t   item_level;
    int32_t   required_level;
    int32_t   is_identified;
    int32_t   is_corrupted;
    int32_t   is_currency;
    int32_t   crafted_mod_count;
    uintptr_t path_addr;
    uintptr_t base_type_name_addr;
    uintptr_t unique_name_addr;
} InventoryItemAbi;

typedef struct {
    int32_t   valid;                // 0 = slot empty / not in game; 1 = filled
    uintptr_t entity_address;       // for ReadCharges/EnumerateItemMods follow-ups
    int32_t   slot_index;           // 0..1; canonical: 0=life, 1=mana
    int32_t   charges_current;
    int32_t   per_use_base;         // raw from Charges component
    int32_t   per_use_effective;    // base * (100 + sum_mods%) / 100  [stats% deferred]
    int32_t   usable;               // bool
    int32_t   active;               // bool, from Buffs.FlaskActive
    int32_t   is_life;              // bool
    int32_t   is_mana;              // bool
    int32_t   slot_x;
    int32_t   slot_y;
    int32_t   mod_count;            // hint; full list via Components / Inventory API
    uintptr_t name_addr;            // host-owned; pass to read_string
    uintptr_t base_type_addr;       // host-owned
    uintptr_t path_addr;            // host-owned
} FlaskAbi;

typedef struct {
    int32_t   valid;
    uintptr_t entity_address;
    int32_t   slot_index;           // 0..2
    int32_t   charges_current;      // 0 if no Charges component
    int32_t   per_use_base;         // 0 if no Charges component
    int32_t   active;               // bool, from Buffs.FlaskActive[2..4]
    int32_t   slot_x;
    int32_t   slot_y;
    int32_t   mod_count;
    uintptr_t name_addr;
    uintptr_t base_type_addr;
    uintptr_t path_addr;
} CharmAbi;

typedef struct {
    int32_t   inventory_id;
    int32_t   total_boxes_x;
    int32_t   total_boxes_y;
    int32_t   server_request_counter;
    uintptr_t address;
    float     grid_screen_x, grid_screen_y;
    float     cell_size;
    int32_t   grid_valid;
    // Items walked via InventoryServiceAbi::enumerate_items(inventory_id, cb).
} InventoryAbi;

typedef struct {
    int32_t generation_type;  // 1=Prefix, 2=Suffix, 3=Implicit
    float   value0;
    float   value1;
    uintptr_t name_addr;
    uintptr_t stat_key_addr;
    uintptr_t affix_name_addr;
} ModAbi;

typedef struct {
    uintptr_t parent_addr;
    int32_t   child_count;
    float     relative_x, relative_y;
    float     position_mod_x, position_mod_y;
    float     unscaled_width, unscaled_height;
    float     local_scale_multiplier;
    uint32_t  flags;
    uint16_t  element_type;
    uint8_t   scale_index;
    int32_t   is_visible;
    int32_t   has_position_modifier;
    uintptr_t string_id_addr;
    int32_t   valid;
} UiElementAbi;

typedef struct {
    float center_x, center_y;
    float scale_x, scale_y;  // pre-multiplied cos/sin
    float player_grid_x, player_grid_y;
    int32_t is_visible;
} MapTransformAbi;

typedef struct {
    int32_t tile_x;
    int32_t tile_y;
    float   x;
    float   y;
    uintptr_t path_addr;
} TgtLocationAbi;

// Grid handles own a snapshot of grid data. The caller must invoke
// `release(opaque)` (or destruct the C++ RAII wrapper) before any subsequent
// snapshot — re-pinning across area changes is undefined.
typedef struct {
    const uint8_t* data;
    int32_t width;
    int32_t height;
    void* opaque;
    void (*release)(void*);
} WalkableGridHandleAbi;

typedef struct {
    const float* data;
    int32_t width;
    int32_t height;
    void* opaque;
    void (*release)(void*);
} HeightGridHandleAbi;

typedef int32_t (*PsdkEntityVisitorFn)(const EntityInfoAbi* e,
                                         const ComponentAddressesAbi* comps,
                                         void* userdata);
typedef int32_t (*PsdkInventoryVisitorFn)(const InventoryAbi* inv, void* userdata);
typedef int32_t (*PsdkInventoryItemVisitorFn)(const InventoryItemAbi* item, void* userdata);
typedef int32_t (*PsdkModVisitorFn)(const ModAbi* mod, PsdkModKind mod_kind, void* userdata);
typedef int32_t (*PsdkBuffVisitorFn)(const BuffAbi* buff, void* userdata);
typedef int32_t (*PsdkActiveSkillVisitorFn)(const ActiveSkillAbi* s, void* userdata);
typedef int32_t (*PsdkStatVisitorFn)(int32_t key, int32_t value, PsdkStatSource source_kind, void* userdata);
typedef int32_t (*PsdkTgtVisitorFn)(const TgtLocationAbi* loc, void* userdata);
typedef int32_t (*PsdkFlaskVisitorFn)(const FlaskAbi* item, void* userdata);
typedef int32_t (*PsdkCharmVisitorFn)(const CharmAbi* item, void* userdata);
typedef int32_t (*PsdkUiChildVisitorFn)(uintptr_t child_addr, int32_t index, void* userdata);
typedef void    (*PsdkEventCallbackFn)(void* userdata);

typedef struct {
    void  (*get_snapshot)(SnapshotAbi* out);
    int32_t (*get_state)(void);
    int32_t (*is_attached)(void);
    int32_t (*is_in_game)(void);
    int32_t (*is_foreground)(void);
    int32_t (*is_menu_visible)(void);
    int32_t (*is_overlay_mode)(void);
    DWORD (*get_process_id)(void);
    HWND  (*get_game_window)(void);
    void  (*get_screen_size)(float* out_w, float* out_h);
} GameServiceAbi;

typedef struct {
    void    (*enumerate)(PsdkEntityVisitorFn cb, void* userdata);
    int32_t (*find_by_id)(uint32_t id, EntityInfoAbi* out_e,
                            ComponentAddressesAbi* out_c);
    void    (*get_player)(EntityInfoAbi* out_e, ComponentAddressesAbi* out_c);
    void    (*watch)(uint32_t id);
    void    (*unwatch)(uint32_t id);
    int32_t (*is_watched)(uint32_t id);
    int32_t (*get_watched_components)(uint32_t id,
                                        ComponentAddressesAbi* out);
} EntitiesServiceAbi;

typedef struct {
    int32_t (*read_life)(uintptr_t addr, LifeAbi* out);
    int32_t (*read_render)(uintptr_t addr, RenderAbi* out);
    int32_t (*read_positioned)(uintptr_t addr, PositionedAbi* out);
    int32_t (*read_targetable)(uintptr_t addr, TargetableAbi* out);
    int32_t (*read_chest)(uintptr_t addr, ChestAbi* out);
    int32_t (*read_shrine)(uintptr_t addr, ShrineAbi* out);
    int32_t (*read_stack)(uintptr_t addr, StackAbi* out);
    int32_t (*read_charges)(uintptr_t addr, ChargesAbi* out);
    int32_t (*read_player)(uintptr_t addr, PlayerAbi* out);
    int32_t (*read_animated)(uintptr_t addr, AnimatedAbi* out);
    int32_t (*read_transitionable)(uintptr_t addr, TransitionableAbi* out);
    int32_t (*read_triggerable_blockage)(uintptr_t addr, TriggerableBlockageAbi* out);
    int32_t (*read_minimap_icon)(uintptr_t addr, MinimapIconAbi* out);
    int32_t (*read_state_machine)(uintptr_t addr, StateMachineAbi* out);
    int32_t (*read_base)(uintptr_t addr, BaseAbi* out);
    int32_t (*read_mods)(uintptr_t addr, ModsAbi* out);
    int32_t (*read_stats)(uintptr_t addr, StatsAbi* out);
    int32_t (*read_buffs)(uintptr_t addr, BuffsAbi* out);
    int32_t (*read_actor)(uintptr_t addr, ActorAbi* out);
    int32_t (*read_npc)(uintptr_t addr, NpcAbi* out);
    int32_t (*read_dies_after_time)(uintptr_t addr, DiesAfterTimeAbi* out);

    void (*enumerate_buffs)(uintptr_t buffs_addr, PsdkBuffVisitorFn cb, void* ud);
    void (*enumerate_active_skills)(uintptr_t actor_addr,
                                       PsdkActiveSkillVisitorFn cb, void* ud);
    void (*enumerate_stats)(uintptr_t stats_addr, PsdkStatVisitorFn cb, void* ud);
    void (*enumerate_item_mods)(uintptr_t mods_addr, PsdkModVisitorFn cb, void* ud);
} ComponentsServiceAbi;

typedef struct {
    void    (*scan)(int32_t inventory_id);
    int32_t (*get)(int32_t inventory_id, InventoryAbi* out);
    void    (*enumerate)(PsdkInventoryVisitorFn cb, void* ud);
    void    (*enumerate_items)(int32_t inventory_id,
                                  PsdkInventoryItemVisitorFn cb, void* ud);
    const char* (*get_name)(int32_t inventory_id);
    int32_t (*read_item_rarity)(uintptr_t entity_addr);
    int32_t (*read_item_stack_count)(uintptr_t entity_addr);
    // String fill-buffer functions: pass buf=NULL/bufsize=0 to query the
    // required byte count (including the trailing null), then call again
    // with a sized buffer.
    size_t  (*read_item_base_type_name)(uintptr_t entity_addr,
                                          char* buf, size_t bufsize);
    size_t  (*read_item_unique_name)(uintptr_t entity_addr,
                                       char* buf, size_t bufsize);
    size_t  (*read_item_path)(uintptr_t entity_addr,
                                char* buf, size_t bufsize);
    // Per-entity item-mod aggregate: summary fills the flags+rarity half;
    // enumerate_item_mods_by_entity walks the per-kind mod lists.
    int32_t (*read_item_mods_summary)(uintptr_t entity_addr,
                                        ItemModsSummaryAbi* out);
    void    (*enumerate_item_mods_by_entity)(uintptr_t entity_addr,
                                               PsdkModVisitorFn cb, void* ud);
} InventoryServiceAbi;

typedef struct {
    int32_t (*read)(uintptr_t addr, UiElementAbi* out);
    void    (*enumerate_children)(uintptr_t addr, PsdkUiChildVisitorFn cb,
                                     void* ud);
    uintptr_t (*get_child_at)(uintptr_t addr, int32_t index);
    uintptr_t (*follow_path)(uintptr_t root, const int32_t* indices,
                                int32_t count);
    int32_t (*is_visible)(uintptr_t addr);
    size_t  (*get_string_id)(uintptr_t addr, char* buf, size_t bufsize);
    size_t  (*get_text)(uintptr_t addr, char* buf, size_t bufsize);
    int32_t (*compute_screen_rect)(uintptr_t addr, float* x, float* y,
                                       float* w, float* h);
    uintptr_t (*get_game_ui_root)(void);
    uintptr_t (*get_ui_root)(void);
    int32_t (*get_cull_value)(void);
    uintptr_t (*find_panel_by_string_id)(uintptr_t parent, const char* string_id);
} UiServiceAbi;

typedef struct {
    int32_t (*world_to_screen)(float wx, float wy, float wz,
                                  float* sx, float* sy);
    int32_t (*grid_to_large_map)(float gx, float gy, float world_z,
                                    float* sx, float* sy);
    int32_t (*grid_to_mini_map)(float gx, float gy, float world_z,
                                   float* sx, float* sy);
    void    (*get_large_map_transform)(MapTransformAbi* out);
    void    (*get_mini_map_transform)(MapTransformAbi* out);
} RenderServiceAbi;

typedef struct {
    void  (*get_walkable_grid)(WalkableGridHandleAbi* out);
    void  (*get_height_grid)(HeightGridHandleAbi* out);
    int32_t (*is_walkable)(int32_t gx, int32_t gy);
    float (*get_terrain_height)(int32_t gx, int32_t gy);
    float (*get_world_to_grid_convertor)(void);
    void  (*enumerate_tgt_locations)(PsdkTgtVisitorFn cb, void* ud);
} TerrainServiceAbi;

typedef struct {
    int32_t (*read)(uintptr_t addr, void* buf, size_t size);
    size_t  (*read_string)(uintptr_t addr, char* buf, size_t bufsize);
    size_t  (*read_wstring)(uintptr_t addr, wchar_t* buf, size_t bufsize);
    size_t  (*read_std_wstring)(uintptr_t container_addr,
                                  wchar_t* buf, size_t bufsize);
    int32_t (*read_std_vector)(uintptr_t container_addr, int32_t element_size,
                                   void* out_buf, int32_t* in_out_count);
    uintptr_t (*get_base_address)(void);
    uintptr_t (*get_module_size)(void);
    uintptr_t (*get_pattern_address)(const char* pattern_name);
} MemoryServiceAbi;

typedef struct {
    void (*log)(const char* level, const char* message);
} LogServiceAbi;

typedef struct {
    uint64_t (*subscribe)(PsdkEventKind event_kind, PsdkEventCallbackFn cb,
                              void* userdata);
    void     (*unsubscribe)(uint64_t token);
} EventsServiceAbi;

// =============================================================================
// OverlayServiceAbi (v6 EXTENSION — APPEND-ONLY, added 2026-05-26)
//
// Per-plugin "request flags" that influence host-side aggregate state.
// `plugin_token` is the plugin's `this` pointer (or any stable per-instance
// pointer); the host uses it to track which plugin made which request and
// auto-clears all of a plugin's flags on Disable/Unload. Multiple plugins'
// flags + the host's built-in needs are OR-aggregated.
//
// Safe from any thread. Bridge is SEH-wrapped and tolerant of NULL token.
//
// Full per-function documentation: see `OverlayService` in PluginSDK.h.
// =============================================================================
typedef struct {
    // Opt this plugin into receiving entities with EntityState Useless
    // (a.k.a. "sleeping" in host terminology) in EntitiesService::enumerate.
    // NOTE: distinct from the per-entity EntityInfoAbi::is_sleeping flag,
    // which specifically marks entities that came from the host's separate
    // SleepingEntities collection. This gate is the broader Useless filter
    // plus the SleepingEntities-collection inclusion in one toggle, matching
    // the host's GameClient::SetIncludeSleepingEntities semantics.
    // Off by default (saves ~5-15% CPU/frame). Use for map-pickers and
    // debug tools that need the full entity pool.
    void (*set_include_sleeping_entities)(void* plugin_token, int32_t enable);

    // Request that the overlay window receives mouse clicks instead of
    // passing them through to the game (i.e., temporarily disable
    // WS_EX_TRANSPARENT while cursor is over your ImGui windows). Use when
    // your plugin opens a popup or implements a map-picker flow. Keyboard is
    // unaffected. See PluginSDK.h for nuances about background draw lists
    // and InvisibleButton-based hit-testing.
    void (*set_wants_overlay_input)(void* plugin_token, int32_t enable);
} OverlayServiceAbi;

// =============================================================================
// FlasksServiceAbi (v6 EXTENSION — APPEND-ONLY, added 2026-05-26)
//
// First-class access to the utility belt (Inventory[12]): 2 flasks (life/mana)
// and 3 charms in canonical slot order. Data is precomputed by the host's
// worker thread each tick from `Inventories[12]` items + player Buffs/Stats.
// Empty slots return Valid=false. See PluginSDK.h for the C++ wrapper.
// =============================================================================
typedef struct {
    // Returns 1 on success (out filled — possibly with valid=0 for empty slot),
    //         0 on out-of-range, not in game, or null out.
    int32_t (*get_flask)(int32_t slot, FlaskAbi* out);
    int32_t (*get_charm)(int32_t slot, CharmAbi* out);

    // Iterate ALL slots including empty ones (valid=0). Return 0 from cb to stop.
    void (*enumerate_flasks)(PsdkFlaskVisitorFn cb, void* userdata);
    void (*enumerate_charms)(PsdkCharmVisitorFn cb, void* userdata);

    // POE2 today returns 2/3. Future-proof in case GGG changes the belt.
    int32_t (*flask_slot_count)(void);
    int32_t (*charm_slot_count)(void);
} FlasksServiceAbi;

typedef struct HostAbi {
    uint32_t              version;     // = PLUGIN_SDK_VERSION (6)
    uint32_t              size_bytes;  // = sizeof(HostAbi)
    GameServiceAbi        game;
    EntitiesServiceAbi    entities;
    ComponentsServiceAbi  components;
    InventoryServiceAbi   inventory;
    UiServiceAbi          ui;
    RenderServiceAbi      render;
    TerrainServiceAbi     terrain;
    MemoryServiceAbi      memory;
    LogServiceAbi         log;
    EventsServiceAbi      events;
    void*                 imgui_context;
    void*                 d3d_device;

    // === v6 EXTENSIONS — APPEND ONLY ===
    // New fields MUST be appended below this marker. Inserting between
    // existing HostAbi fields (above) shifts offsets and breaks binary
    // compatibility for every plugin DLL already built against the
    // current layout. To add new functionality without a version bump,
    // append at the END of HostAbi, never in the middle.

    // Resolve a WorldItem container entity into its inner item entity and
    // populate both ABI structs from a fresh memory read. Returns 1 on
    // success, 0 if `container_addr` is not a WorldItem container, the
    // inner item is not yet resolved (mid-spawn), or any read fails.
    int32_t (*get_world_item_inner)(uintptr_t container_addr,
                                    EntityInfoAbi* out_e,
                                    ComponentAddressesAbi* out_c);

    // OverlayServiceAbi — per-plugin overlay/data inclusion requests.
    // Added 2026-05-26. See OverlayServiceAbi declaration above for
    // full documentation.
    OverlayServiceAbi overlay;

    // FlasksServiceAbi — flask/charm convenience data for plugins.
    // Added 2026-05-26. See FlasksServiceAbi declaration above for details.
    FlasksServiceAbi flasks;
    // === END v6 EXTENSIONS ===
} HostAbi;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // POEFIXER_PLUGIN_ABI_H
