// PluginAbi.h — POEFixer plugin ABI (pure C, v6).
//
// The stable binary contract between the host and plugin DLLs. Plugin authors
// normally use the C++ wrapper in PluginSDK.h and never touch this file; it is
// here for the wrapper and for non-C++ bindings.
//
// ABI rules (read before editing):
//   * POD only — no C++ types cross this boundary.
//   * Layout is frozen. To extend without breaking already-built plugins,
//     APPEND a field/function at the very END of HostAbi (a vtable the host
//     owns and the plugin only reads). The host advertises HostAbi::size_bytes
//     and plugins accept any host where size_bytes >= their own sizeof.
//   * NEVER grow a struct the host fills into a plugin-allocated buffer
//     (anything passed as `T* out`, or embedded by value inside SnapshotAbi):
//     the host writes its own sizeof and overruns older plugins. Deliver new
//     such data through a new tail HostAbi function + its own standalone struct.
//   * uintptr_t "*_addr" fields are addresses in the GAME process; read them
//     with MemoryService. "name_addr"/"path_addr" style fields are host-owned
//     strings — pass to read_string/read_wstring; valid only for the duration
//     of the call/callback that produced them.

#ifndef POEFIXER_PLUGIN_ABI_H
#define POEFIXER_PLUGIN_ABI_H

#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PLUGIN_SDK_VERSION
#undef PLUGIN_SDK_VERSION
#endif
// Bump only on a breaking (non-append) change; the host refuses a plugin whose
// version differs.
#define PLUGIN_SDK_VERSION 6

// Coarse entity classification (EntityInfoAbi::entity_type).
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

// Finer entity classification (EntityInfoAbi::entity_subtype).
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

// Special-case entity flags (EntityInfoAbi::entity_state).
typedef enum {
    PSDK_ENTITY_STATE_NONE                 = 0,
    PSDK_ENTITY_STATE_USELESS              = 1,
    PSDK_ENTITY_STATE_PLAYER_LEADER        = 2,
    PSDK_ENTITY_STATE_MONSTER_FRIENDLY     = 3,
    PSDK_ENTITY_STATE_PINNACLE_BOSS_HIDDEN = 4,
} PsdkEntityState;

// Distance band from the player (EntityInfoAbi::zone).
typedef enum {
    PSDK_NEARBY_NONE         = 0,
    PSDK_NEARBY_INNER_CIRCLE = 1,
    PSDK_NEARBY_OUTER_CIRCLE = 2,
    PSDK_NEARBY_FAR          = 3,
} PsdkNearbyZone;

// Engine state (SnapshotAbi::game_state). In-game data is valid only in IN_GAME.
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
    // PoE2 0.5.x grew a 13th engine state slot (host GameStateTypes::UnknownState).
    // Appended at the tail — NOT_LOADED stays 12 so already-compiled plugins keep
    // reading the not-loaded sentinel at its historical value (append-only ABI).
    PSDK_GAME_STATE_UNKNOWN             = 13,
} PsdkGameState;

// Event kinds for EventsService::subscribe.
typedef enum {
    PSDK_EVENT_AREA_CHANGE   = 0,
    PSDK_EVENT_FRAME         = 1,
    PSDK_EVENT_GAME_ATTACHED = 2,
    PSDK_EVENT_GAME_DETACHED = 3,
} PsdkEventKind;

// Mod list a ModAbi came from (passed to PsdkModVisitorFn).
typedef enum {
    PSDK_MOD_KIND_IMPLICIT  = 0,
    PSDK_MOD_KIND_EXPLICIT  = 1,
    PSDK_MOD_KIND_ENCHANT   = 2,
    PSDK_MOD_KIND_HELLSCAPE = 3,
    PSDK_MOD_KIND_CRUCIBLE  = 4,
} PsdkModKind;

// Origin of an enumerated stat (passed to PsdkStatVisitorFn).
typedef enum {
    PSDK_STAT_SOURCE_ITEMS = 0,
    PSDK_STAT_SOURCE_BUFFS = 1,
} PsdkStatSource;

// ---------------------------------------------------------------------------
// Component PODs. Each is filled by the matching ComponentsService::read_* from
// a component address (see ComponentAddressesAbi). valid==0 means the read
// failed or the entity lacks that component.
// ---------------------------------------------------------------------------

// A single resource pool (life/mana/ES).
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

// World position + model bounds. terrain_height is rounded to 4 decimals.
typedef struct {
    int32_t valid;
    float   world_x, world_y, world_z;
    float   model_bounds_x, model_bounds_y, model_bounds_z;
    float   terrain_height;
    uintptr_t address;
    uintptr_t owner_address;
} RenderAbi;

// reaction: raw faction id; is_friendly is the derived convenience flag.
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

// Stackable count (current_size of max_size).
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

// name_addr is a host-owned string (read_string).
typedef struct {
    int32_t  valid;
    uint32_t xp;
    uint8_t  level;
    uint8_t  _pad0[3];
    uint32_t _pad1;
    uintptr_t name_addr;
} PlayerAbi;

// path_addr is a host-owned string (read_string).
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

// dat_row_addr -> the icon's MinimapIcons.dat row (read_string for its path).
typedef struct {
    int32_t valid;
    int32_t _pad;
    uintptr_t dat_row_addr;
} MinimapIconAbi;

typedef struct {
    int32_t valid;
    int32_t states_count;
    uintptr_t states_ptr;
} StateMachineAbi;

