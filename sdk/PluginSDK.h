// File: POEFixer/plugin_sdk/PluginSDK.h
//
// Header-only C++ wrapper around the PluginAbi C ABI. Plugin authors
// #include this and only this; all std::* types live entirely inside the
// plugin DLL while only POD crosses the host/DLL boundary.
//
// Build the plugin DLL with PLUGIN_EXPORTS defined to export the
// PluginSDK_AttachHost symbol (defined inline below) alongside
// CreatePlugin / DestroyPlugin.

#pragma once

#include "PluginAbi.h"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <map>
#include <utility>
#include <filesystem>
#include <Windows.h>

#ifdef PLUGIN_EXPORTS
    #define PLUGIN_API __declspec(dllexport)
#else
    #define PLUGIN_API __declspec(dllimport)
#endif

namespace PluginSDK {

enum class EntityType : int32_t {
    Unidentified      = PSDK_ENTITY_TYPE_UNIDENTIFIED,
    Chest             = PSDK_ENTITY_TYPE_CHEST,
    NPC               = PSDK_ENTITY_TYPE_NPC,
    Player            = PSDK_ENTITY_TYPE_PLAYER,
    Shrine            = PSDK_ENTITY_TYPE_SHRINE,
    Monster           = PSDK_ENTITY_TYPE_MONSTER,
    DeliriumBomb      = PSDK_ENTITY_TYPE_DELIRIUM_BOMB,
    DeliriumSpawner   = PSDK_ENTITY_TYPE_DELIRIUM_SPAWNER,
    OtherImportant    = PSDK_ENTITY_TYPE_OTHER_IMPORTANT,
    Item              = PSDK_ENTITY_TYPE_ITEM,
    Renderable        = PSDK_ENTITY_TYPE_RENDERABLE,
    AreaTransition    = PSDK_ENTITY_TYPE_AREA_TRANSITION,
    ExpeditionMarker  = PSDK_ENTITY_TYPE_EXPEDITION_MARKER,
    ExpeditionRemnant = PSDK_ENTITY_TYPE_EXPEDITION_REMNANT,
};

enum class EntitySubtype : int32_t {
    Unidentified        = PSDK_ENTITY_SUBTYPE_UNIDENTIFIED,
    None                = PSDK_ENTITY_SUBTYPE_NONE,
    PlayerSelf          = PSDK_ENTITY_SUBTYPE_PLAYER_SELF,
    PlayerOther         = PSDK_ENTITY_SUBTYPE_PLAYER_OTHER,
    ChestMagic          = PSDK_ENTITY_SUBTYPE_CHEST_MAGIC,
    ChestRare           = PSDK_ENTITY_SUBTYPE_CHEST_RARE,
    ExpeditionChest     = PSDK_ENTITY_SUBTYPE_EXPEDITION_CHEST,
    BreachChest         = PSDK_ENTITY_SUBTYPE_BREACH_CHEST,
    Strongbox           = PSDK_ENTITY_SUBTYPE_STRONGBOX,
    JewellerStrongbox   = PSDK_ENTITY_SUBTYPE_JEWELLER_STRONGBOX,
    ResearcherStrongbox = PSDK_ENTITY_SUBTYPE_RESEARCHER_STRONGBOX,
    LargeStrongbox      = PSDK_ENTITY_SUBTYPE_LARGE_STRONGBOX,
    OmenChest           = PSDK_ENTITY_SUBTYPE_OMEN_CHEST,
    SpecialNPC          = PSDK_ENTITY_SUBTYPE_SPECIAL_NPC,
    POIMonster          = PSDK_ENTITY_SUBTYPE_POI_MONSTER,
    PinnacleBoss        = PSDK_ENTITY_SUBTYPE_PINNACLE_BOSS,
    WorldItem           = PSDK_ENTITY_SUBTYPE_WORLD_ITEM,
    InventoryItem       = PSDK_ENTITY_SUBTYPE_INVENTORY_ITEM,
};

enum class EntityState : int32_t {
    None               = PSDK_ENTITY_STATE_NONE,
    Useless            = PSDK_ENTITY_STATE_USELESS,
    PlayerLeader       = PSDK_ENTITY_STATE_PLAYER_LEADER,
    MonsterFriendly    = PSDK_ENTITY_STATE_MONSTER_FRIENDLY,
    PinnacleBossHidden = PSDK_ENTITY_STATE_PINNACLE_BOSS_HIDDEN,
};

enum class NearbyZone : int32_t {
    None        = PSDK_NEARBY_NONE,
    InnerCircle = PSDK_NEARBY_INNER_CIRCLE,
    OuterCircle = PSDK_NEARBY_OUTER_CIRCLE,
    Far         = PSDK_NEARBY_FAR,
};

enum class GameState : int32_t {
    AreaLoading       = PSDK_GAME_STATE_AREA_LOADING,
    ChangePassword    = PSDK_GAME_STATE_CHANGE_PASSWORD,
    Credits           = PSDK_GAME_STATE_CREDITS,
    Escape            = PSDK_GAME_STATE_ESCAPE,
    InGame            = PSDK_GAME_STATE_IN_GAME,
    PreGame           = PSDK_GAME_STATE_PRE_GAME,
    Login             = PSDK_GAME_STATE_LOGIN,
    Waiting           = PSDK_GAME_STATE_WAITING,
    CreateCharacter   = PSDK_GAME_STATE_CREATE_CHARACTER,
    SelectCharacter   = PSDK_GAME_STATE_SELECT_CHARACTER,
    DeleteCharacter   = PSDK_GAME_STATE_DELETE_CHARACTER,
    Loading           = PSDK_GAME_STATE_LOADING,
    NotLoaded         = PSDK_GAME_STATE_NOT_LOADED,
};

enum class EventKind : int32_t {
    AreaChange   = PSDK_EVENT_AREA_CHANGE,
    Frame        = PSDK_EVENT_FRAME,
    GameAttached = PSDK_EVENT_GAME_ATTACHED,
    GameDetached = PSDK_EVENT_GAME_DETACHED,
};

enum class ModKind : int32_t {
    Implicit  = PSDK_MOD_KIND_IMPLICIT,
    Explicit  = PSDK_MOD_KIND_EXPLICIT,
    Enchant   = PSDK_MOD_KIND_ENCHANT,
    Hellscape = PSDK_MOD_KIND_HELLSCAPE,
    Crucible  = PSDK_MOD_KIND_CRUCIBLE,
};

enum class StatSource : int32_t {
    Items = PSDK_STAT_SOURCE_ITEMS,
    Buffs = PSDK_STAT_SOURCE_BUFFS,
};

class GameService;
class EntitiesService;
class ComponentsService;
class InventoryService;
class UiService;
class RenderService;
class TerrainService;
class MemoryService;
class LogService;
class EventsService;
class OverlayService;
class FlasksService;
class Plugin;
struct Context;

// Materialize a UTF-8 / wide string the ABI exposed only by address.
// Two-call protocol: first call with bufsize=0 returns the required byte
// count (incl. trailing null), then the second call fills the buffer.
inline std::string FetchString(uintptr_t addr, const HostAbi* abi) {
    if (!addr || !abi || !abi->memory.read_string) return {};
    size_t needed = abi->memory.read_string(addr, nullptr, 0);
    if (needed <= 1) return {};
    std::string s;
    s.resize(needed - 1);
    abi->memory.read_string(addr, s.data(), needed);
    return s;
}

inline std::wstring FetchWString(uintptr_t addr, const HostAbi* abi) {
    if (!addr || !abi || !abi->memory.read_wstring) return {};
    size_t needed = abi->memory.read_wstring(addr, nullptr, 0);
    if (needed <= 1) return {};
    std::wstring s;
    s.resize(needed - 1);
    abi->memory.read_wstring(addr, s.data(), needed);
    return s;
}

struct Vital {
    int   Current = 0;
    int   Total = 0;
    int   ReservedFlat = 0;
    int   ReservedPercent = 0;
    float Regeneration = 0.f;
    bool  Valid = false;

    static Vital FromAbi(const VitalAbi& a) {
        Vital v;
        v.Current         = a.current;
        v.Total           = a.total;
        v.ReservedFlat    = a.reserved_flat;
        v.ReservedPercent = a.reserved_percent;
        v.Regeneration    = a.regeneration;
        v.Valid           = a.valid != 0;
        return v;
    }
};

struct Life {
    Vital     Health;
    Vital     Mana;
    Vital     EnergyShield;
    uintptr_t Address = 0;
    uintptr_t OwnerAddress = 0;
    bool      Valid = false;

    static Life FromAbi(const LifeAbi& a) {
        Life l;
        l.Health       = Vital::FromAbi(a.health);
        l.Mana         = Vital::FromAbi(a.mana);
        l.EnergyShield = Vital::FromAbi(a.energy_shield);
        l.Address      = a.address;
        l.OwnerAddress = a.owner_address;
        l.Valid        = a.valid != 0;
        return l;
    }
};

struct Render {
    float     WorldX = 0, WorldY = 0, WorldZ = 0;
    float     ModelBoundsX = 0, ModelBoundsY = 0, ModelBoundsZ = 0;
    float     TerrainHeight = 0;
    uintptr_t Address = 0;
    uintptr_t OwnerAddress = 0;
    bool      Valid = false;

    static Render FromAbi(const RenderAbi& a) {
        Render r;
        r.WorldX        = a.world_x;
        r.WorldY        = a.world_y;
        r.WorldZ        = a.world_z;
        r.ModelBoundsX  = a.model_bounds_x;
        r.ModelBoundsY  = a.model_bounds_y;
        r.ModelBoundsZ  = a.model_bounds_z;
        r.TerrainHeight = a.terrain_height;
        r.Address       = a.address;
        r.OwnerAddress  = a.owner_address;
        r.Valid         = a.valid != 0;
        return r;
    }
};

struct Positioned {
    int       Reaction = 0;
    bool      IsFriendly = false;
    uintptr_t Address = 0;
    uintptr_t OwnerAddress = 0;
    bool      Valid = false;

    static Positioned FromAbi(const PositionedAbi& a) {
        Positioned p;
        p.Reaction     = a.reaction;
        p.IsFriendly   = a.is_friendly != 0;
        p.Address      = a.address;
        p.OwnerAddress = a.owner_address;
        p.Valid        = a.valid != 0;
        return p;
    }
};

struct Targetable {
    bool IsTargetable = false;
    bool IsHighlightable = false;
    bool IsTargetedByPlayer = false;
    bool HiddenFromPlayer = false;
    bool MeetsQuestState = false;
    bool MeetsItemRequirements = false;
    bool Valid = false;

    static Targetable FromAbi(const TargetableAbi& a) {
        Targetable t;
        t.IsTargetable          = a.is_targetable != 0;
        t.IsHighlightable       = a.is_highlightable != 0;
        t.IsTargetedByPlayer    = a.is_targeted_by_player != 0;
        t.HiddenFromPlayer      = a.hidden_from_player != 0;
        t.MeetsQuestState       = a.meets_quest_state != 0;
        t.MeetsItemRequirements = a.meets_item_requirements != 0;
        t.Valid                 = a.valid != 0;
        return t;
    }
};

struct Chest {
    bool IsOpened = false;
    bool IsLabelVisible = false;
    bool Valid = false;

    static Chest FromAbi(const ChestAbi& a) {
        Chest c;
        c.IsOpened       = a.is_opened != 0;
        c.IsLabelVisible = a.is_label_visible != 0;
        c.Valid          = a.valid != 0;
        return c;
    }
};

struct Shrine {
    bool IsUsed = false;
    bool Valid = false;

    static Shrine FromAbi(const ShrineAbi& a) {
        Shrine s;
        s.IsUsed = a.is_used != 0;
        s.Valid  = a.valid != 0;
        return s;
    }
};

struct Stack {
    int  CurrentSize = 0;
    int  MaxSize = 0;
    bool Valid = false;

