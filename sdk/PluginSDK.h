// PluginSDK.h — C++ wrapper for writing POEFixer plugins (SDK v6).
//
// Writing a plugin:
//   1. Subclass PluginSDK::Plugin, implement GetName() plus the hooks you need
//      (OnEnable / DrawUI / DrawSettings / SaveSettings).
//   2. Export a factory + deleter from your DLL:
//        extern "C" PLUGIN_API PluginSDK::Plugin* CreatePlugin()            { return new MyPlugin(); }
//        extern "C" PLUGIN_API void               DestroyPlugin(Plugin* p)  { delete p; }
//      (PluginSDK_AttachHost is emitted for you when PLUGIN_EXPORTS is defined.)
//   3. Build as an x64 DLL with PLUGIN_EXPORTS defined; drop it in Plugins/<Name>/.
//
// Inside the plugin, reach the host through ctx(): ctx()->Game.GetSnapshot(),
// ctx()->Entities.Enumerate(...), etc. Returned values are per-call copies and
// their std::string fields are already materialized (safe to keep). To draw,
// set ImGui's context from ctx()->ImGuiContext each frame.
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

// dllexport while building the plugin (define PLUGIN_EXPORTS), dllimport elsewhere.
#ifdef PLUGIN_EXPORTS
    #define PLUGIN_API __declspec(dllexport)
#else
    #define PLUGIN_API __declspec(dllimport)
#endif

namespace PluginSDK {

// Strongly-typed mirrors of the Psdk* ABI enums in PluginAbi.h.
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
    Unknown           = PSDK_GAME_STATE_UNKNOWN,
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

// Host services — instances live on Context; reach them via ctx() in a Plugin.
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
class PricesService;
class RuneshapeService;
class AtlasService;
class SekhemaService;
class Plugin;
struct Context;

// Copy a host-owned string/wstring (by address) into an owned value. The
// wrapper structs below call these for you, so their std::string fields are
// already resolved and safe to keep past the call.
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

// Like FetchWString but narrows to std::string. Intended for ASCII-only internal
// identifiers (dat Ids/paths such as "ShockedGround" or "ground_fire_burn_white");
// any non-ASCII code unit becomes '?'. Use FetchWString when full Unicode matters.
inline std::string FetchWStringNarrow(uintptr_t addr, const HostAbi* abi) {
    std::wstring w = FetchWString(addr, abi);
    std::string s;
    s.reserve(w.size());
    for (wchar_t c : w) s.push_back(c < 0x80 ? static_cast<char>(c) : '?');
    return s;
}

// ---------------------------------------------------------------------------
// Owned views of the ABI PODs. Each ::FromAbi() copies out of the host struct
// and resolves string addresses, so instances are self-contained and safe to
// store. Field meanings match the matching *Abi struct in PluginAbi.h.
// ---------------------------------------------------------------------------

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

// Animation state only. For a unit's current action (casting/target cell) use
// ComponentsService::ReadActorAction instead.
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

// A unit's current action: Flags bitmask (IsUsingAbility() tests bit 0x400)
// plus the target grid cell. From ComponentsService::ReadActorAction.
struct ActorAction {
    int  Flags = 0;
    int  DestX = 0;
    int  DestY = 0;
    bool Valid = false;

    bool IsUsingAbility() const { return (Flags & 0x400) != 0; }

    static ActorAction FromAbi(const ActorActionAbi& a) {
        ActorAction r;
        r.Flags = a.flags;
        r.DestX = a.dest_x;
        r.DestY = a.dest_y;
        r.Valid = a.valid != 0;
        return r;
    }
};

// One waypoint (grid cell) of a movement route from ComponentsService::ReadPathfinding.
struct PathNode { int X = 0; int Y = 0; };

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

    int  MaxUses = 0;
    int  TotalActiveCooldowns = 0;
    uint32_t  EquipmentInfoPacked = 0;
    uintptr_t GrantedEffectsPerLevelAddr = 0;
    uintptr_t ActiveSkillsDatAddr = 0;
    uintptr_t GrantedEffectStatSetsPerLevelAddr = 0;
    uintptr_t SkillDetailsAddr = 0;

    struct EquipmentInfo {
        uint32_t  GemNameHash = 0;
        int       InventorySlot = 0;
        int       LinkIndex = 0;
        int       SocketIndex = 0;
        int       UnknownFlag = 0;
        bool      CanBeOnPlayerItem = false;
    } Equipment;

    static ActiveSkill FromAbi(const ActiveSkillAbi& a, const HostAbi* abi) {
        ActiveSkill s;
        s.CurrentSize     = a.current_size;
        s.TotalUses       = a.total_uses;
        s.UseStage        = a.use_stage;
        s.CastType        = a.cast_type;
        s.TotalCooldownMs = a.total_cooldown_ms;
        s.CanBeUsed       = a.can_be_used != 0;
        s.Name            = FetchString(a.name_addr, abi);

        s.MaxUses                          = a.max_uses;
        s.TotalActiveCooldowns             = a.total_active_cooldowns;
        s.EquipmentInfoPacked              = a.equipment_info_packed;
        s.GrantedEffectsPerLevelAddr       = a.granted_effects_per_level_addr;
        s.ActiveSkillsDatAddr              = a.active_skills_dat_addr;
        s.GrantedEffectStatSetsPerLevelAddr = a.granted_effect_stat_sets_per_level_addr;
        s.SkillDetailsAddr                 = a.skill_details_addr;

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
    std::string Id;            // Mods.dat Id (disambiguates shared stat keys); "" if host predates this field
    uint32_t    Hash32 = 0;    // Mods.dat HASH32 catalog key
    int   GenerationType = 0;
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
        m.Id             = FetchString(a.id_addr, abi);
        m.Hash32         = a.hash32;
        return m;
    }
};

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