// width/height are the item's grid footprint; base_type_name_addr is host-owned.
typedef struct {
    int32_t valid;
    uint8_t width;
    uint8_t height;
    uint8_t _pad[2];
    uintptr_t base_type_name_addr;
} BaseAbi;

// Item flags + rarity. Walk the mod text via enumerate_item_mods.
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
} ModsAbi;

// Per-entity item-mod summary (InventoryService::read_item_mods_summary).
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

// Stat values are walked via enumerate_stats.
typedef struct {
    int32_t valid;
    int32_t current_weapon_index;
    int32_t is_shapeshifted;
} StatsAbi;

// Animation only. Current-action flags/target are delivered separately via
// read_actor_action (see ActorActionAbi) so this struct can stay frozen.
typedef struct {
    int32_t valid;
    int32_t animation_id;
    uintptr_t animation_name_addr;
} ActorAbi;

// A unit's movement route: grid waypoints, node_count valid entries.
// Filled by HostAbi::read_pathfinding (keyed by entity address).
typedef struct {
    int32_t valid;
    int32_t node_count;
    struct { int32_t x, y; } nodes[64];
} PathfindingAbi;

// A unit's current action: flags bitmask (bit 0x400 = using ability) + target
// grid cell. Filled by HostAbi::read_actor_action from an Actor address.
typedef struct {
    int32_t valid;
    int32_t flags;
    int32_t dest_x;
    int32_t dest_y;
} ActorActionAbi;

// A ground effect (the "GroundEffect" component on a
// Metadata/Effects/Spells/ground_effects/VisibleServerGroundEffect entity).
// Filled by HostAbi::read_ground_effect from an ENTITY address — the host
// resolves the component and its groundeffects.datc64 row. Lets a plugin tell
// apart the many same-pathed ground effects (Shocked/Ignited/Caustic/...).
// The *_addr fields are GAME-process addresses of raw UTF-16 strings: resolve
// with MemoryService::read_wstring (FetchWString); 0 means that column is unset
// for this variant. The two *_row fields are raw dat-row pointers (valid for the
// session) for advanced cross-referencing.
typedef struct {
    int32_t   valid;
    float     radius;                  // world units (e.g. 190.0); 0 = unset by this variant
    uintptr_t type_id_addr;            // groundeffecttypes.Id — the stable key, e.g. "ShockedGround"
    uintptr_t end_effect_addr;         // end behaviour: "fadeout" / "close" / "end"
    uintptr_t buff_visual1_addr;       // buffvisuals.Id, e.g. "ground_fire_burn_white" (0 if unset)
    uintptr_t buff_visual2_addr;       // buffdefinitions.Name, e.g. "ground_tar_gold" (0 if unset)
    uintptr_t ao_file_addr;            // first .ao/.aoc visual path (0 if none)
    uintptr_t ground_effects_row;      // raw groundeffects.datc64 row pointer
    uintptr_t ground_effect_types_row; // raw groundeffecttypes.datc64 row pointer
} GroundEffectAbi;

// One monster modifier, read from an ObjectMagicProperties component
// (HostAbi::enumerate_monster_mods). Lets a plugin identify a monster's rolled
// mods the instant it spawns — before any related buff is applied. The three
// *_addr fields are host-owned strings (read via MemoryService / FetchString,
// valid only for the duration of the visitor call); metadata_addr is empty for
// non-monster mods. id/hashes map to the Mods.dat Id/HASH16/HASH32 columns.
typedef struct {
    uintptr_t id_addr;            // Mods.dat Id, e.g. "MonsterAbyssLightlessFaction1"
    uintptr_t display_name_addr;  // Mods.dat Name (display), e.g. "Abyssal"
    uintptr_t metadata_addr;      // Mods.dat MonsterMetadata, e.g. "Metadata/.../LightlessWells"
    uint32_t  hash32;             // Mods.dat HASH32, e.g. 0xBFDA2A36
    uint16_t  hash16;             // Mods.dat HASH16, e.g. 0x63D1
    int16_t   generation_type;    // 1=Prefix 2=Suffix 3=Implicit
} MonsterModAbi;

typedef int32_t (*PsdkMonsterModVisitorFn)(const MonsterModAbi* mod, void* userdata);

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

// Presence marker; walk the actual buffs via enumerate_buffs.
typedef struct {
    int32_t valid;
} BuffsAbi;

// One active/usable skill. Leading fields are v6 original; the trailing block
// (max_uses onward) is an append from 2026-05-23. name_addr is host-owned.
typedef struct {
    int32_t  current_size;
    int32_t  total_uses;
    int32_t  use_stage;
    int32_t  cast_type;
    int32_t  total_cooldown_ms;
    int32_t  can_be_used;
    uintptr_t name_addr;
    int32_t  max_uses;
    int32_t  total_active_cooldowns;
    uint32_t equipment_info_packed;
    int32_t  _pad;
    uintptr_t granted_effects_per_level_addr;
    uintptr_t active_skills_dat_addr;
    uintptr_t granted_effect_stat_sets_per_level_addr;
    uintptr_t skill_details_addr;
} ActiveSkillAbi;

// One active buff/debuff (enumerate_buffs). name_addr is host-owned.
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

// Per-entity component addresses; 0 = entity does not have that component.
// Pass a non-zero field to the matching ComponentsService::read_* call.
// FROZEN LAYOUT: embedded by value in SnapshotAbi and filled into plugin
// buffers — do not add fields (see the ABI rules at the top of this file).
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