    static Stack FromAbi(const StackAbi& a) {
        Stack s;
        s.CurrentSize = a.current_size;
        s.MaxSize     = a.max_size;
        s.Valid       = a.valid != 0;
        return s;
    }
};

struct Charges {
    int  Current = 0;
    int  PerUseCharges = 0;
    bool Valid = false;

    static Charges FromAbi(const ChargesAbi& a) {
        Charges c;
        c.Current       = a.current;
        c.PerUseCharges = a.per_use_charges;
        c.Valid         = a.valid != 0;
        return c;
    }
};

struct Player {
    std::string Name;
    uint32_t    Xp = 0;
    uint8_t     Level = 0;
    bool        Valid = false;

    static Player FromAbi(const PlayerAbi& a, const HostAbi* abi) {
        Player p;
        p.Xp    = a.xp;
        p.Level = a.level;
        p.Valid = a.valid != 0;
        p.Name  = FetchString(a.name_addr, abi);
        return p;
    }
};

struct Animated {
    std::string Path;
    uint32_t    Id = 0;
    bool        Valid = false;

    static Animated FromAbi(const AnimatedAbi& a, const HostAbi* abi) {
        Animated an;
        an.Id    = a.id;
        an.Valid = a.valid != 0;
        an.Path  = FetchString(a.path_addr, abi);
        return an;
    }
};

struct Transitionable {
    int16_t CurrentState = 0;
    bool    Valid = false;

    static Transitionable FromAbi(const TransitionableAbi& a) {
        Transitionable t;
        t.CurrentState = a.current_state;
        t.Valid        = a.valid != 0;
        return t;
    }
};

struct TriggerableBlockage {
    bool IsClosed = false;
    bool IsBlocked = false;
    bool Valid = false;

    static TriggerableBlockage FromAbi(const TriggerableBlockageAbi& a) {
        TriggerableBlockage t;
        t.IsClosed  = a.is_closed != 0;
        t.IsBlocked = a.is_blocked != 0;
        t.Valid     = a.valid != 0;
        return t;
    }
};

struct MinimapIcon {
    uintptr_t DatRowAddress = 0;
    bool      Valid = false;

    static MinimapIcon FromAbi(const MinimapIconAbi& a) {
        MinimapIcon m;
        m.DatRowAddress = a.dat_row_addr;
        m.Valid         = a.valid != 0;
        return m;
    }
};

struct StateMachine {
    int       StatesCount = 0;
    uintptr_t StatesPtr = 0;
    bool      Valid = false;

    static StateMachine FromAbi(const StateMachineAbi& a) {
        StateMachine s;
        s.StatesCount = a.states_count;
        s.StatesPtr   = a.states_ptr;
        s.Valid       = a.valid != 0;
        return s;
    }
};

struct Base {
    std::string BaseTypeName;
    uint8_t     Width = 0;
    uint8_t     Height = 0;
    bool        Valid = false;

    static Base FromAbi(const BaseAbi& a, const HostAbi* abi) {
        Base b;
        b.Width        = a.width;
        b.Height       = a.height;
        b.Valid        = a.valid != 0;
        b.BaseTypeName = FetchString(a.base_type_name_addr, abi);
        return b;
    }
};

struct Mods {
    bool IsIdentified = false;
    bool IsCorrupted = false;
    bool IsSplit = false;
    bool IsMirrored = false;
    bool IsRelic = false;
    bool IsSynthesised = false;
    int  Rarity = 0;
    int  ItemLevel = 0;
    int  RequiredLevel = 0;
    int  CraftedModCount = 0;
    bool Valid = false;

    static Mods FromAbi(const ModsAbi& a) {
        Mods m;
        m.IsIdentified    = a.is_identified != 0;
        m.IsCorrupted     = a.is_corrupted != 0;
        m.IsSplit         = a.is_split != 0;
        m.IsMirrored      = a.is_mirrored != 0;
        m.IsRelic         = a.is_relic != 0;
        m.IsSynthesised   = a.is_synthesised != 0;
        m.Rarity          = a.rarity;
        m.ItemLevel       = a.item_level;
        m.RequiredLevel   = a.required_level;
        m.CraftedModCount = a.crafted_mod_count;
        m.Valid           = a.valid != 0;
        return m;
    }
};

struct Stats {
    int  CurrentWeaponIndex = 0;
    bool IsShapeshifted = false;
    bool Valid = false;

    static Stats FromAbi(const StatsAbi& a) {
        Stats s;
        s.CurrentWeaponIndex = a.current_weapon_index;
        s.IsShapeshifted     = a.is_shapeshifted != 0;
        s.Valid              = a.valid != 0;
        return s;
    }
};

struct Actor {
    std::string AnimationName;
    int         AnimationId = 0;
    bool        Valid = false;

    static Actor FromAbi(const ActorAbi& a, const HostAbi* abi) {
        Actor ac;
        ac.AnimationId   = a.animation_id;
        ac.Valid         = a.valid != 0;
        ac.AnimationName = FetchString(a.animation_name_addr, abi);
        return ac;
    }
};

struct Npc {
    uintptr_t EntityOwnerAddress = 0;
    bool      Valid = false;

    static Npc FromAbi(const NpcAbi& a) {
        Npc n;
        n.EntityOwnerAddress = a.entity_owner_addr;
        n.Valid              = a.valid != 0;
        return n;
    }
};

struct DiesAfterTime {
    uintptr_t EntityOwnerAddress = 0;
    bool      Valid = false;

    static DiesAfterTime FromAbi(const DiesAfterTimeAbi& a) {
        DiesAfterTime d;
        d.EntityOwnerAddress = a.entity_owner_addr;
        d.Valid              = a.valid != 0;
        return d;
    }
};

struct Buff {
    std::string Name;
    float       TotalTime = 0;
    float       TimeLeft = 0;
    int16_t     Charges = 0;
    int16_t     FlaskSlot = 0;
    int16_t     Effectiveness = 0;
    uint32_t    SourceEntityId = 0;

    static Buff FromAbi(const BuffAbi& a, const HostAbi* abi) {
        Buff b;
        b.TotalTime      = a.total_time;
        b.TimeLeft       = a.time_left;
        b.Charges        = a.charges;
        b.FlaskSlot      = a.flask_slot;
        b.Effectiveness  = a.effectiveness;
        b.SourceEntityId = a.source_entity_id;
        b.Name           = FetchString(a.name_addr, abi);
        return b;
    }
};

// Component marker; the actual buff list comes from
// ComponentsService::EnumerateBuffs.
struct Buffs {
    bool Valid = false;

    static Buffs FromAbi(const BuffsAbi& a) {
        Buffs b;
        b.Valid = a.valid != 0;
        return b;
    }
};

struct ActiveSkill {
    std::string Name;
    int  CurrentSize = 0;
    int  TotalUses = 0;
    int  UseStage = 0;
    int  CastType = 0;
    int  TotalCooldownMs = 0;
    bool CanBeUsed = false;

    // === append-only extensions (added 2026-05-23) ===
    int  MaxUses = 0;                   // 0 if not cooldown-bound
    int  TotalActiveCooldowns = 0;      // remaining charges = MaxUses - this (when MaxUses > 0)
    uint32_t  EquipmentInfoPacked = 0;  // also decoded into Equipment below
    uintptr_t GrantedEffectsPerLevelAddr = 0;
    uintptr_t ActiveSkillsDatAddr = 0;
    uintptr_t GrantedEffectStatSetsPerLevelAddr = 0;
    uintptr_t SkillDetailsAddr = 0;     // pass to ctx->Memory.Read for further DAT fields

    // Decoded equipment info. Bit layout mirrors GameHelper2's
    // GameHelper/Utils/MiscHelper.cs `ActiveSkillGemDataParser` exactly.
    struct EquipmentInfo {
        uint32_t  GemNameHash = 0;       // packed >> 0x10              (upper 16 bits)
        int       InventorySlot = 0;     // (low16 & 0x7F) + 1          (1-based InventoryName-ish)
        int       LinkIndex = 0;         // (low16 >> 7) & 0x07
        int       SocketIndex = 0;       // (low16 >> 10) & 0x07
        int       UnknownFlag = 0;       // (low16 >> 13) & 0x03        ("something vaal related" per GameHelper2)
        bool      CanBeOnPlayerItem = false; // (low16 >> 15) > 0       (residual bit after the above shifts)
    } Equipment;

    // NOTE on safe field access: ActiveSkillAbi is an append-only callback
    // payload struct. We always read the full plan-version layout here
    // because the plugin DLL and host build against the same checked-out
    // PluginSDK.h / PluginAbi.h pair (PluginManager already verifies version
    // and size_bytes >= sizeof(HostAbi) at attach time). For plugins built
    // against an older PluginSDK.h, the older FromAbi simply reads fewer
    // fields — that's fine because the host still writes the larger struct
    // and the older code ignores the trailing bytes.

    static ActiveSkill FromAbi(const ActiveSkillAbi& a, const HostAbi* abi) {
        ActiveSkill s;
        s.CurrentSize     = a.current_size;
        s.TotalUses       = a.total_uses;
        s.UseStage        = a.use_stage;
        s.CastType        = a.cast_type;
        s.TotalCooldownMs = a.total_cooldown_ms;
        s.CanBeUsed       = a.can_be_used != 0;
        s.Name            = FetchString(a.name_addr, abi);

        // Extensions
        s.MaxUses                          = a.max_uses;
        s.TotalActiveCooldowns             = a.total_active_cooldowns;
        s.EquipmentInfoPacked              = a.equipment_info_packed;
        s.GrantedEffectsPerLevelAddr       = a.granted_effects_per_level_addr;
        s.ActiveSkillsDatAddr              = a.active_skills_dat_addr;
        s.GrantedEffectStatSetsPerLevelAddr = a.granted_effect_stat_sets_per_level_addr;
        s.SkillDetailsAddr                 = a.skill_details_addr;

        // Decode packed equipment info (MiscHelper.cs:24-49 line-for-line).
        uint32_t packed = a.equipment_info_packed;
        s.Equipment.GemNameHash   = packed >> 0x10;
        uint32_t low = packed & 0x0000FFFF;
        s.Equipment.InventorySlot = static_cast<int>((low & 0x7F) + 1);
        low >>= 7;
        s.Equipment.LinkIndex     = static_cast<int>(low & 0x07);
        low >>= 3;
        s.Equipment.SocketIndex   = static_cast<int>(low & 0x07);
        low >>= 3;
        s.Equipment.UnknownFlag   = static_cast<int>(low & 0x03);
        low >>= 2;
        s.Equipment.CanBeOnPlayerItem = low > 0;

        return s;
    }
};

struct Mod {
    std::string Name;
    std::string StatKey;
    std::string AffixName;
    int   GenerationType = 0;  // 1=Prefix, 2=Suffix, 3=Implicit
    float Value0 = 0.f;
    float Value1 = 0.f;
    ModKind Kind = ModKind::Implicit;