// One monster modifier from an ObjectMagicProperties component
// (ComponentsService::EnumerateMonsterMods). Match on Id (most stable),
// Metadata, or a hash. Metadata is empty for non-monster mods.
struct MonsterMod {
    std::string Id;        // Mods.dat Id, e.g. "MonsterAbyssLightlessFaction1"
    std::string Name;      // Mods.dat Name (display), e.g. "Abyssal"
    std::string Metadata;  // Mods.dat MonsterMetadata, e.g. "Metadata/.../LightlessWells"
    uint16_t    Hash16 = 0;          // e.g. 0x63D1
    uint32_t    Hash32 = 0;          // e.g. 0xBFDA2A36
    int         GenerationType = 0;  // 1=Prefix 2=Suffix 3=Implicit

    static MonsterMod FromAbi(const MonsterModAbi& a, const HostAbi* abi) {
        MonsterMod m;
        m.Id             = FetchString(a.id_addr, abi);
        m.Name           = FetchString(a.display_name_addr, abi);
        m.Metadata       = FetchString(a.metadata_addr, abi);
        m.Hash16         = a.hash16;
        m.Hash32         = a.hash32;
        m.GenerationType = a.generation_type;
        return m;
    }
};

// A ground effect read from a VisibleServerGroundEffect entity's "GroundEffect"
// component (ComponentsService::ReadGroundEffect, by ENTITY address). Lets a
// plugin tell apart the many same-pathed ground effects and draw an accurate
// world-space area. Match on TypeId — the stable, patch-independent key. The
// entity's world position comes from the entity itself (Entity / Positioned /
// Render), so it is not duplicated here.
struct GroundEffect {
    bool        Valid = false;
    std::string TypeId;        // groundeffecttypes.Id — the stable key, e.g. "ShockedGround"
    float       Radius = 0.f;  // world units (e.g. 190.0); 0 = unset by this variant
    std::string EndEffect;     // end behaviour: "fadeout" / "close" / "end"
    std::string BuffVisual1;   // buffvisuals.Id, e.g. "ground_fire_burn_white" (empty if unset)
    std::string BuffVisual2;   // buffdefinitions.Name, e.g. "ground_tar_gold" (empty if unset)
    std::string AoFile;        // first .ao/.aoc visual path (empty if none)
    uintptr_t   GroundEffectsRowAddr = 0;     // raw groundeffects.datc64 row ptr (session-stable)
    uintptr_t   GroundEffectTypesRowAddr = 0; // raw groundeffecttypes.datc64 row ptr

    static GroundEffect FromAbi(const GroundEffectAbi& a, const HostAbi* abi) {
        GroundEffect g;
        g.Valid                    = a.valid != 0;
        g.Radius                   = a.radius;
        g.TypeId                   = FetchWStringNarrow(a.type_id_addr, abi);
        g.EndEffect                = FetchWStringNarrow(a.end_effect_addr, abi);
        g.BuffVisual1              = FetchWStringNarrow(a.buff_visual1_addr, abi);
        g.BuffVisual2              = FetchWStringNarrow(a.buff_visual2_addr, abi);
        g.AoFile                   = FetchWStringNarrow(a.ao_file_addr, abi);
        g.GroundEffectsRowAddr     = a.ground_effects_row;
        g.GroundEffectTypesRowAddr = a.ground_effect_types_row;
        return g;
    }
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
    float ScaleX = 0, ScaleY = 0;
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

    float     ScreenX = 0.0f;
    float     ScreenY = 0.0f;
    float     ScreenW = 0.0f;
    float     ScreenH = 0.0f;
    bool      ScreenValid = false;

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
        i.ScreenX          = a.screen_x;
        i.ScreenY          = a.screen_y;
        i.ScreenW          = a.screen_w;
        i.ScreenH          = a.screen_h;
        i.ScreenValid      = a.screen_valid != 0;
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

// Per-entity component addresses (0 = absent). Pass a non-zero member to the
// matching ComponentsService::Read* call; HasX() are convenience checks.
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

// One entity. Address identifies it (pass to ReadPathfinding etc.); Components
// holds its component addresses; grid vs world coords are both provided.
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

// One frame of world state from GameService::GetSnapshot(). Entities/inventories
// /buffs are not inline — enumerate them through their services.
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

// World/engine state and the per-frame snapshot. Reached as ctx()->Game.
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

    // Cheap area-change counter WITHOUT the per-entity enumeration that
    // GetSnapshot() does in Snapshot::FromAbi. The host get_snapshot ABI fills
    // only scalars (+ player/maps/vitals), so a plugin can detect area
    // transitions every frame without paying the full-snapshot cost. Bumps on
    // every area load (use it to re-arm per-area work).
    uint64_t GetAreaChangeCounter() const {
        SnapshotAbi raw{};
        if (m_abi && m_abi->get_snapshot) m_abi->get_snapshot(&raw);
        return raw.area_change_counter;
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

    // Character gold counter (the inventory gold amount). 0 when not in game.
    // Sourced from the host's ServerData snapshot via the HostAbi tail.
    int GetGold() const {
        return (m_host && m_host->get_gold) ? m_host->get_gold() : 0;
    }

    // Host Radar -> RuneShape "Show weights on map" toggle. Defaults to TRUE on
    // hosts that predate the tail function (keeps legacy chip behaviour).
    bool RuneshapeWeightsShown() const {
        return !m_host || !m_host->get_runeshape_weights_shown ||
               m_host->get_runeshape_weights_shown() != 0;
    }

    // Raw WorldArea id of the current zone (e.g. "Sanctum_1", "Sanctum_2_Foyer_1").
    // Carries the Trial-of-the-Sekhemas floor number even when room content is
    // hidden on the map. Empty on hosts that predate the tail function, off-trial,
    // or when not in game.
    std::string GetAreaId() const {
        if (!m_host || !m_host->get_area_id) return std::string();
        char buf[128];
        int n = m_host->get_area_id(buf, static_cast<int>(sizeof(buf)));
        return (n > 0) ? std::string(buf, static_cast<size_t>(n)) : std::string();
    }

    // Genesis-tree (Hiveblood) resource counter. Returns true and writes `out`
    // when the host's pattern-anchored chain resolves; false when not in game,
    // the chain is broken, or the host predates the tail function (`out`
    // untouched on false). Routed via the HostAbi TOP-LEVEL pointer
    // (m_host->get_hiveblood), NOT a GameServiceAbi member (m_abi->) — the
    // append-only tail keeps GameServiceAbi's layout frozen for already-compiled
    // plugins (same asymmetry as GetGold / GetAreaId).
    bool GetHiveblood(int32_t& out) const {
        return m_host && m_host->get_hiveblood && m_host->get_hiveblood(&out) != 0;
    }
};

// Enumerate / look up entities; Watch() keeps a component map fresh across
// frames. Reached as ctx()->Entities.
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

// Read a single component from its address, or walk the list-valued ones
// (buffs / active skills / stats / mods). Reached as ctx()->Components.
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