// One entity. grid_x/y are cell coords; world_* are render coords. The three
// *_addr fields are host-owned strings. address is the entity base (pass to
// read_pathfinding / read_actor_action and component reads via the comps table).
typedef struct {
    uint32_t  id;
    int32_t   _pad0;
    uintptr_t address;
    uintptr_t entity_details_address;
    uintptr_t render_component_address;
    int32_t   entity_type;
    int32_t   entity_subtype;
    int32_t   entity_state;
    int32_t   rarity;
    int32_t   reaction;
    int32_t   zone;
    float     grid_x, grid_y;
    float     terrain_height;
    float     world_x, world_y, world_z;
    float     model_bounds_z;
    int32_t   hp_current, hp_max;
    int32_t   es_current, es_max;
    int32_t   is_valid;
    int32_t   is_sleeping;
    int32_t   is_chest_opened;
    uintptr_t path_addr;
    uintptr_t player_name_addr;
    uintptr_t tgt_path_addr;
} EntityInfoAbi;

// Large/minimap transform for placing world points on the map.
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

// One-shot world snapshot (GameService::get_snapshot). Entities, inventories
// and buffs are NOT inline here — enumerate them via their services. world_to_
// screen_matrix is row-major. FROZEN LAYOUT (filled into a plugin buffer).
typedef struct {
    int32_t game_state;
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
    uintptr_t area_name_addr;
    uintptr_t area_hash_addr;
    EntityInfoAbi player;
    ComponentAddressesAbi player_components;
    MapDataAbi large_map;
    MapDataAbi mini_map;
    VitalsAbi  vitals;
    float world_to_screen_matrix[16];
} SnapshotAbi;

// One inventory cell (InventoryService::enumerate_items). slot_x/y are grid
// coords. screen_* is the on-screen rect for special tabs whose cells aren't a
// uniform grid; when screen_valid==0 use grid math (grid_screen + slot*cell).
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
    float     screen_x;
    float     screen_y;
    float     screen_w;
    float     screen_h;
    int32_t   screen_valid;
} InventoryItemAbi;

// A belt flask slot (FlasksService). valid==0 for an empty slot.
typedef struct {
    int32_t   valid;
    uintptr_t entity_address;
    int32_t   slot_index;
    int32_t   charges_current;
    int32_t   per_use_base;
    int32_t   per_use_effective;
    int32_t   usable;
    int32_t   active;
    int32_t   is_life;
    int32_t   is_mana;
    int32_t   slot_x;
    int32_t   slot_y;
    int32_t   mod_count;
    uintptr_t name_addr;
    uintptr_t base_type_addr;
    uintptr_t path_addr;
} FlaskAbi;

// A belt charm slot (FlasksService). valid==0 for an empty slot.
typedef struct {
    int32_t   valid;
    uintptr_t entity_address;
    int32_t   slot_index;
    int32_t   charges_current;
    int32_t   per_use_base;
    int32_t   active;
    int32_t   slot_x;
    int32_t   slot_y;
    int32_t   mod_count;
    uintptr_t name_addr;
    uintptr_t base_type_addr;
    uintptr_t path_addr;
} CharmAbi;

// Inventory metadata (InventoryService::get). Items via enumerate_items.
typedef struct {
    int32_t   inventory_id;
    int32_t   total_boxes_x;
    int32_t   total_boxes_y;
    int32_t   server_request_counter;
    uintptr_t address;
    float     grid_screen_x, grid_screen_y;
    float     cell_size;
    int32_t   grid_valid;
} InventoryAbi;

// One mod (enumerate_item_mods). generation_type: 1=prefix 2=suffix 3=implicit.
// The *_addr fields are host-owned strings.
typedef struct {
    int32_t generation_type;
    float   value0;
    float   value1;
    uintptr_t name_addr;
    uintptr_t stat_key_addr;
    uintptr_t affix_name_addr;
    // --- APPEND-ONLY (mod identity; disambiguates mods that share a stat key and
    //     drives the radius-jewel wrapper). Passed by const ptr per visitor callback,
    //     so older plugins safely read only the prefix above. Maps to Mods.dat. ---
    uintptr_t id_addr;   // Mods.dat Id, e.g. "JewelRadiusCooldownSpeed"; 0 if unset
    uint32_t  hash32;    // Mods.dat HASH32 (primary catalog key)
} ModAbi;

// One UI element (UiService::read). string_id_addr is host-owned.
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

// Precomputed map transform (scale_x/y are cos/sin*scale) for grid->screen.
typedef struct {
    float center_x, center_y;
    float scale_x, scale_y;
    float player_grid_x, player_grid_y;
    int32_t is_visible;
} MapTransformAbi;

// A loaded terrain tile location (TerrainService::enumerate_tgt_locations).
typedef struct {
    int32_t tile_x;
    int32_t tile_y;
    float   x;
    float   y;
    uintptr_t path_addr;
} TgtLocationAbi;

// Owning view over a walkable-grid snapshot (1 byte/cell). Call release(opaque)
// (the C++ wrapper does this for you) before the next snapshot/area change.
typedef struct {
    const uint8_t* data;
    int32_t width;
    int32_t height;
    void* opaque;
    void (*release)(void*);
} WalkableGridHandleAbi;