    static Mod FromAbi(const ModAbi& a, ModKind kind, const HostAbi* abi) {
        Mod m;
        m.GenerationType = a.generation_type;
        m.Value0         = a.value0;
        m.Value1         = a.value1;
        m.Kind           = kind;
        m.Name           = FetchString(a.name_addr, abi);
        m.StatKey        = FetchString(a.stat_key_addr, abi);
        m.AffixName      = FetchString(a.affix_name_addr, abi);
        return m;
    }
};

// Per-entity item-mod aggregate. Distinct from `Mods` (the component POD);
// this adds the per-kind mod lists exposed by
// InventoryService::ReadItemMods(entityAddr).
struct ItemMods {
    int Rarity = 0;
    int ItemLevel = 0;
    int RequiredLevel = 0;
    int CraftedModCount = 0;
    bool IsIdentified = false;
    bool IsCorrupted = false;
    bool IsSplit = false;
    bool IsMirrored = false;
    bool IsRelic = false;
    bool IsSynthesised = false;
    std::vector<Mod> ImplicitMods;
    std::vector<Mod> ExplicitMods;
    std::vector<Mod> EnchantMods;
    std::vector<Mod> HellscapeMods;
    std::vector<Mod> CrucibleMods;
    bool Valid = false;
};

struct UiElement {
    uintptr_t ParentAddress = 0;
    int       ChildCount = 0;
    float     RelativeX = 0, RelativeY = 0;
    float     PositionModX = 0, PositionModY = 0;
    float     UnscaledWidth = 0, UnscaledHeight = 0;
    float     LocalScaleMultiplier = 1.f;
    uint32_t  Flags = 0;
    uint16_t  ElementType = 0;
    uint8_t   ScaleIndex = 0;
    bool      IsVisible = false;
    bool      HasPositionModifier = false;
    uintptr_t StringIdAddress = 0;
    bool      Valid = false;

    static UiElement FromAbi(const UiElementAbi& a) {
        UiElement u;
        u.ParentAddress         = a.parent_addr;
        u.ChildCount            = a.child_count;
        u.RelativeX             = a.relative_x;
        u.RelativeY             = a.relative_y;
        u.PositionModX          = a.position_mod_x;
        u.PositionModY          = a.position_mod_y;
        u.UnscaledWidth         = a.unscaled_width;
        u.UnscaledHeight        = a.unscaled_height;
        u.LocalScaleMultiplier  = a.local_scale_multiplier;
        u.Flags                 = a.flags;
        u.ElementType           = a.element_type;
        u.ScaleIndex            = a.scale_index;
        u.IsVisible             = a.is_visible != 0;
        u.HasPositionModifier   = a.has_position_modifier != 0;
        u.StringIdAddress       = a.string_id_addr;
        u.Valid                 = a.valid != 0;
        return u;
    }
};

struct MapTransform {
    float CenterX = 0, CenterY = 0;
    float ScaleX = 0, ScaleY = 0;  // pre-multiplied cos/sin
    float PlayerGridX = 0, PlayerGridY = 0;
    bool  IsVisible = false;

    static MapTransform FromAbi(const MapTransformAbi& a) {
        MapTransform m;
        m.CenterX     = a.center_x;
        m.CenterY     = a.center_y;
        m.ScaleX      = a.scale_x;
        m.ScaleY      = a.scale_y;
        m.PlayerGridX = a.player_grid_x;
        m.PlayerGridY = a.player_grid_y;
        m.IsVisible   = a.is_visible != 0;
        return m;
    }
};

struct MapData {
    float CenterX = 0, CenterY = 0;
    float SizeX = 0, SizeY = 0;
    float ShiftX = 0, ShiftY = 0;
    float DefaultShiftX = 0, DefaultShiftY = 0;
    float Zoom = 0;
    float Scale = 0;
    bool  IsVisible = false;

    static MapData FromAbi(const MapDataAbi& a) {
        MapData m;
        m.CenterX       = a.center_x;
        m.CenterY       = a.center_y;
        m.SizeX         = a.size_x;
        m.SizeY         = a.size_y;
        m.ShiftX        = a.shift_x;
        m.ShiftY        = a.shift_y;
        m.DefaultShiftX = a.default_shift_x;
        m.DefaultShiftY = a.default_shift_y;
        m.Zoom          = a.zoom;
        m.Scale         = a.scale;
        m.IsVisible     = a.is_visible != 0;
        return m;
    }
};

struct Vitals {
    int  CurrentHP = 0, MaxHP = 0, HPPercent = 0;
    int  CurrentES = 0, MaxES = 0, ESPercent = 0;
    int  CurrentMP = 0, MaxMP = 0, MPPercent = 0;
    bool IsTownOrHideout = false;
    bool IsPaused = false;
    bool IsValid = false;

    static Vitals FromAbi(const VitalsAbi& a) {
        Vitals v;
        v.CurrentHP       = a.current_hp;
        v.MaxHP           = a.max_hp;
        v.HPPercent       = a.hp_percent;
        v.CurrentES       = a.current_es;
        v.MaxES           = a.max_es;
        v.ESPercent       = a.es_percent;
        v.CurrentMP       = a.current_mp;
        v.MaxMP           = a.max_mp;
        v.MPPercent       = a.mp_percent;
        v.IsTownOrHideout = a.is_town_or_hideout != 0;
        v.IsPaused        = a.is_paused != 0;
        v.IsValid         = a.is_valid != 0;
        return v;
    }
};

struct TgtLocation {
    std::string Path;
    int   TileX = 0;
    int   TileY = 0;
    float X = 0;
    float Y = 0;

    static TgtLocation FromAbi(const TgtLocationAbi& a, const HostAbi* abi) {
        TgtLocation t;
        t.TileX = a.tile_x;
        t.TileY = a.tile_y;
        t.X     = a.x;
        t.Y     = a.y;
        t.Path  = FetchString(a.path_addr, abi);
        return t;
    }
};

struct InventoryItem {
    uintptr_t Address = 0;
    int       SlotX = 0;
    int       SlotY = 0;
    int       Width = 0;
    int       Height = 0;
    int       StackCount = 0;
    int       Rarity = 0;
    int       ItemLevel = 0;
    int       RequiredLevel = 0;
    bool      IsIdentified = false;
    bool      IsCorrupted = false;
    bool      IsCurrency = false;
    int       CraftedModCount = 0;
    std::string Path;
    std::string BaseTypeName;
    std::string UniqueName;

    static InventoryItem FromAbi(const InventoryItemAbi& a, const HostAbi* abi) {
        InventoryItem i;
        i.Address          = a.address;
        i.SlotX            = a.slot_x;
        i.SlotY            = a.slot_y;
        i.Width            = a.width;
        i.Height           = a.height;
        i.StackCount       = a.stack_count;
        i.Rarity           = a.rarity;
        i.ItemLevel        = a.item_level;
        i.RequiredLevel    = a.required_level;
        i.IsIdentified     = a.is_identified != 0;
        i.IsCorrupted      = a.is_corrupted != 0;
        i.IsCurrency       = a.is_currency != 0;
        i.CraftedModCount  = a.crafted_mod_count;
        i.Path             = FetchString(a.path_addr, abi);
        i.BaseTypeName     = FetchString(a.base_type_name_addr, abi);
        i.UniqueName       = FetchString(a.unique_name_addr, abi);
        return i;
    }
};

struct Flask {
    uintptr_t   EntityAddress = 0;
    int32_t     SlotIndex = -1;
    int32_t     ChargesCurrent = 0;
    int32_t     PerUseBase = 0;
    int32_t     PerUseEffective = 0;
    bool        Usable = false;
    bool        Active = false;
    bool        IsLife = false;
    bool        IsMana = false;
    int32_t     SlotX = 0;
    int32_t     SlotY = 0;
    int32_t     ModCount = 0;
    std::string Name;
    std::string BaseType;
    std::string Path;
    bool        Valid = false;

    static Flask FromAbi(const FlaskAbi& a, const HostAbi* abi) {
        Flask f;
        f.EntityAddress    = a.entity_address;
        f.SlotIndex        = a.slot_index;
        f.ChargesCurrent   = a.charges_current;
        f.PerUseBase       = a.per_use_base;
        f.PerUseEffective  = a.per_use_effective;
        f.Usable           = a.usable != 0;
        f.Active           = a.active != 0;
        f.IsLife           = a.is_life != 0;
        f.IsMana           = a.is_mana != 0;
        f.SlotX            = a.slot_x;
        f.SlotY            = a.slot_y;
        f.ModCount         = a.mod_count;
        f.Valid            = a.valid != 0;
        f.Name             = FetchString(a.name_addr, abi);
        f.BaseType         = FetchString(a.base_type_addr, abi);
        f.Path             = FetchString(a.path_addr, abi);
        return f;
    }
};

struct Charm {
    uintptr_t   EntityAddress = 0;
    int32_t     SlotIndex = -1;
    int32_t     ChargesCurrent = 0;
    int32_t     PerUseBase = 0;
    bool        Active = false;
    int32_t     SlotX = 0;
    int32_t     SlotY = 0;
    int32_t     ModCount = 0;
    std::string Name;
    std::string BaseType;
    std::string Path;
    bool        Valid = false;

    static Charm FromAbi(const CharmAbi& a, const HostAbi* abi) {
        Charm c;
        c.EntityAddress    = a.entity_address;
        c.SlotIndex        = a.slot_index;
        c.ChargesCurrent   = a.charges_current;
        c.PerUseBase       = a.per_use_base;
        c.Active           = a.active != 0;
        c.SlotX            = a.slot_x;
        c.SlotY            = a.slot_y;
        c.ModCount         = a.mod_count;
        c.Valid            = a.valid != 0;
        c.Name             = FetchString(a.name_addr, abi);
        c.BaseType         = FetchString(a.base_type_addr, abi);
        c.Path             = FetchString(a.path_addr, abi);
        return c;
    }
};

struct InventoryGrid {
    float GridScreenX = 0;
    float GridScreenY = 0;
    float CellSize = 0;
    bool  Valid = false;
};

struct Inventory {
    int       InventoryId = 0;
    int       TotalBoxesX = 0;
    int       TotalBoxesY = 0;
    int       ServerRequestCounter = 0;
    uintptr_t Address = 0;
    InventoryGrid Grid;
    std::vector<InventoryItem> Items;

    // FromAbi populates everything except Items; use
    // InventoryService::GetItems to materialize the item list.
    static Inventory FromAbi(const InventoryAbi& a) {
        Inventory i;
        i.InventoryId          = a.inventory_id;
        i.TotalBoxesX          = a.total_boxes_x;
        i.TotalBoxesY          = a.total_boxes_y;
        i.ServerRequestCounter = a.server_request_counter;
        i.Address              = a.address;
        i.Grid.GridScreenX     = a.grid_screen_x;
        i.Grid.GridScreenY     = a.grid_screen_y;
        i.Grid.CellSize        = a.cell_size;
        i.Grid.Valid           = a.grid_valid != 0;
        return i;
    }
};

// Component addresses for one entity. Zero = entity does not carry that
// component.
struct ComponentAddresses {
    uintptr_t Render = 0, Positioned = 0, Life = 0, Targetable = 0;
    uintptr_t Chest = 0, Shrine = 0, Player = 0, Npc = 0;
    uintptr_t Buffs = 0;
    uintptr_t WorldItem = 0, AreaTransition = 0, MinimapIcon = 0;
    uintptr_t Stats = 0, Actor = 0, Animated = 0, Base = 0;
    uintptr_t Charges = 0, Mods = 0, Stack = 0, Transitionable = 0;
    uintptr_t StateMachine = 0, DiesAfterTime = 0;
    uintptr_t TriggerableBlockage = 0, OMP = 0;

    static ComponentAddresses FromAbi(const ComponentAddressesAbi& a) {
        ComponentAddresses c;
        c.Render               = a.render;
        c.Positioned           = a.positioned;
        c.Life                 = a.life;
        c.Targetable           = a.targetable;
        c.Chest                = a.chest;
        c.Shrine               = a.shrine;
        c.Player               = a.player;
        c.Npc                  = a.npc;
        c.Buffs                = a.buffs;
        c.WorldItem            = a.world_item;
        c.AreaTransition       = a.area_transition;
        c.MinimapIcon          = a.minimap_icon;
        c.Stats                = a.stats;
        c.Actor                = a.actor;
        c.Animated             = a.animated;
        c.Base                 = a.base;
        c.Charges              = a.charges;
        c.Mods                 = a.mods;
        c.Stack                = a.stack;
        c.Transitionable       = a.transitionable;
        c.StateMachine         = a.state_machine;
        c.DiesAfterTime        = a.dies_after_time;
        c.TriggerableBlockage  = a.triggerable_blockage;
        c.OMP                  = a.omp;
        return c;
    }

