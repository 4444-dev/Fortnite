#pragma once

enum EObjectFlags : u32 {
	RF_NoFlags = 0x00000000,
	RF_Public = 0x00000001,
	RF_Standalone = 0x00000002,
	RF_MarkAsNative = 0x00000004,
	RF_Transactional = 0x00000008,
	RF_ClassDefaultObject = 0x00000010,
	RF_ArchetypeObject = 0x00000020,
	RF_Transient = 0x00000040,
	RF_MarkAsRootSet = 0x00000080,
	RF_TagGarbageTemp = 0x00000100,
	RF_NeedInitialization = 0x00000200,
	RF_NeedLoad = 0x00000400,
	RF_KeepForCooker = 0x00000800,
	RF_NeedPostLoad = 0x00001000,
	RF_NeedPostLoadSubobjects = 0x00002000,
	RF_NewerVersionExists = 0x00004000,
	RF_BeginDestroyed = 0x00008000,
	RF_FinishDestroyed = 0x00010000,
	RF_BeingRegenerated = 0x00020000,
	RF_DefaultSubObject = 0x00040000,
	RF_WasLoaded = 0x00080000,
	RF_TextExportTransient = 0x00100000,
	RF_LoadCompleted = 0x00200000,
	RF_InheritableComponentTemplate = 0x00400000,
	RF_DuplicateTransient = 0x00800000,
	RF_StrongRefOnFrame = 0x01000000,
	RF_NonPIEDuplicateTransient = 0x02000000,
	RF_Dynamic = 0x04000000,
	RF_WillBeLoaded = 0x08000000,
	RF_HasExternalPackage = 0x10000000,
	RF_Garbage = 0x20000000,
	RF_MirroredGarbage = 0x40000000,
	RF_AllocatedInSharedPage = 0x80000000,
};

enum ENetRole : u8 {
	ROLE_None = 0,
	ROLE_SimulatedProxy = 1,
	ROLE_AutonomousProxy = 2,
	ROLE_Authority = 3,
	ROLE_MAX = 4,
};

enum ENetDormancy : u8 {
	DORM_Never = 0,
	DORM_Awake = 1,
	DORM_DormantAll = 2,
	DORM_DormantPartial = 3,
	DORM_Initial = 4,
	DORM_MAX = 5,
};

enum ECollisionChannel : u8 {
	ECC_WorldStatic = 0,
	ECC_WorldDynamic = 1,
	ECC_Pawn = 2,
	ECC_Visibility = 3,
	ECC_Camera = 4,
	ECC_PhysicsBody = 5,
	ECC_Vehicle = 6,
	ECC_Destructible = 7,
	ECC_EngineTraceChannel1 = 8,
	ECC_EngineTraceChannel2 = 9,
	ECC_EngineTraceChannel3 = 10,
	ECC_EngineTraceChannel4 = 11,
	ECC_EngineTraceChannel5 = 12,
	ECC_EngineTraceChannel6 = 13,
	ECC_GameTraceChannel1 = 14,
	ECC_GameTraceChannel2 = 15,
	ECC_GameTraceChannel3 = 16,
	ECC_GameTraceChannel4 = 17,
	ECC_GameTraceChannel5 = 18,
	ECC_GameTraceChannel6 = 19,
	ECC_GameTraceChannel7 = 20,
	ECC_GameTraceChannel8 = 21,
	ECC_GameTraceChannel9 = 22,
	ECC_GameTraceChannel10 = 23,
	ECC_GameTraceChannel11 = 24,
	ECC_GameTraceChannel12 = 25,
	ECC_GameTraceChannel13 = 26,
	ECC_GameTraceChannel14 = 27,
	ECC_GameTraceChannel15 = 28,
	ECC_GameTraceChannel16 = 29,
	ECC_GameTraceChannel17 = 30,
	ECC_GameTraceChannel18 = 31,
	ECC_OverlapAll_Deprecated = 32,
	ECC_MAX = 33,
};

enum class ECollisionEnabled : u8 {
	NoCollision = 0,
	QueryOnly = 1,
	PhysicsOnly = 2,
	QueryAndPhysics = 3,
	ProbeOnly = 4,
	QueryAndProbe = 5,
};

enum class EComponentMobility : u8 {
	Static = 0,
	Stationary = 1,
	Movable = 2,
};

enum EMovementMode : u8 {
	MOVE_None = 0,
	MOVE_Walking = 1,
	MOVE_NavWalking = 2,
	MOVE_Falling = 3,
	MOVE_Swimming = 4,
	MOVE_Flying = 5,
	MOVE_Custom = 6,
	MOVE_MAX = 7,
};

enum ETickingGroup : u8 {
	TG_PrePhysics = 0,
	TG_StartPhysics = 1,
	TG_DuringPhysics = 2,
	TG_EndPhysics = 3,
	TG_PostPhysics = 4,
	TG_PostUpdateWork = 5,
	TG_LastDemotable = 6,
	TG_NewlySpawned = 7,
	TG_MAX = 8,
};

enum class EFortRarity : u8 {
	Common = 0,
	Uncommon = 1,
	Rare = 2,
	Epic = 3,
	Legendary = 4,
	Mythic = 5,
	Transcendent = 6,
	Unattainable = 7,
	NumRarityValues = 8,
};