// Owning view over a height-grid snapshot (1 float/cell). See WalkableGrid.
typedef struct {
    const float* data;
    int32_t width;
    int32_t height;
    void* opaque;
    void (*release)(void*);
} HeightGridHandleAbi;

// Visitor callbacks. Return non-zero to continue iterating, 0 to stop early.
// The pointers/strings passed in are valid ONLY for the duration of the call.
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

// ---------------------------------------------------------------------------
// Service vtables. Every call is SEH-guarded host-side: a bad read returns 0 /
// valid==0 rather than crashing. Grouped into HostAbi below.
// ---------------------------------------------------------------------------

// Top-level game/world state and the per-frame snapshot.
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

// Enumerate / look up entities. watch() keeps an entity's component map fresh
// for cross-frame reads. out_* params are filled into caller-owned buffers.
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

// Read a component from its address (from ComponentAddressesAbi), or walk the
// list-valued components (buffs/skills/stats/mods) via the enumerate_* calls.
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

// Inventories and item details. scan() requests a (cheap, async) refresh; the
// read_item_* string getters use the query-then-fill convention: call with
// buf=NULL to get the required size, then again with a sized buffer.
typedef struct {
    void    (*scan)(int32_t inventory_id);
    int32_t (*get)(int32_t inventory_id, InventoryAbi* out);
    void    (*enumerate)(PsdkInventoryVisitorFn cb, void* ud);
    void    (*enumerate_items)(int32_t inventory_id,
                                  PsdkInventoryItemVisitorFn cb, void* ud);
    const char* (*get_name)(int32_t inventory_id);
    int32_t (*read_item_rarity)(uintptr_t entity_addr);
    int32_t (*read_item_stack_count)(uintptr_t entity_addr);
    size_t  (*read_item_base_type_name)(uintptr_t entity_addr,
                                          char* buf, size_t bufsize);
    size_t  (*read_item_unique_name)(uintptr_t entity_addr,
                                       char* buf, size_t bufsize);
    size_t  (*read_item_path)(uintptr_t entity_addr,
                                char* buf, size_t bufsize);
    int32_t (*read_item_mods_summary)(uintptr_t entity_addr,
                                        ItemModsSummaryAbi* out);
    void    (*enumerate_item_mods_by_entity)(uintptr_t entity_addr,
                                               PsdkModVisitorFn cb, void* ud);
} InventoryServiceAbi;

// One UI element's screen rect; ok==0 means the element could not be projected.
typedef struct { float x, y, w, h; int32_t ok; } PsdkScreenRectAbi;

// Walk the game's UI tree. follow_path indexes child-by-child from a root;
// the string getters use the query-then-fill convention (buf=NULL for size).
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

// Project coordinates to screen. All return 0 if the point is off-screen / the
// relevant map isn't visible.
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

// Terrain queries. The grid handles own a snapshot — release before re-pinning.
typedef struct {
    void  (*get_walkable_grid)(WalkableGridHandleAbi* out);
    void  (*get_height_grid)(HeightGridHandleAbi* out);
    int32_t (*is_walkable)(int32_t gx, int32_t gy);
    float (*get_terrain_height)(int32_t gx, int32_t gy);
    float (*get_world_to_grid_convertor)(void);
    void  (*enumerate_tgt_locations)(PsdkTgtVisitorFn cb, void* ud);
} TerrainServiceAbi;

// Raw game-memory reads. The string getters use the query-then-fill convention
// (buf=NULL for required size). read_std_vector: pass element_size and the
// in/out count (in = buffer capacity, out = elements available).
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

// level is "info" / "warn" / "error".
typedef struct {
    void (*log)(const char* level, const char* message);
} LogServiceAbi;

// Subscribe to host events; keep the token to unsubscribe (also auto-cleared
// on unload). The callback runs on the host thread that raised the event.
typedef struct {
    uint64_t (*subscribe)(PsdkEventKind event_kind, PsdkEventCallbackFn cb,
                              void* userdata);
    void     (*unsubscribe)(uint64_t token);
} EventsServiceAbi;

// Per-plugin overlay requests. plugin_token is any stable per-plugin pointer
// (e.g. `this`); the host auto-clears a plugin's requests on unload.
typedef struct {
    void (*set_include_sleeping_entities)(void* plugin_token, int32_t enable);
    void (*set_wants_overlay_input)(void* plugin_token, int32_t enable);
} OverlayServiceAbi;

// Convenience access to the utility belt (life/mana flasks + charms). Empty
// slots come back with valid==0. Slot counts are 2 flasks / 3 charms today.
typedef struct {
    int32_t (*get_flask)(int32_t slot, FlaskAbi* out);
    int32_t (*get_charm)(int32_t slot, CharmAbi* out);
    void (*enumerate_flasks)(PsdkFlaskVisitorFn cb, void* userdata);
    void (*enumerate_charms)(PsdkCharmVisitorFn cb, void* userdata);
    int32_t (*flask_slot_count)(void);
    int32_t (*charm_slot_count)(void);
} FlasksServiceAbi;

// Item prices from the host PriceService (poe2scout). lookup_price returns 1
// when the name resolved; out fields are in chaos / divine / exalted units.
typedef struct { int32_t found; float chaos; float divine; float exalt; char category[32]; } PriceResultAbi;
typedef struct { int32_t loaded; int32_t total_items; float divine_in_chaos;
                 float exalted_in_chaos; int32_t cats_ok, cats_pending, cats_failed; } PriceStatusAbi;