    bool HasRender() const              { return Render != 0; }
    bool HasPositioned() const          { return Positioned != 0; }
    bool HasLife() const                { return Life != 0; }
    bool HasTargetable() const          { return Targetable != 0; }
    bool HasChest() const               { return Chest != 0; }
    bool HasShrine() const              { return Shrine != 0; }
    bool HasPlayer() const              { return Player != 0; }
    bool HasNpc() const                 { return Npc != 0; }
    bool HasBuffs() const               { return Buffs != 0; }
    bool HasWorldItem() const           { return WorldItem != 0; }
    bool HasAreaTransition() const      { return AreaTransition != 0; }
    bool HasMinimapIcon() const         { return MinimapIcon != 0; }
    bool HasStats() const               { return Stats != 0; }
    bool HasActor() const               { return Actor != 0; }
    bool HasAnimated() const            { return Animated != 0; }
    bool HasBase() const                { return Base != 0; }
    bool HasCharges() const             { return Charges != 0; }
    bool HasMods() const                { return Mods != 0; }
    bool HasStack() const               { return Stack != 0; }
    bool HasTransitionable() const      { return Transitionable != 0; }
    bool HasStateMachine() const        { return StateMachine != 0; }
    bool HasDiesAfterTime() const       { return DiesAfterTime != 0; }
    bool HasTriggerableBlockage() const { return TriggerableBlockage != 0; }
    bool HasOMP() const                 { return OMP != 0; }
};

using EntityComponents = ComponentAddresses;

struct Entity {
    uint32_t  Id = 0;
    uintptr_t Address = 0;
    uintptr_t EntityDetailsAddress = 0;
    uintptr_t RenderComponentAddress = 0;
    bool      IsValid = false;
    PluginSDK::EntityType    EntityType    = PluginSDK::EntityType::Unidentified;
    PluginSDK::EntitySubtype EntitySubtype = PluginSDK::EntitySubtype::Unidentified;
    PluginSDK::EntityState   EntityState   = PluginSDK::EntityState::None;
    int       Rarity = 0;
    int       Reaction = 0;
    float     GridPositionX = 0, GridPositionY = 0;
    float     TerrainHeight = 0;
    float     WorldX = 0, WorldY = 0, WorldZ = 0;
    float     ModelBoundsZ = 0;
    std::wstring Path;
    std::wstring PlayerName;
    std::string  TgtPath;
    int       CurrentHP = 0, MaxHP = 0;
    int       CurrentES = 0, MaxES = 0;
    bool      IsSleeping = false;
    bool      IsChestOpened = false;
    PluginSDK::NearbyZone Zone = PluginSDK::NearbyZone::None;
    ComponentAddresses Components;

    static Entity FromAbi(const EntityInfoAbi& e,
                          const ComponentAddressesAbi& c,
                          const HostAbi* abi) {
        Entity x;
        x.Id                       = e.id;
        x.Address                  = e.address;
        x.EntityDetailsAddress     = e.entity_details_address;
        x.RenderComponentAddress   = e.render_component_address;
        x.EntityType               = static_cast<PluginSDK::EntityType>(e.entity_type);
        x.EntitySubtype            = static_cast<PluginSDK::EntitySubtype>(e.entity_subtype);
        x.EntityState              = static_cast<PluginSDK::EntityState>(e.entity_state);
        x.Rarity                   = e.rarity;
        x.Reaction                 = e.reaction;
        x.Zone                     = static_cast<PluginSDK::NearbyZone>(e.zone);
        x.GridPositionX            = e.grid_x;
        x.GridPositionY            = e.grid_y;
        x.TerrainHeight            = e.terrain_height;
        x.WorldX                   = e.world_x;
        x.WorldY                   = e.world_y;
        x.WorldZ                   = e.world_z;
        x.ModelBoundsZ             = e.model_bounds_z;
        x.CurrentHP                = e.hp_current;
        x.MaxHP                    = e.hp_max;
        x.CurrentES                = e.es_current;
        x.MaxES                    = e.es_max;
        x.IsValid                  = e.is_valid != 0;
        x.IsSleeping               = e.is_sleeping != 0;
        x.IsChestOpened            = e.is_chest_opened != 0;
        x.Path                     = FetchWString(e.path_addr, abi);
        x.PlayerName               = FetchWString(e.player_name_addr, abi);
        x.TgtPath                  = FetchString(e.tgt_path_addr, abi);
        x.Components               = ComponentAddresses::FromAbi(c);
        return x;
    }
};

struct Snapshot {
    GameState   State = GameState::NotLoaded;
    std::string CurrentAreaName;
    std::string CurrentAreaHash;
    int         CurrentAreaLevel = 0;
    bool        IsTown = false;
    bool        IsHideout = false;
    bool        IsPaused = false;
    bool        IsSkillTreeVisible = false;
    float       WorldToGridConvertor = 0;
    Entity              Player;
    std::vector<Entity> Entities;
    MapData     LargeMap;
    MapData     MiniMap;
    PluginSDK::Vitals Vitals;
    int         ScreenWidth = 0, ScreenHeight = 0;
    DWORD       ProcessId = 0;
    HWND        GameWindow = nullptr;
    bool        GameWindowForeground = true;
    bool        IsAttached = false;
    bool        IsWindowValid = false;
    uint64_t    LastUpdateTime = 0;
    uint64_t    AreaChangeCounter = 0;
    float       WorldToScreenMatrix[16] = {};