    // Movement route as grid waypoints. Pass the ENTITY address (Entity::Address),
    // not a component address; empty if the unit isn't moving / not in the snapshot.
    std::vector<PathNode> ReadPathfinding(uintptr_t entityAddr) const {
        std::vector<PathNode> out;
        if (!m_host || !m_host->read_pathfinding || entityAddr == 0) return out;
        PathfindingAbi a{};
        if (!m_host->read_pathfinding(entityAddr, &a) || a.valid == 0) return out;
        int count = a.node_count;
        if (count < 0) count = 0;
        if (count > 64) count = 64;
        out.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
            out.push_back(PathNode{ a.nodes[i].x, a.nodes[i].y });
        return out;
    }

    // Current action (flags + target cell). Pass the Actor component address
    // (Entity::Components.Actor).
    ActorAction ReadActorAction(uintptr_t actorAddr) const {
        if (m_host && m_host->read_actor_action && actorAddr != 0) {
            ActorActionAbi a{};
            if (m_host->read_actor_action(actorAddr, &a))
                return ActorAction::FromAbi(a);
        }
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

    // Like EnumerateBuffs, but duplicate-named status effects are aggregated
    // into ONE Buff whose Charges is the summed stack count, matching the
    // in-game charge-stack icon. Example: Chayula breach charges are stored as
    // N separate StatusEffect instances of Charges=1 — EnumerateBuffs returns N
    // rows of "1", while this returns one row of "N". Single-instance buffs
    // (power/frenzy charges) are identical to EnumerateBuffs. Returns empty if
    // the host predates this API (it lives on the HostAbi tail; null-checked).
    std::vector<Buff> EnumerateAggregatedBuffs(uintptr_t buffsAddr) const {
        std::vector<Buff> out;
        if (!m_host || !m_host->enumerate_buffs_aggregated) return out;
        struct Ctx { std::vector<Buff>* out; const HostAbi* host; };
        Ctx c{ &out, m_host };
        m_host->enumerate_buffs_aggregated(buffsAddr,
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

    // Enumerate a monster's rolled mods from its ObjectMagicProperties
    // component address (Entity::Components.OMP). Lets a plugin identify a
    // monster's modifiers the instant it spawns — before any related buff is
    // applied. Empty if the unit has no OMP component / no mods. Match on
    // MonsterMod::Id (most stable), Metadata, or Hash16/Hash32.
    std::vector<MonsterMod> EnumerateMonsterMods(uintptr_t ompAddr) const {
        std::vector<MonsterMod> out;
        if (!m_host || !m_host->enumerate_monster_mods || ompAddr == 0) return out;
        struct Ctx { std::vector<MonsterMod>* out; const HostAbi* host; };
        Ctx c{ &out, m_host };
        m_host->enumerate_monster_mods(ompAddr,
            [](const MonsterModAbi* m, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                p->out->push_back(MonsterMod::FromAbi(*m, p->host));
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

    // Read a ground effect from a VisibleServerGroundEffect entity. Pass the
    // ENTITY address (Entity::Address), NOT a component address — the host
    // resolves the "GroundEffect" component itself (it is not in the per-entity
    // component-address table). Returns an invalid GroundEffect if the entity has
    // no GroundEffect component, or if the host predates this API (it lives on
    // the HostAbi tail and is null-checked). Distinguish effects via TypeId, e.g.
    // "ShockedGround" / "IgnitedGround" / "CausticCloud" / "ChilledGround".
    GroundEffect ReadGroundEffect(uintptr_t entityAddr) const {
        if (m_host && m_host->read_ground_effect && entityAddr != 0) {
            GroundEffectAbi a{};
            if (m_host->read_ground_effect(entityAddr, &a))
                return GroundEffect::FromAbi(a, m_host);
        }
        return {};
    }
};

// Inventories, their items, and item-detail lookups. Reached as ctx()->Inventory.
// Item base defensive values (base, not the computed tooltip total).
struct ItemBaseStats {
    bool Valid = false;
    int Armour = 0, Evasion = 0, EnergyShield = 0, Ward = 0;
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

    // In-game-style text for a stat key + value(s) via the host .csd formatter
    // (the same formatting the Debug panel uses). Empty if unavailable.
    std::string FormatStat(const std::string& statKey, float v0, float v1 = 0.0f) const {
        if (!m_host || !m_host->format_stat_description) return {};
        char buf[256];
        int32_t n = m_host->format_stat_description(statKey.c_str(), v0, v1, buf,
                                                    static_cast<int32_t>(sizeof(buf)));
        return (n > 0) ? std::string(buf, static_cast<size_t>(n)) : std::string();
    }

    // Base defensive values for an item (base, not the computed tooltip total).
    ItemBaseStats ReadItemBaseStats(uintptr_t entityAddr) const {
        ItemBaseStats r;
        if (!m_host || !m_host->read_item_base_stats) return r;
        int32_t v[4] = { 0, 0, 0, 0 };
        if (m_host->read_item_base_stats(entityAddr, v)) {
            r.Valid = true;
            r.Armour = v[0]; r.Evasion = v[1]; r.EnergyShield = v[2]; r.Ward = v[3];
        }
        return r;
    }

    // Aggregated stats (StatsFromMods) as {statId, value} pairs (waystone
    // Item Rarity/Pack Size/etc. + every aggregated stat).
    std::vector<std::pair<int, int>> ReadItemAggregatedStats(uintptr_t entityAddr) const {
        std::vector<std::pair<int, int>> out;
        if (!m_host || !m_host->read_item_aggregated_stats) return out;
        int32_t buf[128];
        int32_t count = m_host->read_item_aggregated_stats(entityAddr, buf, 64);
        int32_t n = (count < 64) ? count : 64;
        for (int32_t i = 0; i < n; ++i)
            out.emplace_back(buf[2 * i], buf[2 * i + 1]);
        return out;
    }
};

// Walk the game's UI tree (children, paths, visibility, screen rects).
// Reached as ctx()->Ui.
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
            [](uintptr_t child, int32_t , void* ud) -> int32_t {
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

    struct ScreenRect { float x = 0, y = 0, w = 0, h = 0; bool ok = false; };

    // Batch variant of ComputeScreenRect. Returns one ScreenRect per input addr
    // (same order). Empty / all-not-ok if the host is too old to provide it.
    std::vector<ScreenRect> ComputeScreenRects(const std::vector<uintptr_t>& addrs) const {
        std::vector<ScreenRect> out(addrs.size());
        // compute_screen_rects lives on the HostAbi tail (see PluginAbi.h), so it
        // is reached through m_host, not m_abi. Old hosts leave it null -> all not-ok.
        if (!m_host || !m_host->compute_screen_rects || addrs.empty()) return out;
        std::vector<PsdkScreenRectAbi> tmp(addrs.size());
        m_host->compute_screen_rects(addrs.data(), static_cast<int32_t>(addrs.size()), tmp.data());
        for (size_t i = 0; i < addrs.size(); ++i)
            out[i] = { tmp[i].x, tmp[i].y, tmp[i].w, tmp[i].h, tmp[i].ok != 0 };
        return out;
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

// Project world/grid coordinates to screen and to the large/mini map.
// Reached as ctx()->Render.
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

// RAII view over a walkable-grid snapshot; releases the host buffer on destruct.
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

    size_t SizeBytes() const {
        if (m_h.width <= 0 || m_h.height <= 0) return 0;
        return (static_cast<size_t>(m_h.width)
              * static_cast<size_t>(m_h.height)) / 2;
    }
};

// RAII view over a height-grid snapshot; releases the host buffer on destruct.
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

    size_t ElementCount() const {
        if (m_h.width <= 0 || m_h.height <= 0) return 0;
        return static_cast<size_t>(m_h.width)
             * static_cast<size_t>(m_h.height);
    }
    size_t SizeBytes() const { return ElementCount() * sizeof(float); }
};

// Walkability, terrain height, and loaded tile locations. Reached as ctx()->Terrain.
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

// Raw reads from the game process (bytes / strings / std::vector / patterns).
// Reached as ctx()->Memory.
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

// Write to the host log (Info / Warn / Error). Reached as ctx()->Log.
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

// Subscribe to host events (area change, frame, attach/detach); the returned
// Token unsubscribes on destruct. Reached as ctx()->Events.
class EventsService {
    const EventsServiceAbi* m_abi = nullptr;
    std::map<uint64_t, std::function<void()>*> m_holders;
public:
    struct Token {
        uint64_t Value = 0;
        bool Valid() const { return Value != 0; }
    };

    void Init(const EventsServiceAbi* abi, const HostAbi* ) {
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

// Per-plugin overlay requests (receive sleeping entities, capture overlay
// input). Auto-cleared on unload. Reached as ctx()->Overlay.
class OverlayService {
    const OverlayServiceAbi* m_abi = nullptr;
    const HostAbi*           m_host = nullptr;
    void*                    m_plugin_token = nullptr;
public:
    void Init(const OverlayServiceAbi* abi, const HostAbi* host, void* token) {
        m_abi          = abi;
        m_host         = host;
        m_plugin_token = token;
    }

    void SetIncludeSleepingEntities(bool enable) const {
        if (m_abi && m_abi->set_include_sleeping_entities) {
            m_abi->set_include_sleeping_entities(m_plugin_token, enable ? 1 : 0);
        }
    }

    void SetWantsOverlayInput(bool enable) const {
        if (m_abi && m_abi->set_wants_overlay_input) {
            m_abi->set_wants_overlay_input(m_plugin_token, enable ? 1 : 0);
        }
    }
};

// The utility belt: life/mana flasks and charms in slot order (empty =>
// Valid==false). Reached as ctx()->Flasks.
class FlasksService {
    const FlasksServiceAbi* m_abi = nullptr;
    const HostAbi*          m_host = nullptr;
public:
    void Init(const FlasksServiceAbi* abi, const HostAbi* host) {
        m_abi  = abi;
        m_host = host;
    }

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

// Item prices from the host PriceService. Reached as ctx()->Prices.
struct PriceResult { bool found=false; float chaos=0, divine=0, exalt=0; std::string category; std::string iconPath; };
struct PriceRates  { float divineInChaos=0, exaltedInChaos=0; };
struct PriceStatus { bool loaded=false; int totalItems=0;
                     float divineInChaos=0, exaltedInChaos=0;
                     int catsOk=0, catsPending=0, catsFailed=0; };

class PricesService {
    const PricesServiceAbi* m_abi = nullptr;
    const HostAbi*          m_host = nullptr;
public:
    void Init(const PricesServiceAbi* abi, const HostAbi* host) { m_abi = abi; m_host = host; }

    PriceResult LookupPrice(const std::string& name) const {
        PriceResult r;
        if (!m_abi || !m_abi->lookup_price) return r;
        PriceResultAbi a{};
        if (m_abi->lookup_price(name.c_str(), &a)) {
            r.found = a.found != 0; r.chaos = a.chaos; r.divine = a.divine; r.exalt = a.exalt;
            r.category = a.category;
            if (m_host && m_host->lookup_price_icon) {
                char ip[260] = {};
                if (m_host->lookup_price_icon(name.c_str(), ip, (int32_t)sizeof(ip)))
                    r.iconPath = ip;
            }
        }
        return r;
    }
    PriceRates GetRates() const {
        PriceRates r;
        if (m_abi && m_abi->get_rates) m_abi->get_rates(&r.divineInChaos, &r.exaltedInChaos);
        return r;
    }
    PriceStatus GetStatus() const {
        PriceStatus s;
        if (m_abi && m_abi->get_status) {
            PriceStatusAbi a{};
            m_abi->get_status(&a);
            s.loaded = a.loaded != 0; s.totalItems = a.total_items;
            s.divineInChaos = a.divine_in_chaos; s.exaltedInChaos = a.exalted_in_chaos;
            s.catsOk = a.cats_ok; s.catsPending = a.cats_pending; s.catsFailed = a.cats_failed;
        }
        return s;
    }
};

// One Runeshape device resolved by the host RuneshapeResolver.
struct Runeshape {
    uint64_t    entityId    = 0;
    uint32_t    color       = 0;
    bool        isUnique    = false;
    int         holeCount   = 0;
    std::string anchorName;
    int         rewardCount = 0;
    int         bestIndex   = -1;
    std::vector<int> propagatingSlots;  // slot(s) whose rune propagates (0.5.4)
    // Best recipe-combination weight at this station (user per-rune weights,
    // Radar -> RuneShape tab; 0 when the user has no weights configured).
    int         comboWeight = 0;
    // Anchor rune identity: Expedition2Runes row index (-1 = anchor-less
    // unique station) and the anchor's slot position (0-based).
    int         anchorRuneIdx = -1;
    int         anchorPos     = 0;
    // Collected or broken/unavailable — the host hides it on the radar;
    // overlays should gray it out.
    bool        completed     = false;
    // Per-slot rune indices of the best-priced recipe (empty = unknown; show
    // just the anchor rune in its slot). -1 entries = empty slots.
    std::vector<int> bestRunes;
};

// One reward slot for a Runeshape device.
struct RuneshapeReward {
    std::string name;
    int         count      = 0;
    float       unitChaos  = 0.f;
    float       totalChaos = 0.f;
    bool        priced     = false;
    // Rune propagation (0.5.4): rune(s) at this recipe's propagating slot(s).
    std::string propagatingRunes;            // e.g. "Power" / "Cold, Time"; "" if none
    int         propagatingCount   = 0;
    bool        propagatingHasRare = false;  // any propagating rune is rare ("purple")
    // Combination weight of THIS recipe: sum of the user's per-rune weights
    // over its slots (Radar -> RuneShape tab).
    int         comboWeight        = 0;
    // Number of rune slots in this recipe (recipe size).
    int         recipeSize         = 0;
};

// Runeshape devices and their per-entity reward lists.
// Reached as ctx()->Runeshape.
class RuneshapeService {
    const RuneshapeServiceAbi* m_abi  = nullptr;
    const HostAbi*             m_host = nullptr;
public:
    void Init(const RuneshapeServiceAbi* abi, const HostAbi* host) {
        m_abi  = abi;
        m_host = host;
    }

    // Collect all resolved Runeshape devices.
    std::vector<Runeshape> Runeshapes() const {
        std::vector<Runeshape> out;
        if (!m_abi || !m_abi->enumerate_runeshapes) return out;
        struct Ctx { std::vector<Runeshape>* out; };
        Ctx c{ &out };
        m_abi->enumerate_runeshapes(
            [](const RuneshapeAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                Runeshape r;
                r.entityId   = a->entity_id;
                r.color      = a->color;
                r.isUnique   = a->is_unique != 0;
                r.holeCount  = a->hole_count;
                r.anchorName = a->anchor_name;  // NUL-terminated char buffer
                r.rewardCount = a->reward_count;
                r.bestIndex  = a->best_index;
                for (int i = 0; i < a->propagating_slot_count && i < 4; ++i)
                    r.propagatingSlots.push_back(a->propagating_slots[i]);
                r.comboWeight   = a->combo_weight;
                r.anchorRuneIdx = a->anchor_rune_idx;
                r.anchorPos     = a->anchor_pos;
                r.completed     = a->completed != 0;
                for (int i = 0; i < a->best_rune_count && i < 12; ++i)
                    r.bestRunes.push_back(a->best_runes[i]);
                p->out->push_back(std::move(r));
                return 1;
            },
            &c);
        return out;
    }

    // Collect rewards for a specific Runeshape entity.
    std::vector<RuneshapeReward> Rewards(uint64_t entityId) const {
        std::vector<RuneshapeReward> out;
        if (!m_abi || !m_abi->enumerate_rewards) return out;
        struct Ctx { std::vector<RuneshapeReward>* out; };
        Ctx c{ &out };
        m_abi->enumerate_rewards(entityId,
            [](const RuneshapeRewardAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                RuneshapeReward r;
                r.name       = a->name;  // NUL-terminated char buffer
                r.count      = a->count;
                r.unitChaos  = a->unit_chaos;
                r.totalChaos = a->total_chaos;
                r.priced     = a->priced != 0;
                r.propagatingRunes   = a->propagating_runes;  // NUL-terminated
                r.propagatingCount   = a->propagating_count;
                r.propagatingHasRare = a->propagating_has_rare != 0;
                r.comboWeight        = a->combo_weight;
                r.recipeSize         = a->recipe_size;
                p->out->push_back(std::move(r));
                return 1;
            },
            &c);
        return out;
    }
};

// ---------------------------------------------------------------------------
// Atlas service — live endgame-atlas panel data (nodes / per-anchor adjacency
// / selection vectors / Rite line seed / eligibility weights), read host-side
// through the GameLibrary atlas offsets. See PsdkAtlasNodeDetail for the
// per-node cost model: request only the detail bits a hot path needs.
// ---------------------------------------------------------------------------

// One atlas node (owned view of AtlasNodeAbi). Grid coords are stable per map;
// the node ADDRESSES churn every frame as the atlas UI rebuilds.
struct AtlasNode {
    int         gridX = 0;
    int         gridY = 0;
    uintptr_t   uiAddress    = 0;   // node UI widget (Ui.ComputeScreenRect target)
    uintptr_t   extraAddress = 0;   // per-node data record
    uint32_t    marker   = 0;       // Record detail: Rite line-map type (0x47B = non-reward)
    uint32_t    flags    = 0;       // Record detail: bit0 AccessibleNow, bit1 Completed
    int         biome    = 0;       // UiState detail: EndGameMapBiomes _rid
    int         mapState = 0;       // UiState detail: low 2 bits; bit1 = completed
    std::string name;               // Name detail: display name ("" unresolved)
};

// One per-anchor adjacency entry (the Rite "conn table"). Neighbour slot
// order is preserved — the reward fn's neighbour rank derives from it.
struct AtlasConnection {
    int x = 0, y = 0;                             // anchor map grid
    std::vector<std::pair<int, int>> neighbors;   // linked grids (up to 5)
};

struct AtlasGridPoint { int x = 0, y = 0; };
// One RAW weight row; keys repeat across rows — dedup-sum client-side.
struct AtlasWeight    { int key = 0; int value = 0; };

class AtlasService {
    const AtlasServiceAbi* m_abi  = nullptr;
    const HostAbi*         m_host = nullptr;
public:
    void Init(const AtlasServiceAbi* abi, const HostAbi* host) {
        m_abi  = abi;
        m_host = host;
    }

    // Atlas panel address; 0 = panel absent (not in game / UI not built).
    uintptr_t GetPanel() const {
        return (m_abi && m_abi->get_panel) ? m_abi->get_panel() : 0;
    }

    // All atlas nodes. detail = PSDK_ATLAS_NODE_* bits; the default skips the
    // expensive names — resolve those lazily via GetNodeName().
    std::vector<AtlasNode> Nodes(
            int detail = PSDK_ATLAS_NODE_RECORD | PSDK_ATLAS_NODE_UISTATE) const {
        std::vector<AtlasNode> out;
        if (!m_abi || !m_abi->enumerate_nodes) return out;
        struct Ctx { std::vector<AtlasNode>* out; };
        Ctx c{ &out };
        m_abi->enumerate_nodes(static_cast<int32_t>(detail),
            [](const AtlasNodeAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                AtlasNode n;
                n.gridX        = a->grid_x;
                n.gridY        = a->grid_y;
                n.uiAddress    = a->ui_addr;
                n.extraAddress = a->extra_addr;
                n.marker       = a->marker;
                n.flags        = a->flags;
                n.biome        = a->biome;
                n.mapState     = a->map_state;
                n.name         = a->name;  // NUL-terminated char buffer
                p->out->push_back(std::move(n));
                return 1;
            },
            &c);
        return out;
    }

    // The per-anchor adjacency table (viewport-dependent: grows/shrinks with
    // the loaded atlas region).
    std::vector<AtlasConnection> Connections() const {
        std::vector<AtlasConnection> out;
        if (!m_abi || !m_abi->enumerate_connections) return out;
        struct Ctx { std::vector<AtlasConnection>* out; };
        Ctx c{ &out };
        m_abi->enumerate_connections(
            [](const AtlasConnAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                AtlasConnection e;
                e.x = a->key_x;
                e.y = a->key_y;
                int n = a->neighbor_count;
                if (n > 5) n = 5;
                e.neighbors.reserve(static_cast<size_t>(n));
                for (int i = 0; i < n; ++i)
                    e.neighbors.emplace_back(a->neighbor_x[i], a->neighbor_y[i]);
                p->out->push_back(std::move(e));
                return 1;
            },
            &c);
        return out;
    }

    // which = 0 -> selection A (revealed Rite-line "purple" maps),
    // which = 1 -> selection B (picked anchors, in pick order).
    std::vector<AtlasGridPoint> Selection(int which) const {
        std::vector<AtlasGridPoint> out;
        if (!m_abi || !m_abi->get_selection) return out;
        struct Ctx { std::vector<AtlasGridPoint>* out; };
        Ctx c{ &out };
        m_abi->get_selection(static_cast<int32_t>(which),
            [](const AtlasGridAbi* g, void* ud) -> int32_t {
                static_cast<Ctx*>(ud)->out->push_back(AtlasGridPoint{ g->x, g->y });
                return 1;
            },
            &c);
        return out;
    }

    // Rite reward-selection seed; 0 = no Rite line / panel absent.
    uint32_t GetLineSeed() const {
        uint32_t seed = 0;
        if (m_abi && m_abi->get_line_seed) m_abi->get_line_seed(&seed);
        return seed;
    }

    // Raw eligibility-weight rows. The path-cap stat id is key 26379 (0.5.4).
    std::vector<AtlasWeight> Weights() const {
        std::vector<AtlasWeight> out;
        if (!m_abi || !m_abi->enumerate_weights) return out;
        struct Ctx { std::vector<AtlasWeight>* out; };
        Ctx c{ &out };
        m_abi->enumerate_weights(
            [](const AtlasWeightAbi* w, void* ud) -> int32_t {
                static_cast<Ctx*>(ud)->out->push_back(AtlasWeight{ w->key, w->value });
                return 1;
            },
            &c);
        return out;
    }

    // Display name for a node's uiAddress ("" when unresolved).
    std::string GetNodeName(uintptr_t uiAddress) const {
        if (!m_abi || !m_abi->get_node_name) return {};
        char buf[128] = {};
        int32_t n = m_abi->get_node_name(uiAddress, buf,
                                         static_cast<int32_t>(sizeof(buf)));
        return (n > 0) ? std::string(buf, static_cast<size_t>(n)) : std::string();
    }
};

// ---------------------------------------------------------------------------
// Sekhema service — Trial of the Sekhemas floor-map data (room graph /
// choices / content FK rows) plus the StateMachine flag reads trial objects
// need, read host-side through the GameLibrary SekhemaTrial offsets. The
// graph calls take the trial panel UI address explicitly: keep your own panel
// discovery (ProbeFloor is the cheap per-node BFS pre-filter) or start from
// GetPanel(), the host's direct child-index resolution.
// ---------------------------------------------------------------------------

// Floor header: per-run dynamic state (choices/counter) + per-layer room
// counts of the static graph. (counter & 7) = choices made.
struct SekhemaFloor {
    bool      valid = false;
    uintptr_t floorData  = 0;        // resolved FloorData base (change signal)
    int       layerCount = 0;
    std::vector<int>     roomCounts; // rooms per layer
    std::vector<uint8_t> choices;    // per-layer chosen room index; 0xFF = none
    uint8_t   counter = 0;
};

// One room of the static floor graph (immutable for a whole floor).
struct SekhemaRoom {
    int layer = 0;
    int index = 0;                   // room index within its layer
    std::vector<int> connections;    // room indices in the NEXT layer
};

// One resolved content FK pair. Dispatch on tablePath and read only the field
// meaningful for that table (SanctumRooms -> rowId, SanctumPersistentEffects
// -> rowName) — the other may hold noise where the row layout differs.
struct SekhemaContentFk {
    uintptr_t   rowAddress   = 0;    // DAT row object (identity/change signal)
    uintptr_t   tableAddress = 0;    // DAT table object
    std::string tablePath;           // in-memory .dat table path
    std::string rowId;               // row Id (e.g. "Caverns_Arena_03")
    std::string rowName;             // localized display name
};

// One content entry: which (layer, room) it decorates + its FK pairs. The
// target indices are delivered as read — bounds-check against the graph
// before joining.
struct SekhemaContentEntry {
    int layer = 0;
    int roomIndex = 0;
    std::vector<SekhemaContentFk> fks;   // up to 3
};

class SekhemaService {
    const SekhemaServiceAbi* m_abi  = nullptr;
    const HostAbi*           m_host = nullptr;
public:
    void Init(const SekhemaServiceAbi* abi, const HostAbi* host) {
        m_abi  = abi;
        m_host = host;
    }

    // Host-resolved trial panel (GameUI child[84], floor-obj gated);
    // 0 = absent / controller mode / not in game.
    uintptr_t GetPanel() const {
        return (m_abi && m_abi->get_panel) ? m_abi->get_panel() : 0;
    }

    // Cheap probe: the layer count of the FloorData resolving from uiAddress,
    // 0 = not a trial floor. Use as the per-node pre-filter when BFS-scanning
    // the UI tree for the panel.
    int ProbeFloor(uintptr_t uiAddress) const {
        return (m_abi && m_abi->probe_floor)
            ? static_cast<int>(m_abi->probe_floor(uiAddress)) : 0;
    }

    // Floor header; {valid=false} when no FloorData resolves from the panel.
    SekhemaFloor GetFloor(uintptr_t panelAddress) const {
        SekhemaFloor out;
        if (!m_abi || !m_abi->get_floor) return out;
        SekhemaFloorAbi abi{};
        if (!m_abi->get_floor(panelAddress, &abi) || !abi.valid) return out;
        out.valid      = true;
        out.floorData  = abi.floor_data;
        out.layerCount = abi.layer_count;
        int n = abi.layer_count;
        if (n < 0) n = 0;
        if (n > 64) n = 64;
        out.roomCounts.reserve(static_cast<size_t>(n));
        out.choices.reserve(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) {
            out.roomCounts.push_back(abi.room_counts[i]);
            out.choices.push_back(abi.choices[i]);
        }
        out.counter = abi.counter;
        return out;
    }

    // Every room of the static floor graph, in (layer, index) order.
    std::vector<SekhemaRoom> Rooms(uintptr_t panelAddress) const {
        std::vector<SekhemaRoom> out;
        if (!m_abi || !m_abi->enumerate_rooms) return out;
        struct Ctx { std::vector<SekhemaRoom>* out; };
        Ctx c{ &out };
        m_abi->enumerate_rooms(panelAddress,
            [](const SekhemaRoomAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                SekhemaRoom r;
                r.layer = a->layer;
                r.index = a->index;
                int n = a->connection_count;
                if (n > 16) n = 16;
                r.connections.reserve(static_cast<size_t>(n > 0 ? n : 0));
                for (int i = 0; i < n; ++i)
                    r.connections.push_back(a->connections[i]);
                p->out->push_back(std::move(r));
                return 1;
            },
            &c);
        return out;
    }

    // Every content entry, FK rows resolved to id/name strings.
    std::vector<SekhemaContentEntry> Content(uintptr_t panelAddress) const {
        std::vector<SekhemaContentEntry> out;
        if (!m_abi || !m_abi->enumerate_content) return out;
        struct Ctx { std::vector<SekhemaContentEntry>* out; };
        Ctx c{ &out };
        m_abi->enumerate_content(panelAddress,
            [](const SekhemaContentAbi* a, void* ud) -> int32_t {
                auto* p = static_cast<Ctx*>(ud);
                SekhemaContentEntry e;
                e.layer = a->layer;
                e.roomIndex = a->room_index;
                int n = a->fk_count;
                if (n > 3) n = 3;
                e.fks.reserve(static_cast<size_t>(n > 0 ? n : 0));
                for (int i = 0; i < n; ++i) {
                    SekhemaContentFk fk;
                    fk.rowAddress   = a->fks[i].row_addr;
                    fk.tableAddress = a->fks[i].table_addr;
                    fk.tablePath    = a->fks[i].table_path;  // NUL-terminated
                    fk.rowId        = a->fks[i].row_id;
                    fk.rowName      = a->fks[i].row_name;
                    e.fks.push_back(std::move(fk));
                }
                p->out->push_back(std::move(e));
                return 1;
            },
            &c);
        return out;
    }

    // StateMachine used/collected/closed flag: 1 = used, 0 = active,
    // -1 = unreadable (null component / host not attached).
    int GetRoomUsedFlag(uintptr_t stateMachineAddress) const {
        if (!m_abi || !m_abi->get_room_used_flag) return -1;
        int32_t used = 0;
        if (!m_abi->get_room_used_flag(stateMachineAddress, &used)) return -1;
        return used ? 1 : 0;
    }

    // One shared-state VALUE (8 bytes per define_shared_state entry, define
    // order — door open/activate states). False when out of range/unreadable.
    bool GetStateMachineValue(uintptr_t stateMachineAddress, int index,
                              uint64_t& outValue) const {
        outValue = 0;
        if (!m_abi || !m_abi->get_state_machine_value) return false;
        return m_abi->get_state_machine_value(stateMachineAddress,
                                              static_cast<int32_t>(index),
                                              &outValue) != 0;
    }

    // A UI element's StringId StdWString as UTF-8 — the field trial-HUD leaves
    // render their numbers into (NOT the Ui.GetText field). "" when empty.
    std::string GetUiStringId(uintptr_t uiAddress) const {
        if (!m_abi || !m_abi->get_ui_string_id) return {};
        char buf[256] = {};
        int32_t n = m_abi->get_ui_string_id(uiAddress, buf,
                                            static_cast<int32_t>(sizeof(buf)));
        return (n > 0) ? std::string(buf, static_cast<size_t>(n)) : std::string();
    }
};

// Bundles the host services plus the ImGui/D3D handles. Accessed via Plugin::ctx().
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
    PricesService     Prices;
    RuneshapeService  Runeshape;
    AtlasService      Atlas;
    SekhemaService    Sekhema;
    void* ImGuiContext = nullptr;
    void* D3DDevice    = nullptr;
};

}

// Wires the host ABI into a plugin instance. Auto-emitted in your DLL when
// PLUGIN_EXPORTS is defined (the inline definition is at the bottom of this
// file) — you never call this yourself.
extern "C" PLUGIN_API void PluginSDK_AttachHost(
    PluginSDK::Plugin* p, const HostAbi* abi, const char* directory);

namespace PluginSDK {

// Base class for every plugin. Subclass it, implement GetName(), and override
// the hooks you need. The host owns the instance (created via your exported
// CreatePlugin) — never delete it yourself. Reach the host through ctx().
class Plugin {
public:
    virtual ~Plugin() = default;

    // Required: a unique, stable plugin name.
    virtual const char* GetName() const = 0;

    // Enabled (arg = the game is already attached). Acquire resources / load config.
    virtual void OnEnable(bool ) {}

    // Disabled or unloaded. Release resources and persist state.
    virtual void OnDisable() {}

    // Draw your controls inside the host's plugin-settings window.
    virtual void DrawSettings() {}

    // Draw every frame (overlay + windows). Set ImGui's context from
    // ctx()->ImGuiContext first; gate work on ctx()->Game.IsInGame() as needed.
    virtual void DrawUI() {}

    // Persist configuration; the host may call this on shutdown.
    virtual void SaveSettings() {}

    // Return true if DrawUI renders an in-game overlay (vs. windows only).
    virtual bool WantsOverlay() const { return false; }

    // ABI version this plugin was built against; the host checks it on load.
    int GetSDKVersion() const { return PLUGIN_SDK_VERSION; }

protected:

    // The host API. Call services as ctx()->Game, ctx()->Entities, etc.
    const Context* ctx() const { return &m_ctx; }

    // This plugin's folder as UTF-8. Prefer DirectoryPath() for a Unicode-safe
    // path when loading/saving files next to the DLL.
    const char* Directory() const { return m_directory.c_str(); }

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

    // False if the host ABI is incompatible (older host) — services are inert.
    bool HostCompatible() const { return m_host_compatible; }

private:
    friend void ::PluginSDK_AttachHost(Plugin*, const HostAbi*, const char*);

    Context     m_ctx{};

    std::string m_directory;
    bool        m_host_compatible = false;
};

}

inline void PluginSDK_AttachHost(PluginSDK::Plugin* p,
                                 const HostAbi*  abi,
                                 const char*        directory) {
    if (!p) return;
    p->m_directory = directory ? directory : "";
    p->m_host_compatible =
        (abi != nullptr
         && abi->version == PLUGIN_SDK_VERSION
         && abi->size_bytes >= sizeof(HostAbi));
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
    p->m_ctx.Overlay   .Init(&abi->overlay,    abi, p);
    p->m_ctx.Flasks    .Init(&abi->flasks,     abi);
    p->m_ctx.Prices    .Init(&abi->prices,     abi);
    p->m_ctx.Runeshape .Init(&abi->runeshape,  abi);
    p->m_ctx.Atlas     .Init(&abi->atlas,      abi);
    p->m_ctx.Sekhema   .Init(&abi->sekhema,    abi);
    p->m_ctx.ImGuiContext = abi->imgui_context;
    p->m_ctx.D3DDevice    = abi->d3d_device;
}