typedef struct {
    int32_t (*lookup_price)(const char* name, PriceResultAbi* out);
    void    (*get_rates)(float* divine_in_chaos, float* exalted_in_chaos);
    void    (*get_status)(PriceStatusAbi* out);
} PricesServiceAbi;

// Runeshape devices (Expedition2Encounter) resolved by RuneshapeResolver.
// One reward slot per recipe; reward_count / best_index live in RuneshapeAbi
// so the visitor can pre-allocate. enumerate_rewards passes rewards for the
// matching entity_id — call after enumerate_runeshapes per item.
typedef struct {
    char    name[48];
    int32_t count;
    float   unit_chaos;
    float   total_chaos;
    int32_t priced;
    // --- APPEND-ONLY (rune propagation, 0.5.4). Passed by const ptr per visitor
    //     callback, so older plugins safely read only the prefix above. ---
    char    propagating_runes[64];  // rune(s) at this recipe's propagating slot(s); "" if none
    int32_t propagating_count;      // number of propagating runes for this reward
    int32_t propagating_has_rare;   // 1 if any propagating rune is rare (idx 23-32)
    // --- APPEND-ONLY (rune weights) ---
    int32_t combo_weight;           // sum of user per-rune weights over this recipe's slots
    int32_t recipe_size;            // number of rune slots in this recipe
} RuneshapeRewardAbi;

typedef struct {
    uint64_t entity_id;
    uint32_t color;
    int32_t  is_unique;
    int32_t  hole_count;
    char     anchor_name[32];
    int32_t  reward_count;
    int32_t  best_index;
    // --- APPEND-ONLY (rune propagation, 0.5.4) ---
    int32_t  propagating_slots[4];   // raw propagating slot indices; unused entries = -1
    int32_t  propagating_slot_count; // number of valid entries in propagating_slots (0..4)
    // --- APPEND-ONLY (rune weights) ---
    int32_t  combo_weight;           // best recipe-combination weight at this station
    // --- APPEND-ONLY (anchor rune identity) ---
    int32_t  anchor_rune_idx;        // Expedition2Runes row of the anchor rune; -1 = anchor-less unique
    int32_t  anchor_pos;             // anchor slot position (0-based, < hole_count)
    // --- APPEND-ONLY (completion state) ---
    int32_t  completed;              // 1 = collected or broken/unavailable (host hides it on the radar)
    // --- APPEND-ONLY (best recipe composition) ---
    int32_t  best_rune_count;        // valid entries in best_runes (0 = unknown, show anchor only)
    int32_t  best_runes[12];         // per-slot rune idx of the best-priced recipe; -1 = empty slot
} RuneshapeAbi;

typedef int32_t (*PsdkRuneshapeVisitorFn)(const RuneshapeAbi*, void*);
typedef int32_t (*PsdkRuneshapeRewardVisitorFn)(const RuneshapeRewardAbi*, void*);

typedef struct {
    void (*enumerate_runeshapes)(PsdkRuneshapeVisitorFn cb, void* ud);
    void (*enumerate_rewards)(uint64_t entity_id, PsdkRuneshapeRewardVisitorFn cb, void* ud);
} RuneshapeServiceAbi;

// ---- Atlas service ----------------------------------------------------------
// Live endgame-atlas panel data (nodes / per-anchor adjacency / selection
// vectors / Rite line seed / eligibility weights), read host-side through the
// GameLibrary atlas offsets — the same single source the built-in Atlas
// overlay uses — so atlas plugins (ForetoldRewards) need no raw memory reads.
// Every call resolves the panel fresh (mouse UI path, controller hint
// fallback) and yields nothing when the panel is absent.

// Per-node detail mask for enumerate_nodes. BASIC costs ONE bulk read for the
// whole node vector; RECORD/UISTATE add one small read per node and NAME a
// multi-hop string walk per node — request only what the call site needs.
typedef enum {
    PSDK_ATLAS_NODE_BASIC   = 0,        // grid + ui_addr + extra_addr only
    PSDK_ATLAS_NODE_RECORD  = 1 << 0,   // + marker / flags (ExtraPtr record)
    PSDK_ATLAS_NODE_UISTATE = 1 << 1,   // + biome / map_state (node UI element)
    PSDK_ATLAS_NODE_NAME    = 1 << 2,   // + name (WorldAreas row walk — expensive)
    PSDK_ATLAS_NODE_ALL     = 0x7,
} PsdkAtlasNodeDetail;

typedef struct {
    int32_t   grid_x;           // AtlasNodes grid coords — stable per map (node
    int32_t   grid_y;           //   ADDRESSES churn every frame; grids do not)
    uintptr_t ui_addr;          // node UI widget (screen-rect target); may be 0
    uintptr_t extra_addr;       // per-node data record (marker/flags home); may be 0
    uint32_t  marker;           // RECORD: Rite line-map type. 0 = not a line map /
                                //   entry not yet loaded; 0x47B = line map with NO
                                //   foretold reward; other non-zero = reward map
    uint32_t  flags;            // RECORD: bit0 = AccessibleNow, bit1 = Completed
    int32_t   biome;            // UISTATE: EndGameMapBiomes _rid (9 = Breach Stronghold)
    int32_t   map_state;        // UISTATE: low 2 bits; bit1 = completed ("Map Attempted")
    char      name[64];         // NAME: UTF-8 display name; "" when unresolved
} AtlasNodeAbi;