    static Snapshot FromAbi(const SnapshotAbi& a, const HostAbi* abi);
};

inline Snapshot Snapshot::FromAbi(const SnapshotAbi& a, const HostAbi* abi) {
    Snapshot s;
    s.State                = static_cast<GameState>(a.game_state);
    s.CurrentAreaLevel     = a.current_area_level;
    s.IsTown               = a.is_town != 0;
    s.IsHideout            = a.is_hideout != 0;
    s.IsPaused             = a.is_paused != 0;
    s.IsSkillTreeVisible   = a.is_skill_tree_visible != 0;
    s.IsAttached           = a.is_attached != 0;
    s.IsWindowValid        = a.is_window_valid != 0;
    s.GameWindowForeground = a.game_window_foreground != 0;
    s.ScreenWidth          = a.screen_width;
    s.ScreenHeight         = a.screen_height;
    s.WorldToGridConvertor = a.world_to_grid_convertor;
    s.LastUpdateTime       = a.last_update_time;
    s.AreaChangeCounter    = a.area_change_counter;
    s.ProcessId            = a.process_id;
    s.GameWindow           = a.game_window;
    s.LargeMap             = MapData::FromAbi(a.large_map);
    s.MiniMap              = MapData::FromAbi(a.mini_map);
    s.Vitals               = PluginSDK::Vitals::FromAbi(a.vitals);
    s.Player               = Entity::FromAbi(a.player, a.player_components, abi);
    s.CurrentAreaName      = FetchString(a.area_name_addr, abi);
    s.CurrentAreaHash      = FetchString(a.area_hash_addr, abi);
    for (int i = 0; i < 16; ++i) {
        s.WorldToScreenMatrix[i] = a.world_to_screen_matrix[i];
    }
    if (abi && abi->entities.enumerate) {
        struct Ctx {
            std::vector<Entity>* out;
            const HostAbi*    host;
        };
        Ctx ctx{ &s.Entities, abi };
        abi->entities.enumerate(
            [](const EntityInfoAbi* ei, const ComponentAddressesAbi* ca, void* ud) -> int32_t {
                auto* c = static_cast<Ctx*>(ud);
                c->out->push_back(Entity::FromAbi(*ei, *ca, c->host));
                return 1;
            },
            &ctx);
    }
    return s;
}

struct ScreenSize {
    float Width = 0;
    float Height = 0;
};

// Service wrappers below all follow the same C-trampoline pattern: each
// `Enumerate*` allocates a captures struct on the stack, the C ABI receives
// a `+[](...) -> int32_t { ... }` lambda (no captures, so it decays to a
// function pointer), and that lambda reinterprets userdata back to the
// captures struct. Returning 1 from the trampoline keeps enumeration going;
// 0 stops it.

class GameService {
    const GameServiceAbi* m_abi = nullptr;
    const HostAbi*     m_host = nullptr;
public:
    void Init(const GameServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    Snapshot GetSnapshot() const {
        SnapshotAbi raw{};
        if (m_abi && m_abi->get_snapshot) m_abi->get_snapshot(&raw);
        return Snapshot::FromAbi(raw, m_host);
    }

    GameState GetState() const {
        return static_cast<GameState>(
            (m_abi && m_abi->get_state) ? m_abi->get_state() : PSDK_GAME_STATE_NOT_LOADED);
    }

    bool IsAttached()    const { return m_abi && m_abi->is_attached     && m_abi->is_attached()     != 0; }
    bool IsInGame()      const { return m_abi && m_abi->is_in_game      && m_abi->is_in_game()      != 0; }
    bool IsForeground()  const { return m_abi && m_abi->is_foreground   && m_abi->is_foreground()   != 0; }
    bool IsMenuVisible() const { return m_abi && m_abi->is_menu_visible && m_abi->is_menu_visible() != 0; }
    bool IsOverlayMode() const { return m_abi && m_abi->is_overlay_mode && m_abi->is_overlay_mode() != 0; }
    DWORD GetProcessId() const { return (m_abi && m_abi->get_process_id) ? m_abi->get_process_id() : 0; }
    HWND  GetGameWindow() const {
        return (m_abi && m_abi->get_game_window) ? m_abi->get_game_window() : nullptr;
    }

    ScreenSize GetScreenSize() const {
        ScreenSize s;
        if (m_abi && m_abi->get_screen_size) m_abi->get_screen_size(&s.Width, &s.Height);
        return s;
    }
};

class EntitiesService {
    const EntitiesServiceAbi* m_abi = nullptr;
    const HostAbi*         m_host = nullptr;
public:
    void Init(const EntitiesServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    void Enumerate(std::function<bool(const Entity&)> cb) const {
        if (!m_abi || !m_abi->enumerate) return;
        struct Ctx {
            std::function<bool(const Entity&)>* cb;
            const HostAbi*                   host;
        };
        Ctx ctx{ &cb, m_host };
        m_abi->enumerate(
            [](const EntityInfoAbi* e, const ComponentAddressesAbi* c, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                return (*p->cb)(Entity::FromAbi(*e, *c, p->host)) ? 1 : 0;
            },
            &ctx);
    }

    Entity GetPlayer() const {
        if (!m_abi || !m_abi->get_player) return {};
        EntityInfoAbi e{};
        ComponentAddressesAbi c{};
        m_abi->get_player(&e, &c);
        return Entity::FromAbi(e, c, m_host);
    }

    std::optional<Entity> FindById(uint32_t id) const {
        if (!m_abi || !m_abi->find_by_id) return std::nullopt;
        EntityInfoAbi e{};
        ComponentAddressesAbi c{};
        if (!m_abi->find_by_id(id, &e, &c)) return std::nullopt;
        return Entity::FromAbi(e, c, m_host);
    }

    void Watch(uint32_t id)   const { if (m_abi && m_abi->watch)   m_abi->watch(id); }
    void Unwatch(uint32_t id) const { if (m_abi && m_abi->unwatch) m_abi->unwatch(id); }

    bool IsWatched(uint32_t id) const {
        return m_abi && m_abi->is_watched && m_abi->is_watched(id) != 0;
    }

    std::optional<ComponentAddresses> GetWatchedComponents(uint32_t id) const {
        if (!m_abi || !m_abi->get_watched_components) return std::nullopt;
        ComponentAddressesAbi raw{};
        if (!m_abi->get_watched_components(id, &raw)) return std::nullopt;
        return ComponentAddresses::FromAbi(raw);
    }

    // Resolve a WorldItem container entity into its inner item entity.
    // Returns std::nullopt if the address is not a WorldItem container,
    // the inner item is not yet resolved, or any read fails.
    // The returned Entity has its Components field populated with the
    // inner item's component addresses (Mods, Base, Stack, Sockets, ...).
    //
    // NOTE: this routes through m_host (HostAbi top-level pointer), not
    // m_abi (EntitiesServiceAbi), so we can grow the SDK surface without
    // shifting any inner-service-struct offsets — preserves v6 ABI compat.
    std::optional<Entity> GetWorldItemInner(uintptr_t containerAddr) const {
        if (!m_host || !m_host->get_world_item_inner || containerAddr == 0)
            return std::nullopt;
        EntityInfoAbi e{};
        ComponentAddressesAbi c{};
        if (!m_host->get_world_item_inner(containerAddr, &e, &c))
            return std::nullopt;
        return Entity::FromAbi(e, c, m_host);
    }
};

class ComponentsService {
    const ComponentsServiceAbi* m_abi = nullptr;
    const HostAbi*           m_host = nullptr;
public:
    void Init(const ComponentsServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    Life ReadLife(uintptr_t addr) const {
        LifeAbi a{};
        if (m_abi && m_abi->read_life && m_abi->read_life(addr, &a))
            return Life::FromAbi(a);
        return {};
    }
    Render ReadRender(uintptr_t addr) const {
        RenderAbi a{};
        if (m_abi && m_abi->read_render && m_abi->read_render(addr, &a))
            return Render::FromAbi(a);
        return {};
    }
    Positioned ReadPositioned(uintptr_t addr) const {
        PositionedAbi a{};
        if (m_abi && m_abi->read_positioned && m_abi->read_positioned(addr, &a))
            return Positioned::FromAbi(a);
        return {};
    }
    Targetable ReadTargetable(uintptr_t addr) const {
        TargetableAbi a{};
        if (m_abi && m_abi->read_targetable && m_abi->read_targetable(addr, &a))
            return Targetable::FromAbi(a);
        return {};
    }
    Chest ReadChest(uintptr_t addr) const {
        ChestAbi a{};
        if (m_abi && m_abi->read_chest && m_abi->read_chest(addr, &a))
            return Chest::FromAbi(a);
        return {};
    }
    Shrine ReadShrine(uintptr_t addr) const {
        ShrineAbi a{};
        if (m_abi && m_abi->read_shrine && m_abi->read_shrine(addr, &a))
            return Shrine::FromAbi(a);
        return {};
    }
    Stack ReadStack(uintptr_t addr) const {
        StackAbi a{};
        if (m_abi && m_abi->read_stack && m_abi->read_stack(addr, &a))
            return Stack::FromAbi(a);
        return {};
    }
    Charges ReadCharges(uintptr_t addr) const {
        ChargesAbi a{};
        if (m_abi && m_abi->read_charges && m_abi->read_charges(addr, &a))
            return Charges::FromAbi(a);
        return {};
    }
    Player ReadPlayer(uintptr_t addr) const {
        PlayerAbi a{};
        if (m_abi && m_abi->read_player && m_abi->read_player(addr, &a))
            return Player::FromAbi(a, m_host);
        return {};
    }
    Animated ReadAnimated(uintptr_t addr) const {
        AnimatedAbi a{};
        if (m_abi && m_abi->read_animated && m_abi->read_animated(addr, &a))
            return Animated::FromAbi(a, m_host);
        return {};
    }
    Transitionable ReadTransitionable(uintptr_t addr) const {
        TransitionableAbi a{};
        if (m_abi && m_abi->read_transitionable && m_abi->read_transitionable(addr, &a))
            return Transitionable::FromAbi(a);
        return {};
    }
    TriggerableBlockage ReadTriggerableBlockage(uintptr_t addr) const {
        TriggerableBlockageAbi a{};
        if (m_abi && m_abi->read_triggerable_blockage
            && m_abi->read_triggerable_blockage(addr, &a))
            return TriggerableBlockage::FromAbi(a);
        return {};
    }
    MinimapIcon ReadMinimapIcon(uintptr_t addr) const {
        MinimapIconAbi a{};
        if (m_abi && m_abi->read_minimap_icon && m_abi->read_minimap_icon(addr, &a))
            return MinimapIcon::FromAbi(a);
        return {};
    }
    StateMachine ReadStateMachine(uintptr_t addr) const {
        StateMachineAbi a{};
        if (m_abi && m_abi->read_state_machine && m_abi->read_state_machine(addr, &a))
            return StateMachine::FromAbi(a);
        return {};
    }
    Base ReadBase(uintptr_t addr) const {
        BaseAbi a{};
        if (m_abi && m_abi->read_base && m_abi->read_base(addr, &a))
            return Base::FromAbi(a, m_host);
        return {};
    }
    Mods ReadMods(uintptr_t addr) const {
        ModsAbi a{};
        if (m_abi && m_abi->read_mods && m_abi->read_mods(addr, &a))
            return Mods::FromAbi(a);
        return {};
    }
    Stats ReadStats(uintptr_t addr) const {
        StatsAbi a{};
        if (m_abi && m_abi->read_stats && m_abi->read_stats(addr, &a))
            return Stats::FromAbi(a);
        return {};
    }
    Buffs ReadBuffs(uintptr_t addr) const {
        BuffsAbi a{};
        if (m_abi && m_abi->read_buffs && m_abi->read_buffs(addr, &a))
            return Buffs::FromAbi(a);
        return {};
    }
    Actor ReadActor(uintptr_t addr) const {
        ActorAbi a{};
        if (m_abi && m_abi->read_actor && m_abi->read_actor(addr, &a))
            return Actor::FromAbi(a, m_host);
        return {};
    }
    Npc ReadNpc(uintptr_t addr) const {
        NpcAbi a{};
        if (m_abi && m_abi->read_npc && m_abi->read_npc(addr, &a))
            return Npc::FromAbi(a);
        return {};
    }
    DiesAfterTime ReadDiesAfterTime(uintptr_t addr) const {
        DiesAfterTimeAbi a{};
        if (m_abi && m_abi->read_dies_after_time && m_abi->read_dies_after_time(addr, &a))
            return DiesAfterTime::FromAbi(a);
        return {};
    }

    std::vector<Buff> EnumerateBuffs(uintptr_t buffsAddr) const {
        std::vector<Buff> out;
        if (!m_abi || !m_abi->enumerate_buffs) return out;
        struct Ctx { std::vector<Buff>* out; const HostAbi* host; };
        Ctx c{ &out, m_host };
        m_abi->enumerate_buffs(buffsAddr,
            [](const BuffAbi* b, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                p->out->push_back(Buff::FromAbi(*b, p->host));
                return 1;
            },
            &c);
        return out;
    }

    std::vector<ActiveSkill> EnumerateActiveSkills(uintptr_t actorAddr) const {
        std::vector<ActiveSkill> out;
        if (!m_abi || !m_abi->enumerate_active_skills) return out;
        struct Ctx { std::vector<ActiveSkill>* out; const HostAbi* host; };
        Ctx c{ &out, m_host };
        m_abi->enumerate_active_skills(actorAddr,
            [](const ActiveSkillAbi* s, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                p->out->push_back(ActiveSkill::FromAbi(*s, p->host));
                return 1;
            },
            &c);
        return out;
    }

    struct StatEntry {
        int        Key = 0;
        int        Value = 0;
        StatSource Source = StatSource::Items;
    };
    std::vector<StatEntry> EnumerateStats(uintptr_t statsAddr) const {
        std::vector<StatEntry> out;
        if (!m_abi || !m_abi->enumerate_stats) return out;
        struct Ctx { std::vector<StatEntry>* out; };
        Ctx c{ &out };
        m_abi->enumerate_stats(statsAddr,
            [](int32_t key, int32_t value, PsdkStatSource src, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                StatEntry e;
                e.Key    = key;
                e.Value  = value;
                e.Source = static_cast<StatSource>(src);
                p->out->push_back(e);
                return 1;
            },
            &c);
        return out;
    }

    std::vector<Mod> EnumerateItemMods(uintptr_t modsAddr) const {
        std::vector<Mod> out;
        if (!m_abi || !m_abi->enumerate_item_mods) return out;
        struct Ctx { std::vector<Mod>* out; const HostAbi* host; };
        Ctx c{ &out, m_host };
        m_abi->enumerate_item_mods(modsAddr,
            [](const ModAbi* m, PsdkModKind kind, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                p->out->push_back(Mod::FromAbi(*m, static_cast<ModKind>(kind), p->host));
                return 1;
            },
            &c);
        return out;
    }

    float GetHealthPercent(uintptr_t lifeAddr) const {
        Life l = ReadLife(lifeAddr);
        return (l.Valid && l.Health.Total > 0)
            ? 100.f * (float)l.Health.Current / (float)l.Health.Total : 0.f;
    }
    float GetEsPercent(uintptr_t lifeAddr) const {
        Life l = ReadLife(lifeAddr);
        return (l.Valid && l.EnergyShield.Total > 0)
            ? 100.f * (float)l.EnergyShield.Current / (float)l.EnergyShield.Total : 0.f;
    }
    float GetManaPercent(uintptr_t lifeAddr) const {
        Life l = ReadLife(lifeAddr);
        return (l.Valid && l.Mana.Total > 0)
            ? 100.f * (float)l.Mana.Current / (float)l.Mana.Total : 0.f;
    }
    bool IsAlive(uintptr_t lifeAddr) const {
        Life l = ReadLife(lifeAddr);
        return l.Valid && l.Health.Current > 0;
    }
    bool GetWorldPosition(uintptr_t renderAddr, float& x, float& y, float& z) const {
        Render r = ReadRender(renderAddr);
        if (!r.Valid) return false;
        x = r.WorldX;
        y = r.WorldY;
        z = r.WorldZ;
        return true;
    }
    int GetItemRarity(uintptr_t modsAddr) const {
        Mods m = ReadMods(modsAddr);
        return m.Valid ? m.Rarity : 0;
    }
    bool IsItemIdentified(uintptr_t modsAddr) const {
        Mods m = ReadMods(modsAddr);
        return m.Valid && m.IsIdentified;
    }
    int GetStackCount(uintptr_t stackAddr) const {
        Stack s = ReadStack(stackAddr);
        return s.Valid ? s.CurrentSize : 0;
    }
    std::string GetPlayerName(uintptr_t playerAddr) const {
        return ReadPlayer(playerAddr).Name;
    }
    bool IsChestOpened(uintptr_t chestAddr) const {
        Chest c = ReadChest(chestAddr);
        return c.Valid && c.IsOpened;
    }
};

class InventoryService {
    const InventoryServiceAbi* m_abi = nullptr;
    const HostAbi*          m_host = nullptr;
public:
    void Init(const InventoryServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    void Scan(int inventoryId) const {
        if (m_abi && m_abi->scan) m_abi->scan(inventoryId);
    }

    Inventory Get(int inventoryId) const {
        InventoryAbi raw{};
        if (m_abi && m_abi->get && m_abi->get(inventoryId, &raw)) {
            Inventory inv = Inventory::FromAbi(raw);
            inv.Items = GetItems(inventoryId);
            return inv;
        }
        return {};
    }

    std::vector<InventoryItem> GetItems(int inventoryId) const {
        std::vector<InventoryItem> out;
        if (!m_abi || !m_abi->enumerate_items) return out;
        struct Ctx { std::vector<InventoryItem>* out; const HostAbi* host; };
        Ctx c{ &out, m_host };
        m_abi->enumerate_items(inventoryId,
            [](const InventoryItemAbi* it, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                p->out->push_back(InventoryItem::FromAbi(*it, p->host));
                return 1;
            },
            &c);
        return out;
    }

    std::vector<Inventory> GetAll() const {
        std::vector<Inventory> out;
        if (!m_abi || !m_abi->enumerate) return out;
        struct Ctx { std::vector<Inventory>* out; const InventoryService* self; };
        Ctx c{ &out, this };
        m_abi->enumerate(
            [](const InventoryAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                Inventory inv = Inventory::FromAbi(*a);
                inv.Items = p->self->GetItems(a->inventory_id);
                p->out->push_back(std::move(inv));
                return 1;
            },
            &c);
        return out;
    }

    const char* GetName(int inventoryId) const {
        return (m_abi && m_abi->get_name) ? m_abi->get_name(inventoryId) : "";
    }

    int ReadItemRarity(uintptr_t entityAddr) const {
        return (m_abi && m_abi->read_item_rarity) ? m_abi->read_item_rarity(entityAddr) : 0;
    }
    int ReadItemStackCount(uintptr_t entityAddr) const {
        return (m_abi && m_abi->read_item_stack_count)
            ? m_abi->read_item_stack_count(entityAddr) : 0;
    }

    std::string ReadItemBaseTypeName(uintptr_t entityAddr) const {
        if (!m_abi || !m_abi->read_item_base_type_name) return {};
        size_t needed = m_abi->read_item_base_type_name(entityAddr, nullptr, 0);
        if (needed <= 1) return {};
        std::string s;
        s.resize(needed - 1);
        m_abi->read_item_base_type_name(entityAddr, s.data(), needed);
        return s;
    }

    std::string ReadItemUniqueName(uintptr_t entityAddr) const {
        if (!m_abi || !m_abi->read_item_unique_name) return {};
        size_t needed = m_abi->read_item_unique_name(entityAddr, nullptr, 0);
        if (needed <= 1) return {};
        std::string s;
        s.resize(needed - 1);
        m_abi->read_item_unique_name(entityAddr, s.data(), needed);
        return s;
    }

    std::string ReadItemPath(uintptr_t entityAddr) const {
        if (!m_abi || !m_abi->read_item_path) return {};
        size_t needed = m_abi->read_item_path(entityAddr, nullptr, 0);
        if (needed <= 1) return {};
        std::string s;
        s.resize(needed - 1);
        m_abi->read_item_path(entityAddr, s.data(), needed);
        return s;
    }

    ItemMods ReadItemMods(uintptr_t entityAddr) const {
        ItemMods im{};
        if (!m_abi || !m_abi->read_item_mods_summary) return im;
        ItemModsSummaryAbi s{};
        if (!m_abi->read_item_mods_summary(entityAddr, &s)) return im;
        im.Rarity          = s.rarity;
        im.ItemLevel       = s.item_level;
        im.RequiredLevel   = s.required_level;
        im.CraftedModCount = s.crafted_mod_count;
        im.IsIdentified    = s.is_identified  != 0;
        im.IsCorrupted     = s.is_corrupted   != 0;
        im.IsSplit         = s.is_split       != 0;
        im.IsMirrored      = s.is_mirrored    != 0;
        im.IsRelic         = s.is_relic       != 0;
        im.IsSynthesised   = s.is_synthesised != 0;
        im.Valid           = s.valid != 0;

        if (m_abi->enumerate_item_mods_by_entity) {
            struct Ctx { ItemMods* out; const HostAbi* host; };
            Ctx c{ &im, m_host };
            m_abi->enumerate_item_mods_by_entity(entityAddr,
                [](const ModAbi* mod, PsdkModKind kind, void* ud) -> int32_t {
                    auto* p = static_cast<Ctx*>(ud);
                    Mod m = Mod::FromAbi(*mod, static_cast<ModKind>(kind), p->host);
                    switch (kind) {
                        case PSDK_MOD_KIND_IMPLICIT:
                            p->out->ImplicitMods.push_back(std::move(m));
                            break;
                        case PSDK_MOD_KIND_EXPLICIT:
                            p->out->ExplicitMods.push_back(std::move(m));
                            break;
                        case PSDK_MOD_KIND_ENCHANT:
                            p->out->EnchantMods.push_back(std::move(m));
                            break;
                        case PSDK_MOD_KIND_HELLSCAPE:
                            p->out->HellscapeMods.push_back(std::move(m));
                            break;
                        case PSDK_MOD_KIND_CRUCIBLE:
                            p->out->CrucibleMods.push_back(std::move(m));
                            break;
                        default:
                            break;
                    }
                    return 1;
                },
                &c);
        }
        return im;
    }
};

class UiService {
    const UiServiceAbi* m_abi = nullptr;
    const HostAbi*   m_host = nullptr;
public:
    void Init(const UiServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    UiElement Read(uintptr_t addr) const {
        UiElementAbi a{};
        if (m_abi && m_abi->read && m_abi->read(addr, &a))
            return UiElement::FromAbi(a);
        return {};
    }

    std::vector<uintptr_t> GetChildren(uintptr_t addr) const {
        std::vector<uintptr_t> out;
        if (!m_abi || !m_abi->enumerate_children) return out;
        struct Ctx { std::vector<uintptr_t>* out; };
        Ctx c{ &out };
        m_abi->enumerate_children(addr,
            [](uintptr_t child, int32_t /*idx*/, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                p->out->push_back(child);
                return 1;
            },
            &c);
        return out;
    }

    uintptr_t GetChildAt(uintptr_t addr, int index) const {
        return (m_abi && m_abi->get_child_at) ? m_abi->get_child_at(addr, index) : 0;
    }

    uintptr_t FollowPath(uintptr_t root, const int* indices, int count) const {
        if (!m_abi || !m_abi->follow_path) return 0;
        return m_abi->follow_path(root,
                                  reinterpret_cast<const int32_t*>(indices),
                                  count);
    }

    bool IsVisible(uintptr_t addr) const {
        return m_abi && m_abi->is_visible && m_abi->is_visible(addr) != 0;
    }

    std::string GetStringId(uintptr_t addr) const {
        if (!m_abi || !m_abi->get_string_id) return {};
        size_t needed = m_abi->get_string_id(addr, nullptr, 0);
        if (needed <= 1) return {};
        std::string s;
        s.resize(needed - 1);
        m_abi->get_string_id(addr, s.data(), needed);
        return s;
    }

    std::string GetText(uintptr_t addr) const {
        if (!m_abi || !m_abi->get_text) return {};
        size_t needed = m_abi->get_text(addr, nullptr, 0);
        if (needed <= 1) return {};
        std::string s;
        s.resize(needed - 1);
        m_abi->get_text(addr, s.data(), needed);
        return s;
    }

    bool ComputeScreenRect(uintptr_t addr, float& x, float& y, float& w, float& h) const {
        if (!m_abi || !m_abi->compute_screen_rect) return false;
        return m_abi->compute_screen_rect(addr, &x, &y, &w, &h) != 0;
    }

    uintptr_t GetGameUiRoot() const {
        return (m_abi && m_abi->get_game_ui_root) ? m_abi->get_game_ui_root() : 0;
    }
    uintptr_t GetUiRoot() const {
        return (m_abi && m_abi->get_ui_root) ? m_abi->get_ui_root() : 0;
    }
    int GetCullValue() const {
        return (m_abi && m_abi->get_cull_value) ? m_abi->get_cull_value() : 0;
    }

    uintptr_t FindPanelByStringId(uintptr_t parent, const char* stringId) const {
        return (m_abi && m_abi->find_panel_by_string_id)
            ? m_abi->find_panel_by_string_id(parent, stringId) : 0;
    }
};

class RenderService {
    const RenderServiceAbi* m_abi = nullptr;
    const HostAbi*       m_host = nullptr;
public:
    void Init(const RenderServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    bool WorldToScreen(float wx, float wy, float wz, float& sx, float& sy) const {
        return m_abi && m_abi->world_to_screen
            && m_abi->world_to_screen(wx, wy, wz, &sx, &sy) != 0;
    }
    bool GridToLargeMap(float gx, float gy, float worldZ, float& sx, float& sy) const {
        return m_abi && m_abi->grid_to_large_map
            && m_abi->grid_to_large_map(gx, gy, worldZ, &sx, &sy) != 0;
    }
    bool GridToMiniMap(float gx, float gy, float worldZ, float& sx, float& sy) const {
        return m_abi && m_abi->grid_to_mini_map
            && m_abi->grid_to_mini_map(gx, gy, worldZ, &sx, &sy) != 0;
    }

    MapTransform GetLargeMapTransform() const {
        MapTransformAbi raw{};
        if (m_abi && m_abi->get_large_map_transform)
            m_abi->get_large_map_transform(&raw);
        return MapTransform::FromAbi(raw);
    }
    MapTransform GetMiniMapTransform() const {
        MapTransformAbi raw{};
        if (m_abi && m_abi->get_mini_map_transform)
            m_abi->get_mini_map_transform(&raw);
        return MapTransform::FromAbi(raw);
    }
};

// RAII handle around a terrain-grid snapshot. Move-only: the held buffer
// must be released exactly once via the ABI's release() callback. Always
// bound-check indices against SizeBytes() / ElementCount() — the underlying
// storage is the host's atomic snapshot, not a live pointer.
class WalkableGridHandle {
    WalkableGridHandleAbi m_h{};
public:
    WalkableGridHandle() = default;
    explicit WalkableGridHandle(const WalkableGridHandleAbi& h) : m_h(h) {}
    WalkableGridHandle(const WalkableGridHandle&) = delete;
    WalkableGridHandle& operator=(const WalkableGridHandle&) = delete;
    WalkableGridHandle(WalkableGridHandle&& o) noexcept : m_h(o.m_h) { o.m_h = {}; }
    WalkableGridHandle& operator=(WalkableGridHandle&& o) noexcept {
        if (this != &o) {
            Reset();
            m_h = o.m_h;
            o.m_h = {};
        }
        return *this;
    }
    ~WalkableGridHandle() { Reset(); }

