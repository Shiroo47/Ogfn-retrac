#pragma once
#include <cstdint>

namespace globals {
    inline constexpr std::uintptr_t GObjects = 0x093E0A40;
    inline constexpr std::uintptr_t GNames   = 0x093C3F00;
    inline constexpr std::uintptr_t GWorld   = 0x0951B9D0;
}

enum guobject_array_offsets : std::int32_t {
    TUObjectArray_Objects     = 0x00,
    TUObjectArray_NumElements = 0x14,
    FUObjectItem_Size         = 0x18,
    UObject_VTable            = 0x00,
    UObject_ObjectFlags       = 0x08,
    UObject_InternalIndex     = 0x0C,
    UObject_Class             = 0x10,
    UObject_Name              = 0x18,
    UObject_Outer             = 0x20,
    UStruct_SuperStruct       = 0x40,
    UStruct_Size              = 0x58,
};

namespace class_sizes {
    inline constexpr std::int32_t player_pawn_chain[] = { 0x4130, 0x3AB0, 0x3870, 0x3770 };
    inline constexpr std::int32_t FortPlayerControllerFrontEnd = 0x23C0;
}

enum offsets : std::int32_t {
    GameInstance            = 0x180,
    LocalPlayers            = 0x38,
    PersistentLevel         = 0x30,
    Levels                  = 0x138,
    Actors                  = 0x98,
    GameState               = 0x120,
    PlayerArray             = 0x238,

    PlayerController        = 0x30,
    AcknowledgedPawn        = 0x2A0,

    RootComponent           = 0x130,
    RelativeLocation        = 0x11C,
    RelativeRotation        = 0x128,
    RelativeRotationYaw     = 0x12C,
    ComponentToWorld        = 0x1C0,

    Mesh                    = 0x280,
    BoneArray               = 0x488,
    BoneArrayFallback       = 0x10,
    BoneTransformSize       = 0x30,
    RecentlyRendered        = 0x5DF,
    RecentlyRenderedBit     = 6,

    PlayerState             = 0x240,
    PlayerNamePrivate       = 0x300,
    TeamIndex               = 0xE88,
    bIsDying                = 0x538,
    bIsDBNO                 = 0x552,
    CurrentWeapon           = 0x5C0,
    WeaponData              = 0x378,
    bAlreadySearched        = 0xC71,
    bAlreadySearchedBit     = 7,
    PrimaryPickupItemEntry  = 0x2A8,
    ItemEntryCount          = 0x0C,
    ItemEntryDefinition     = 0x18,
    ItemRarity              = 0x50,
    ItemDisplayName         = 0x70,
    TextDataSourceString    = 0x28,

    PlayerCameraManager     = 0x2B8,
    CameraCachePrivate      = 0x1AA0,
    CameraCachePovOffset    = 0x10,
    MinimalViewInfoFOV      = 0x18,

    CameraPointerA          = 0x70,
    CameraPointerAInner     = 0x98,
    CameraPointerAInner2    = 0x130,
    CameraPointerAOffset    = 0x10,
    CameraTransform         = 0x00,

    ProcessEvent            = 0x376B220,
    FNameToString           = 0x355E530,
};

enum fort_item_tier : std::uint8_t {
    No_Tier = 0,
    I = 1, II = 2, III = 3, IV = 4, V = 5, VI = 6, VII = 7,
    VIII = 8, IX = 9, X = 10
};