// One per-anchor adjacency entry (the Rite "conn table"). Slot ORDER is
// preserved and meaningful: the reward fn's neighbour rank derives from it.
typedef struct {
    int32_t key_x, key_y;       // anchor map grid
    int32_t neighbor_count;     // valid entries below (0..5); unused slots dropped
    int32_t neighbor_x[5];
    int32_t neighbor_y[5];
} AtlasConnAbi;

typedef struct { int32_t x, y; } AtlasGridAbi;                 // one selection-vector grid
typedef struct { int32_t key; int32_t value; } AtlasWeightAbi; // one RAW weight row (keys repeat — dedup-sum client-side)

typedef int32_t (*PsdkAtlasNodeVisitorFn)(const AtlasNodeAbi*, void*);
typedef int32_t (*PsdkAtlasConnVisitorFn)(const AtlasConnAbi*, void*);
typedef int32_t (*PsdkAtlasGridVisitorFn)(const AtlasGridAbi*, void*);
typedef int32_t (*PsdkAtlasWeightVisitorFn)(const AtlasWeightAbi*, void*);

typedef struct {
    // --- FROZEN (2026-07-07): `HostAbi::sekhema` was appended after
    //     `HostAbi::atlas`, so appending here would shift it. Extend via new
    //     HostAbi tail functions instead. ---
    // Atlas panel address (0 = panel absent / not in game). Also usable as the
    // panel-change signal: stable while the panel object lives.
    uintptr_t (*get_panel)(void);
    // Visit every atlas node. detail = PsdkAtlasNodeDetail bit mask.
    void (*enumerate_nodes)(int32_t detail, PsdkAtlasNodeVisitorFn cb, void* ud);
    // Visit the per-anchor adjacency table (Rite line connectivity; viewport-
    // dependent — it grows/shrinks with the loaded atlas region).
    void (*enumerate_connections)(PsdkAtlasConnVisitorFn cb, void* ud);
    // Visit a selection vector: which = 0 -> A (revealed Rite-line "purple"
    // maps), which = 1 -> B (selected anchors, in pick order).
    void (*get_selection)(int32_t which, PsdkAtlasGridVisitorFn cb, void* ud);
    // Rite reward-selection seed (non-zero only while a Rite line is active on
    // the open atlas). Returns 1 + writes *out when the panel resolved, else 0.
    int32_t (*get_line_seed)(uint32_t* out);
    // Visit the raw eligibility-weight rows (panel -> server data -> gold data
    // -> weights vector). The path-cap stat id is key 26379 (0.5.4).
    void (*enumerate_weights)(PsdkAtlasWeightVisitorFn cb, void* ud);
    // Resolve one node's display name (ui_addr = AtlasNodeAbi::ui_addr) into a
    // UTF-8 buffer — the cheap lazy alternative to PSDK_ATLAS_NODE_NAME.
    // Returns the length written (excl. NUL), or 0 when unresolved.
    int32_t (*get_node_name)(uintptr_t ui_addr, char* out, int32_t out_size);
} AtlasServiceAbi;

// ---- Sekhema service --------------------------------------------------------
// Trial of the Sekhemas floor-map data (FloorData room graph / choices /
// content FK rows) plus the StateMachine flag reads trial objects need, read
// host-side through the GameLibrary SekhemaTrial / DatTable / StateMachine
// offsets — so trial plugins (SekhemaHelper) carry no raw memory offsets.
// The graph calls take the trial panel UI address explicitly: plugins keep
// their own panel discovery (UI paths / BFS probing via probe_floor), or use
// get_panel for the host's direct child-index resolution. The fail-closed
// bounds live host-side: layers/rooms accepted in [1,64]/[0,64], connections
// capped at 16, content entries at 512.

// Floor header: dynamic per-run state (one get_floor call) + the per-layer
// room counts of the static graph. layer_count trusts room_counts/choices up
// to 64 entries.
typedef struct {
    int32_t   valid;            // 1 = FloorData resolved (layer_count in [1, 64])
    int32_t   layer_count;
    uintptr_t floor_data;       // resolved FloorData base (change/diagnostic signal)
    int32_t   room_counts[64];  // rooms per layer
    uint8_t   choices[64];      // per-layer chosen room index; 0xFF = none
    uint8_t   counter;          // raw counter byte; (counter & 7) = choices made
    uint8_t   _pad[7];
} SekhemaFloorAbi;

// One room of the static floor graph (immutable for a whole floor).
typedef struct {
    int32_t layer;              // 0-based layer index
    int32_t index;              // room index within its layer
    int32_t connection_count;   // valid entries below (0..16)
    uint8_t connections[16];    // room indices in the NEXT layer
} SekhemaRoomAbi;

// One resolved content FK pair. Both strings are resolved unconditionally;
// dispatch on table_path and read only the field meaningful for that table
// (SanctumRooms -> row_id, SanctumPersistentEffects -> row_name) — the other
// may hold noise where the row layout differs. All strings UTF-8, truncated.
typedef struct {
    uintptr_t row_addr;         // DAT row object (identity/change signal)
    uintptr_t table_addr;       // DAT table object
    char      table_path[128];  // in-memory .dat table path ("" unreadable)
    char      row_id[96];       // row Id (row+0x00; e.g. "Caverns_Arena_03")
    char      row_name[160];    // display name (row+0x28; localized)
} SekhemaContentFkAbi;