    void Reset() {
        if (m_h.release && m_h.opaque) m_h.release(m_h.opaque);
        m_h = {};
    }
    const uint8_t* Data() const { return m_h.data; }
    int Width()  const { return m_h.width; }
    int Height() const { return m_h.height; }
    bool Valid() const { return m_h.data != nullptr; }

    // Buffer size in bytes. Walkable grid is 4-bit-per-tile packed
    // (two tiles per byte), so size = (Width * Height) / 2.
    size_t SizeBytes() const {
        if (m_h.width <= 0 || m_h.height <= 0) return 0;
        return (static_cast<size_t>(m_h.width)
              * static_cast<size_t>(m_h.height)) / 2;
    }
};

class HeightGridHandle {
    HeightGridHandleAbi m_h{};
public:
    HeightGridHandle() = default;
    explicit HeightGridHandle(const HeightGridHandleAbi& h) : m_h(h) {}
    HeightGridHandle(const HeightGridHandle&) = delete;
    HeightGridHandle& operator=(const HeightGridHandle&) = delete;
    HeightGridHandle(HeightGridHandle&& o) noexcept : m_h(o.m_h) { o.m_h = {}; }
    HeightGridHandle& operator=(HeightGridHandle&& o) noexcept {
        if (this != &o) {
            Reset();
            m_h = o.m_h;
            o.m_h = {};
        }
        return *this;
    }
    ~HeightGridHandle() { Reset(); }

    void Reset() {
        if (m_h.release && m_h.opaque) m_h.release(m_h.opaque);
        m_h = {};
    }
    const float* Data() const { return m_h.data; }
    int Width()  const { return m_h.width; }
    int Height() const { return m_h.height; }
    bool Valid() const { return m_h.data != nullptr; }