enum class EFortItemTier : u8 {
	No_Tier = 0,
	I = 1,
	II = 2,
	III = 3,
	IV = 4,
	V = 5,
	VI = 6,
	VII = 7,
	VIII = 8,
	IX = 9,
	X = 10,
	NumItemTierValues = 11,
};

enum class EFortItemType : u8 {
	WorldItem = 0,
	Ammo = 1,
	Utility = 2,
	Trap = 3,
	CampaignHero = 4,
	BuildingPiece = 5,
	EditTool = 6,
	BuildingMod = 7,
	WorldResource = 8,
	Deployment = 9,
	Consumable = 10,
	Ingredient = 11,
	AccountResource = 12,
	Token = 13,
	Collectible = 14,
	AccountItem = 15,
	Schematic = 16,
	CardPack = 17,
	Character = 18,
	Defender = 19,
	Weapon = 20,
	WeaponMelee = 21,
	WeaponRanged = 22,
	WeaponHarvest = 23,
	WeaponMod = 24,
	Gadget = 25,
	AbilityKit = 26,
	Stat = 27,
	Buff = 28,
	BuffCredit = 29,
	Pickaxe = 30,
	Quest = 31,
	CodeToken = 32,
	Backpack = 33,
	HomebaseNode = 34,
	Worker = 35,
	LeadSurvivor = 36,
	Manager = 37,
	GameplayModifier = 38,
	Mission = 39,
	Outpost = 40,
	SkillTreeNode = 41,
	Badge = 42,
	PlayerAugment = 43,
	Medal = 44,
	GiftBox = 45,
	GiftBoxUnlock = 46,
	Playset = 47,
	Preroll = 48,
	PlayerSurveyToken = 49,
	CosmeticVariantToken = 50,
	SpecialItem = 51,
	AccountTrophy = 52,
	Profile = 53,
	Max = 54,
};

enum class EFortCustomPartType : u8 {
	Head = 0,
	Body = 1,
	Hat = 2,
	Backpack = 3,
	MiscOrTail = 4,
	Face = 5,
	Gameplay = 6,
	NumTypes = 7,
};

enum class EFortCustomGender : u8 {
	Invalid = 0,
	Male = 1,
	Female = 2,
	Both = 3,
};

enum class EFortPickupSpawnSource : u8 {
	Unset = 0,
	PlayerElimination = 1,
	Chest = 2,
	SupplyDrop = 3,
	AmmoBox = 4,
};

enum class EFortPickupTossState : u8 {
	NotTossed = 0,
	InProgress = 1,
	AtRest = 2,
};

enum class EFortWeaponTriggerType : u8 {
	OnPress = 0,
	Automatic = 1,
	OnRelease = 2,
	OnPressAndRelease = 3,
};

enum class EFortWeaponCoreAnimation : u8 {
	Melee = 0,
	Pistol = 1,
	Shotgun = 2,
	PaperBlueprint = 3,
	Rifle = 4,
	MeleeOneHand = 5,
	MeleeTwoHand = 6,
	HandheldCannon = 7,
	Bow = 8,
	Minigun = 9,
	Launcher = 10,
	Thrown = 11,
	Harvest = 12,
	ExtraSpecial = 13,
	Special = 14,
	Max = 15,
};

enum class EFortBuildingType : u8 {
	Wall = 0,
	Floor = 1,
	Corner = 2,
	Deco = 3,
	Prop = 4,
	Stairs = 5,
	Roof = 6,
	Pillar = 7,
	SpawnedItem = 8,
	Container = 9,
	Trap = 10,
	GenericCenterCellActor = 11,
	Connector = 12,
	None = 13,
};

enum class EFortBuildingState : u8 {
	Building = 0,
	Editing = 1,
	None = 2,
};

enum class EFortResourceType : u8 {
	Wood = 0,
	Stone = 1,
	Metal = 2,
	Permanite = 3,
	GoldCurrency = 4,
	IngredientCapsule = 5,
	None = 6,
};

enum class EFortMovementStyle : u8 {
	Running = 0,
	Sprinting = 1,
	Crouching = 2,
	Charging = 3,
	InVehicle = 4,
	Flying = 5,
	Tethered = 6,
	Burrowing = 7,
	EvasiveSpring = 8,
	Hovering = 9,
};

enum class EAthenaGamePhase : u8 {
	None = 0,
	Setup = 1,
	Warmup = 2,
	Aircraft = 3,
	SafeZones = 4,
	EndGame = 5,
	Count = 6,
};

enum class EFortPawnStasisMode : u8 {
	None = 0,
	NoMovement = 1,
	NoMovementOrTurning = 2,
	NoMovementOrFalling = 3,
};

enum class EFortSkeleton : i32 {
	Root = 0,
	Attach = 1,
	Pelvis = 2,
	Spine_01 = 3,
	Spine_02 = 4,
	Spine_03 = 5,
	Spine_04 = 6,
	Spine_05 = 7,
	Clavicle_L = 8,
	Upperarm_L = 9,
	Lowerarm_L = 10,
	Hand_L = 11,
	Clavicle_R = 37,
	Upperarm_R = 38,
	Lowerarm_R = 39,
	Hand_R = 40,
	Neck_01 = 66,
	Neck_02 = 67,
	Chest = 68,
	Head = 110,
	Thigh_L = 71,
	Calf_L = 72,
	Foot_L = 86,
	Thigh_R = 78,
	Calf_R = 79,
	Foot_R = 87,
	Weapon_L = 31,
	Weapon_R = 60,
};