// One content entry: which (layer, room) it decorates + its FK pairs. The
// target bytes are delivered as read — bounds-check against the graph before
// joining (entries can reference hidden/out-of-range rooms).
typedef struct {
    int32_t layer;
    int32_t room_index;
    int32_t fk_count;           // valid entries in fks (0..3), compacted
    int32_t _pad;
    SekhemaContentFkAbi fks[3];
} SekhemaContentAbi;

typedef int32_t (*PsdkSekhemaRoomVisitorFn)(const SekhemaRoomAbi*, void*);
typedef int32_t (*PsdkSekhemaContentVisitorFn)(const SekhemaContentAbi*, void*);

typedef struct {
    // --- APPEND ONLY: add new functions at the END, never reorder. Valid only
    //     while `HostAbi::sekhema` stays the LAST HostAbi member; after that the
    //     struct is frozen — extend via new HostAbi tail functions instead. ---
    // Host-resolved trial panel (GameUI child[84], gated on a non-null floor-obj
    // pointer). 0 = absent / controller mode / not in game.
    uintptr_t (*get_panel)(void);
    // Cheap probe (~6 reads): does ui_addr resolve a FloorData whose layer
    // vector counts in [1, 64]? Returns the layer count, 0 = not a trial floor.
    // Intended as the per-node pre-filter for plugin-side panel BFS.
    int32_t (*probe_floor)(uintptr_t ui_addr);
    // Floor header (see SekhemaFloorAbi). Returns 1 + fills *out when a
    // FloorData resolves from panel_addr (with the panel-parent fallback),
    // else 0 with *out zeroed.
    int32_t (*get_floor)(uintptr_t panel_addr, SekhemaFloorAbi* out);
    // Visit every room of the static floor graph in (layer, index) order.
    void (*enumerate_rooms)(uintptr_t panel_addr, PsdkSekhemaRoomVisitorFn cb, void* ud);
    // Visit every content entry (FK rows resolved to id/name strings).
    void (*enumerate_content)(uintptr_t panel_addr, PsdkSekhemaContentVisitorFn cb, void* ud);
    // StateMachine used/collected/closed flag (first byte past the component
    // header; crystals/levers/portals/chest-likes). Returns 1 + writes *out_used
    // (0 active / 1 used) when state_machine_addr != 0 and the host is attached.
    int32_t (*get_room_used_flag)(uintptr_t state_machine_addr, int32_t* out_used);
    // One shared-state VALUE from a StateMachine's values vector (8 bytes per
    // define_shared_state entry, define order — door open/activate states).
    // Returns 1 + writes *out_value; 0 when the vector is empty/oversized (>64)
    // or index is out of range.
    int32_t (*get_state_machine_value)(uintptr_t state_machine_addr, int32_t index,
                                       uint64_t* out_value);
    // A UI element's StringId StdWString as UTF-8 — the field trial-HUD leaves
    // render their numbers into (NOT the get_text field). Returns the length
    // written (excl. NUL), or 0 when empty/unreadable.
    int32_t (*get_ui_string_id)(uintptr_t ui_addr, char* out, int32_t out_size);
} SekhemaServiceAbi;