    // Height grid is one float per tile, unpacked.
    size_t ElementCount() const {
        if (m_h.width <= 0 || m_h.height <= 0) return 0;
        return static_cast<size_t>(m_h.width)
             * static_cast<size_t>(m_h.height);
    }
    size_t SizeBytes() const { return ElementCount() * sizeof(float); }
};

class TerrainService {
    const TerrainServiceAbi* m_abi = nullptr;
    const HostAbi*        m_host = nullptr;
public:
    void Init(const TerrainServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    WalkableGridHandle GetWalkableGrid() const {
        WalkableGridHandleAbi raw{};
        if (m_abi && m_abi->get_walkable_grid) m_abi->get_walkable_grid(&raw);
        return WalkableGridHandle{ raw };
    }
    HeightGridHandle GetHeightGrid() const {
        HeightGridHandleAbi raw{};
        if (m_abi && m_abi->get_height_grid) m_abi->get_height_grid(&raw);
        return HeightGridHandle{ raw };
    }

    bool IsWalkable(int gridX, int gridY) const {
        return m_abi && m_abi->is_walkable && m_abi->is_walkable(gridX, gridY) != 0;
    }
    float GetTerrainHeight(int gridX, int gridY) const {
        return (m_abi && m_abi->get_terrain_height)
            ? m_abi->get_terrain_height(gridX, gridY) : 0.f;
    }
    float GetWorldToGridConvertor() const {
        return (m_abi && m_abi->get_world_to_grid_convertor)
            ? m_abi->get_world_to_grid_convertor() : 0.f;
    }

    void EnumerateTgtLocations(std::function<bool(const TgtLocation&)> cb) const {
        if (!m_abi || !m_abi->enumerate_tgt_locations) return;
        struct Ctx {
            std::function<bool(const TgtLocation&)>* cb;
            const HostAbi*                        host;
        };
        Ctx c{ &cb, m_host };
        m_abi->enumerate_tgt_locations(
            [](const TgtLocationAbi* loc, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                return (*p->cb)(TgtLocation::FromAbi(*loc, p->host)) ? 1 : 0;
            },
            &c);
    }
};

class MemoryService {
    const MemoryServiceAbi* m_abi = nullptr;
    const HostAbi*       m_host = nullptr;
public:
    void Init(const MemoryServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    bool Read(uintptr_t addr, void* buf, size_t size) const {
        return m_abi && m_abi->read && m_abi->read(addr, buf, size) != 0;
    }

    std::string  ReadString(uintptr_t addr)  const { return FetchString(addr, m_host); }
    std::wstring ReadWString(uintptr_t addr) const { return FetchWString(addr, m_host); }

    std::wstring ReadStdWString(uintptr_t containerAddr) const {
        if (!m_abi || !m_abi->read_std_wstring) return {};
        size_t needed = m_abi->read_std_wstring(containerAddr, nullptr, 0);
        if (needed <= 1) return {};
        std::wstring s;
        s.resize(needed - 1);
        m_abi->read_std_wstring(containerAddr, s.data(), needed);
        return s;
    }

    // ReadStdVector: caller knows element size + count expectations.
    // Returns the raw bytes (count * elementSize) so the caller can reinterpret_cast.
    std::vector<uint8_t> ReadStdVector(uintptr_t containerAddr, int elementSize,
                                       int maxElements = 1024) const {
        if (!m_abi || !m_abi->read_std_vector || elementSize <= 0) return {};
        std::vector<uint8_t> buf(static_cast<size_t>(maxElements) * elementSize);
        int32_t count = maxElements;
        if (!m_abi->read_std_vector(containerAddr, elementSize, buf.data(), &count))
            return {};
        if (count < 0) count = 0;
        buf.resize(static_cast<size_t>(count) * elementSize);
        return buf;
    }

    uintptr_t GetBaseAddress() const {
        return (m_abi && m_abi->get_base_address) ? m_abi->get_base_address() : 0;
    }
    uintptr_t GetModuleSize() const {
        return (m_abi && m_abi->get_module_size) ? m_abi->get_module_size() : 0;
    }
    uintptr_t GetPatternAddress(const char* patternName) const {
        return (m_abi && m_abi->get_pattern_address)
            ? m_abi->get_pattern_address(patternName) : 0;
    }
};

class LogService {
    const LogServiceAbi* m_abi = nullptr;
    const HostAbi*    m_host = nullptr;
public:
    void Init(const LogServiceAbi* abi, const HostAbi* host) {
        m_abi = abi;
        m_host = host;
    }

    void Log(const char* level, const char* message) const {
        if (m_abi && m_abi->log) m_abi->log(level, message ? message : "");
    }
    void Debug(const char* message) const { Log("Debug",   message); }
    void Info (const char* message) const { Log("Info",    message); }
    void Warn (const char* message) const { Log("Warning", message); }
    void Error(const char* message) const { Log("Error",   message); }
};

// Event subscriptions store heap-allocated std::function holders keyed by
// token. The destructor unsubscribes any remaining tokens and frees the
// holders — the EventsService instance must outlive every callback it owns.
class EventsService {
    const EventsServiceAbi* m_abi = nullptr;
    std::map<uint64_t, std::function<void()>*> m_holders;
public:
    struct Token {
        uint64_t Value = 0;
        bool Valid() const { return Value != 0; }
    };

    void Init(const EventsServiceAbi* abi, const HostAbi* /*host*/) {
        m_abi = abi;
    }

    EventsService() = default;
    EventsService(const EventsService&) = delete;
    EventsService& operator=(const EventsService&) = delete;
    EventsService(EventsService&&) = delete;
    EventsService& operator=(EventsService&&) = delete;

    ~EventsService() {
        if (m_abi && m_abi->unsubscribe) {
            for (auto& kv : m_holders) {
                m_abi->unsubscribe(kv.first);
                delete kv.second;
            }
        } else {
            for (auto& kv : m_holders) {
                delete kv.second;
            }
        }
        m_holders.clear();
    }

    Token Subscribe(EventKind kind, std::function<void()> cb) {
        if (!m_abi || !m_abi->subscribe) return {};
        auto* holder = new std::function<void()>(std::move(cb));
        uint64_t tok = m_abi->subscribe(
            static_cast<PsdkEventKind>(kind),
            [](void* ud) {
                (*static_cast<std::function<void()>*>(ud))();
            },
            holder);
        if (!tok) {
            delete holder;
            return {};
        }
        m_holders[tok] = holder;
        return Token{ tok };
    }

    Token OnAreaChange  (std::function<void()> cb) { return Subscribe(EventKind::AreaChange,   std::move(cb)); }
    Token OnFrame       (std::function<void()> cb) { return Subscribe(EventKind::Frame,        std::move(cb)); }
    Token OnGameAttached(std::function<void()> cb) { return Subscribe(EventKind::GameAttached, std::move(cb)); }
    Token OnGameDetached(std::function<void()> cb) { return Subscribe(EventKind::GameDetached, std::move(cb)); }

    void Unsubscribe(Token tok) {
        if (!tok.Valid() || !m_abi || !m_abi->unsubscribe) return;
        auto it = m_holders.find(tok.Value);
        if (it == m_holders.end()) return;
        m_abi->unsubscribe(tok.Value);
        delete it->second;
        m_holders.erase(it);
    }
};

// ============================================================================
// OverlayService — per-plugin influence on overlay/data behavior
// ============================================================================
//
// Two independent, idempotent "request flags" that a plugin can toggle to
// change overlay-wide behavior for its own use case. Both follow the same
// shape:
//
//     ctx()->Overlay.SetX(true);   // I want X
//     ...
//     ctx()->Overlay.SetX(false);  // I no longer want X
//
// The host stores per-plugin flags (keyed by your plugin `this`) and computes
// the effective behavior as the OR of:
//   - all enabled plugins' flags,
//   - the host's own built-in needs (main menu visible, Radar's built-in
//     "Add Entity from Map" picker, AutoCraft lock, etc.).
//
// Multiple plugins can request the same thing simultaneously without
// interfering. The host clears all of a plugin's flags when it disables or
// unloads the plugin — so a crash or a forgotten cleanup will NOT permanently
// stick the overlay/data in the requested state. Well-behaved plugins still
// pair their on/off calls so neighboring plugins and the player remain
// responsive in the meantime.
//
// Latency: a Set call updates the per-plugin flag synchronously, but the
// effective behavior change manifests on the next host frame (overlay
// input) or the next GameClient worker tick (sleeping entities). For
// typical plugin workflows the lag is sub-frame and unobservable.
//
// All methods are safe to call from any thread.
class OverlayService {
    const OverlayServiceAbi* m_abi = nullptr;
    const HostAbi*           m_host = nullptr;
    void*                    m_plugin_token = nullptr;  // = Plugin*, opaque to host
public:
    void Init(const OverlayServiceAbi* abi, const HostAbi* host, void* token) {
        m_abi          = abi;
        m_host         = host;
        m_plugin_token = token;
    }

    // ── Gap 1: SetIncludeSleepingEntities ───────────────────────────────────
    //
    // Background
    //   PoE2 keeps a large pool of far-away or dormant entities live in memory
    //   but flagged "Useless" (EntityState::Useless). To keep snapshot cost
    //   bounded, the host hides Useless entities from EntitiesService.Enumerate
    //   by default — plugins only see active, nearby entities.
    //
    // Effect when enabled
    //   Useless entities also appear in EntitiesService.Enumerate and
    //   Snapshot.Entities. The Entity::IsSleeping flag marks specifically
    //   those entities that came from the host's separate SleepingEntities
    //   collection (orthogonal to EntityState::Useless — many entities are
    //   Useless without being in that collection).
    //
    // When to use
    //   - Map-picker UIs that need the full area entity pool (so you can let
    //     the user click on an entity that's not yet active).
    //   - Debug viewers showing every entity, sleeping or not.
    //
    // Cost
    //   Roughly +5–15% snapshot CPU per frame in dense areas while enabled.
    //   Negligible while disabled (default). Leave OFF unless you need it.
    //
    // Lifecycle
    //   Idempotent. Auto-cleared on plugin disable/unload. Pair on/off calls.
    void SetIncludeSleepingEntities(bool enable) const {
        if (m_abi && m_abi->set_include_sleeping_entities) {
            m_abi->set_include_sleeping_entities(m_plugin_token, enable ? 1 : 0);
        }
    }