// The root table handed to every plugin. version must equal PLUGIN_SDK_VERSION;
// trust a field only when size_bytes covers it. Everything below d3d_device is
// an APPEND-ONLY tail — add new host functions here, never in the middle.
typedef struct HostAbi {
    uint32_t              version;
    uint32_t              size_bytes;
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
    void*                 imgui_context;   // ImGuiContext* — set before drawing
    void*                 d3d_device;      // ID3D11Device* — for texture upload

    // --- append-only tail ---
    // Resolve a WorldItem container into its inner item entity (both buffers).
    int32_t (*get_world_item_inner)(uintptr_t container_addr,
                                    EntityInfoAbi* out_e,
                                    ComponentAddressesAbi* out_c);

    OverlayServiceAbi overlay;
    FlasksServiceAbi flasks;

    // Movement route by ENTITY address (host resolves the Pathfinding comp).
    int32_t (*read_pathfinding)(uintptr_t entity_addr, PathfindingAbi* out);

    // Current action (flags + target cell) from an Actor component address.
    int32_t (*read_actor_action)(uintptr_t actor_addr, ActorActionAbi* out);

    // Enumerate a monster's rolled mods from its ObjectMagicProperties
    // component address (ComponentAddressesAbi::omp). Detects monster modifiers
    // at spawn, before any related buff. Append-only tail (2026-06-07).
    void (*enumerate_monster_mods)(uintptr_t omp_addr,
                                   PsdkMonsterModVisitorFn cb, void* ud);

    // Batch-project N UI elements to screen rects in one ABI hop (shared-
    // ancestor memo). MUST live on the HostAbi tail, NOT inside UiServiceAbi:
    // services are embedded by value, so growing a mid-struct service shifts
    // every field after it and breaks already-compiled plugins. Append-only
    // tail (moved off UiServiceAbi 2026-06-16 to restore that ABI compat).
    int32_t (*compute_screen_rects)(const uintptr_t* addrs, int32_t count,
                                    PsdkScreenRectAbi* out);

    // Enumerate buffs AGGREGATED by name: duplicate-named status effects
    // (e.g. each Chayula breach charge is its own StatusEffect instance)
    // collapse into ONE entry whose `charges` = the first instance's raw
    // charges + 1 per additional instance — matching the in-game charge-stack
    // icon and the host Debug "Chg" column (mirrors GameData::Buffs::
    // StatusEffects()). The plain ComponentsService enumerate_buffs returns the
    // raw per-instance list, so single-instance buffs (power/frenzy charges that
    // already carry their count) read identically either way; only multi-
    // instance stacks differ. Append-only tail (2026-06-16).
    void (*enumerate_buffs_aggregated)(uintptr_t buffs_addr,
                                       PsdkBuffVisitorFn cb, void* ud);

    // Item prices from the host PriceService. Append-only tail (2026-06-17).
    PricesServiceAbi prices;

    // Runeshape devices (Expedition2Encounter) with per-entity rewards.
    // Append-only tail (2026-06-17).
    RuneshapeServiceAbi runeshape;

    // Resolve an item name to its host-cached icon PNG path (absolute UTF-8) in a
    // PLUGIN-OWNED buffer. Returns 1 + writes a NUL-terminated path when an icon is
    // cached, else 0. Append-only tail (2026-06-18). NEVER add this to PriceResultAbi
    // (that struct is filled into a plugin buffer — growing it overruns old plugins).
    int32_t (*lookup_price_icon)(const char* name, char* out_path, int32_t out_path_size);

    // Character gold counter — the amount shown on the in-game inventory gold
    // display. Returns the current gold, or 0 when not in game / unavailable.
    // Sourced from the host's ServerData snapshot. Append-only tail (2026-06-24).
    int32_t (*get_gold)(void);

    // Format a stat key + value(s) into in-game-style text using the host's .csd
    // stat-description set (the same formatting the Debug panel uses). Writes a
    // NUL-terminated UTF-8 string into `out` (truncated to out_size); returns the
    // length written (excl. NUL), or 0 if unavailable. Append-only tail
    // (2026-06-24, after get_gold).
    int32_t (*format_stat_description)(const char* stat_key, float v0, float v1,
                                       char* out, int32_t out_size);

    // Read an inventory item's base defensive values into out4 = {armour,
    // evasion, energy_shield, ward}. Returns 1 if the item has an Armour
    // component (out4 always written, zeroed when absent), else 0. Append-only
    // tail (2026-06-24).
    int32_t (*read_item_base_stats)(uintptr_t entity_addr, int32_t* out4);

    // Read an item's aggregated stats (StatsFromMods) as flat {id,value} pairs
    // into out_pairs (capacity 2*max_pairs ints). Returns the TOTAL pair count
    // (may exceed max_pairs; only max_pairs are written). Append-only tail
    // (2026-06-24).
    int32_t (*read_item_aggregated_stats)(uintptr_t entity_addr, int32_t* out_pairs,
                                          int32_t max_pairs);

    // Read a ground effect from an ENTITY address: the host resolves the
    // "GroundEffect" component (not in ComponentAddressesAbi, so it takes the
    // entity address like read_pathfinding) and its groundeffects.datc64 row,
    // exposing the type Id / radius / visuals so a plugin can distinguish the
    // many same-pathed VisibleServerGroundEffect entities. Append-only tail
    // (2026-06-25).
    int32_t (*read_ground_effect)(uintptr_t entity_addr, GroundEffectAbi* out);

    // Whether the host Radar -> RuneShape "Show weights on map" toggle is ON.
    // Overlay plugins gate their rune-weight chips on it so weight display
    // follows the one host-side switch. Returns 0/1. Append-only tail
    // (2026-07-03).
    int32_t (*get_runeshape_weights_shown)(void);

    // Raw WorldArea id of the current zone (e.g. "Sanctum_1",
    // "Sanctum_2_Foyer_1"). Carries the Trial-of-the-Sekhemas FLOOR number even
    // when room content is hidden on the map (relic "The Burden of Leadership"),
    // where the localized display name and the numeric hash do not. Writes a
    // NUL-terminated UTF-8 string into `out` (truncated to out_size); returns the
    // length written (excl. NUL), or 0 if unavailable. Append-only tail
    // (2026-07-06).
    int32_t (*get_area_id)(char* out, int32_t out_size);

    // Genesis-tree (Hiveblood) resource counter. Host walks the pattern-anchored
    // chain (GameOffsets::Objects::Hiveblood). Returns 1 + writes *out on success,
    // 0 when not in game / chain broken. Append-only tail (2026-07-07).
    int32_t (*get_hiveblood)(int32_t* out);

    // Live endgame-atlas panel data (nodes / adjacency / selection / line seed /
    // weights / node names), read host-side via the GameLibrary atlas offsets.
    // Embedded by-value: FROZEN since `sekhema` was appended after it —
    // extensions must land as new HostAbi tail functions (the UiServiceAbi /
    // compute_screen_rects lesson). Append-only tail (2026-07-07, after
    // get_hiveblood).
    AtlasServiceAbi atlas;

    // Trial of the Sekhemas floor-map data (room graph / choices / content FK
    // rows) + StateMachine flag reads, read host-side via the GameLibrary
    // SekhemaTrial offsets. Embedded by-value: appending inside
    // SekhemaServiceAbi is safe ONLY while it is the LAST HostAbi member; the
    // moment anything is appended after it, the struct is frozen and extensions
    // must land as new HostAbi tail functions. Append-only tail (2026-07-07,
    // after atlas).
    SekhemaServiceAbi sekhema;
} HostAbi;

#ifdef __cplusplus
}
#endif

#endif