    // ── Gap 2: SetWantsOverlayInput ─────────────────────────────────────────
    //
    // Request that the overlay window starts CONSUMING mouse clicks (so your
    // ImGui windows / popups receive them) instead of letting them pass
    // through to the game underneath.
    //
    // ## Background — what "input capture" actually means here
    //
    // The overlay window is a transparent always-on-top child sitting on top
    // of the PoE2 client window. In its default state it has the Win32 style
    // WS_EX_TRANSPARENT: every mouse click passes straight through to the
    // game underneath. This is the correct default for read-only overlays
    // (radar, health-bar tracker, DPS readout) — the player keeps playing
    // normally and never notices the overlay is there.
    //
    // The moment a plugin wants the user to CLICK ON SOMETHING INSIDE THE
    // OVERLAY — confirm a popup button, pick an entity on the map, drag a
    // marker, dismiss a modal — that default breaks: the click would land in
    // the game and your popup would never see it. SetWantsOverlayInput(true)
    // fixes this for the duration of your interactive mode.
    //
    // ## Effect when enabled
    //
    // The host's overlay input loop runs every frame. While ANY plugin (or
    // the host itself) has this flag on:
    //
    //   - If the OS cursor is over a visible ImGui window owned by you or
    //     any other plugin/host UI, the overlay temporarily clears
    //     WS_EX_TRANSPARENT and the click is delivered to ImGui (and NOT to
    //     the game). Your ImGui::Begin(...) window receives clicks normally.
    //
    //   - If the cursor is NOT over any visible ImGui window — the player is
    //     clicking on the world — WS_EX_TRANSPARENT is restored and the click
    //     reaches the game. Movement, attacks, looting all keep working.
    //
    // This per-frame "claim only what the user is actually over" behavior is
    // what makes a heavy interactive overlay coexist with active gameplay.
    //
    // ## Scope — what this flag does and does NOT affect
    //
    //   - Mouse buttons (LMB/RMB): YES, gated by this flag.
    //   - Mouse position / hover: ALWAYS works, regardless of this flag.
    //     Tooltips and hover effects don't need this.
    //   - Keyboard: ALWAYS reaches the plugin via the host's WindowProc, not
    //     gated by this flag. ImGui::IsKeyPressed(ImGuiKey_Escape) etc. work
    //     either way.
    //   - Game input: only loses clicks where your ImGui window covers them.
    //     The player can keep playing around your popup.
    //
    // ## Background-draw-list caveat — READ THIS for map-picker plugins
    //
    // If you draw clickable markers via ImGui::GetBackgroundDrawList() (typical
    // for radar/large-map overlays), the background list has no ImGui window
    // backing it. The host's "is the cursor over UI?" check walks the ImGui
    // window list and finds nothing → click-through is re-enabled and your
    // background-list click goes to the game.
    //
    // To make background-list clicks work, you need a real ImGui window the
    // host's hit-test recognizes (HitTestInternal in radar/render/render.cpp
    // iterates ctx->Windows; ImGui widgets like InvisibleButton are inside a
    // window, not windows themselves). Choose one:
    //
    //   1. (RECOMMENDED) Open a full-screen ImGui window with no decoration
    //      and put an InvisibleButton inside it. The window is what the host
    //      hit-tests; the InvisibleButton gives you ImGui::IsItemClicked()
    //      for click detection. Sketch:
    //
    //          ImGui::SetNextWindowPos({0, 0});
    //          ImGui::SetNextWindowSize(screenSize);
    //          ImGui::Begin("##picker", nullptr,
    //              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
    //              ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoResize    |
    //              ImGuiWindowFlags_NoScrollbar);
    //          ImGui::InvisibleButton("##picker_hit", screenSize);
    //          bool clicked = ImGui::IsItemClicked();
    //          // ... draw markers via ImGui::GetForegroundDrawList() or
    //          //     GetWindowDrawList(); do your world-space hit-test against
    //          //     markers using ImGui::GetMousePos() vs marker screen pos ...
    //          ImGui::End();
    //
    //   2. Same idea with a smaller window covering only the picker region
    //      instead of full-screen. Pick whichever fits — the host doesn't
    //      care about size, only that some window is under the cursor.
    //
    // ## When to use
    //
    //   - Plugin opens an ImGui popup/modal the user must click.
    //   - Map-picker flows: click on an entity / POI marker.
    //   - Drag-to-position UI for radar icons, custom overlays.
    //
    // ## When NOT to use
    //
    //   - Read-only overlays — leave OFF so the player isn't interrupted.
    //   - Hover-only tooltips — ImGui hover always works.
    //
    // ## Aggregation across plugins and host
    //
    // OR'd with the host's built-in needs (main menu visible, AutoCraft locked,
    // host's own map picker active) and every other plugin's flag. Multiple
    // simultaneous requests coexist fine. The effective state is recomputed
    // every frame from the aggregate.
    //
    // ## Lifecycle
    //
    // Remember to call SetWantsOverlayInput(false) when your interactive mode
    // ends (user picked / pressed Escape / closed the popup). The host clears
    // all of a plugin's flags automatically on OnDisable / unload, so a crash
    // or a forgotten cleanup will NOT permanently stick the overlay in capture
    // mode — but well-behaved plugins still pair their on/off calls so other
    // plugins and the game stay responsive in between.
    //
    // Idempotent. Safe from any thread.
    void SetWantsOverlayInput(bool enable) const {
        if (m_abi && m_abi->set_wants_overlay_input) {
            m_abi->set_wants_overlay_input(m_plugin_token, enable ? 1 : 0);
        }
    }
};

class FlasksService {
    const FlasksServiceAbi* m_abi = nullptr;
    const HostAbi*          m_host = nullptr;
public:
    void Init(const FlasksServiceAbi* abi, const HostAbi* host) {
        m_abi  = abi;
        m_host = host;
    }

    // GetFlask/GetCharm: returns nullopt on out-of-range or not-in-game.
    // Empty slot returns Flask{ Valid=false, ... } (still a Flask wrapper).
    std::optional<Flask> GetFlask(int32_t slot) const {
        if (!m_abi || !m_abi->get_flask) return std::nullopt;
        FlaskAbi a{};
        if (!m_abi->get_flask(slot, &a)) return std::nullopt;
        return Flask::FromAbi(a, m_host);
    }

    std::optional<Charm> GetCharm(int32_t slot) const {
        if (!m_abi || !m_abi->get_charm) return std::nullopt;
        CharmAbi a{};
        if (!m_abi->get_charm(slot, &a)) return std::nullopt;
        return Charm::FromAbi(a, m_host);
    }

    std::vector<Flask> AllFlasks() const {
        std::vector<Flask> out;
        if (!m_abi || !m_abi->enumerate_flasks) return out;
        struct Ctx { std::vector<Flask>* out; const HostAbi* host; };
        Ctx c{ &out, m_host };
        m_abi->enumerate_flasks(
            [](const FlaskAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                p->out->push_back(Flask::FromAbi(*a, p->host));
                return 1;
            },
            &c);
        return out;
    }

    std::vector<Charm> AllCharms() const {
        std::vector<Charm> out;
        if (!m_abi || !m_abi->enumerate_charms) return out;
        struct Ctx { std::vector<Charm>* out; const HostAbi* host; };
        Ctx c{ &out, m_host };
        m_abi->enumerate_charms(
            [](const CharmAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                p->out->push_back(Charm::FromAbi(*a, p->host));
                return 1;
            },
            &c);
        return out;
    }

    int32_t FlaskSlotCount() const {
        return (m_abi && m_abi->flask_slot_count) ? m_abi->flask_slot_count() : 0;
    }
    int32_t CharmSlotCount() const {
        return (m_abi && m_abi->charm_slot_count) ? m_abi->charm_slot_count() : 0;
    }
};

struct Context {
    GameService       Game;
    EntitiesService   Entities;
    ComponentsService Components;
    InventoryService  Inventory;
    UiService         Ui;
    RenderService     Render;
    TerrainService    Terrain;
    MemoryService     Memory;
    LogService        Log;
    EventsService     Events;
    OverlayService    Overlay;
    FlasksService     Flasks;
    void* ImGuiContext = nullptr;
    void* D3DDevice    = nullptr;
};

}  // namespace PluginSDK

// Forward-declared in the global namespace so PluginSDK::Plugin's friend
// declaration can name it. Inline definition lives at the bottom of this file.
extern "C" PLUGIN_API void PluginSDK_AttachHost(
    PluginSDK::Plugin* p, const HostAbi* abi, const char* directory);

namespace PluginSDK {

class Plugin {
public:
    virtual ~Plugin() = default;

    /// Display name shown in the host's Plugins settings tab. Must be ASCII.
    virtual const char* GetName() const = 0;

    /// Called once after construction + host attach. Load settings here.
    /// @param isGameAttached true if game process is already attached.
    virtual void OnEnable(bool /*isGameAttached*/) {}

    /// Called when the user disables the plugin. Free resources here.
    virtual void OnDisable() {}

    /// Called every frame on the Plugin settings tab to render ImGui config UI.
    virtual void DrawSettings() {}

    /// Called every frame on the main render loop when the plugin is enabled.
    virtual void DrawUI() {}

    /// Called periodically (~5s) and on disable. Persist settings to disk here.
    virtual void SaveSettings() {}

    /// Return true if this plugin wants the host to be in overlay mode.
    virtual bool WantsOverlay() const { return false; }

    /// SDK version this plugin was built against. DO NOT override.
    int GetSDKVersion() const { return PLUGIN_SDK_VERSION; }

protected:
    /// Access to host services. Valid after PluginSDK_AttachHost, before OnEnable.
    const Context* ctx() const { return &m_ctx; }

    /// Absolute path to this plugin's directory (Plugins/<Name>/). UTF-8.
    const char* Directory() const { return m_directory.c_str(); }

    /// Same as Directory() but returned as std::filesystem::path with the
    /// proper UTF-8 → wide conversion baked in. Use this whenever you need
    /// to construct fs::path / std::ifstream / CreateDirectoryW etc.
    ///
    /// Why: fs::path(std::string) on Windows interprets the string via the
    /// system ANSI codepage, NOT UTF-8. If the host's exe lives in a folder
    /// with non-ASCII chars (Cyrillic, CJK, …) and the system ACP isn't
    /// UTF-8, the bytes get misinterpreted → mojibake wide path → file
    /// operations land on the wrong location (often creating a sibling
    /// folder with garbled name next to the real one). Going through
    /// CP_UTF8 → wide → fs::path keeps the original chars intact.
    std::filesystem::path DirectoryPath() const {
        if (m_directory.empty()) return {};
        int needed = ::MultiByteToWideChar(
            CP_UTF8, 0, m_directory.c_str(),
            static_cast<int>(m_directory.size()), nullptr, 0);
        if (needed <= 0) return {};
        std::wstring wide(static_cast<size_t>(needed), L'\0');
        ::MultiByteToWideChar(
            CP_UTF8, 0, m_directory.c_str(),
            static_cast<int>(m_directory.size()),
            wide.data(), needed);
        return std::filesystem::path(wide);
    }

    /// True if the host ABI version and size matched at attach time. When
    /// false, `m_ctx` is unpopulated and the plugin should refuse to function.
    bool HostCompatible() const { return m_host_compatible; }

private:
    friend void ::PluginSDK_AttachHost(Plugin*, const HostAbi*, const char*);

    Context     m_ctx{};
    // Owned-by-value so Directory() never dangles even if the host's
    // PluginContainer vector reallocates after attach.
    std::string m_directory;
    bool        m_host_compatible = false;
};

}  // namespace PluginSDK

// Lives inside the plugin DLL; the host loader resolves it via GetProcAddress
// and calls it between CreatePlugin() and OnEnable(). Rejects mismatched
// version, leaving HostCompatible() false.
//
// Size compatibility is one-way: the host may grow `sizeof(HostAbi)` by
// appending fields (no version bump) as long as existing fields keep their
// offsets. A plugin compiled against an older (smaller) `sizeof(HostAbi)`
// therefore accepts a larger host struct because the host's layout is a
// superset of what the plugin reads. We only reject hosts whose struct is
// *smaller* than what this plugin was built against — that would mean the
// host is missing fields the plugin may dereference.
inline void PluginSDK_AttachHost(PluginSDK::Plugin* p,
                                 const HostAbi*  abi,
                                 const char*        directory) {
    if (!p) return;
    p->m_directory = directory ? directory : "";
    p->m_host_compatible =
        (abi != nullptr
         && abi->version == PLUGIN_SDK_VERSION
         && abi->size_bytes >= sizeof(HostAbi));  // append-only growth allowed
    if (!p->m_host_compatible) return;

    p->m_ctx.Game      .Init(&abi->game,       abi);
    p->m_ctx.Entities  .Init(&abi->entities,   abi);
    p->m_ctx.Components.Init(&abi->components, abi);
    p->m_ctx.Inventory .Init(&abi->inventory,  abi);
    p->m_ctx.Ui        .Init(&abi->ui,         abi);
    p->m_ctx.Render    .Init(&abi->render,     abi);
    p->m_ctx.Terrain   .Init(&abi->terrain,    abi);
    p->m_ctx.Memory    .Init(&abi->memory,     abi);
    p->m_ctx.Log       .Init(&abi->log,        abi);
    p->m_ctx.Events    .Init(&abi->events,     abi);
    p->m_ctx.Overlay   .Init(&abi->overlay,    abi, p);   // p = plugin_token
    p->m_ctx.Flasks    .Init(&abi->flasks,     abi);
    p->m_ctx.ImGuiContext = abi->imgui_context;
    p->m_ctx.D3DDevice    = abi->d3d_device;
}
