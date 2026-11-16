/*
 * CanineDrivesAPI.h - read-only C API exported by CanineDrives.dll (Canine Drives 0.26.6 and later).
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Nosimo-SN.
 *
 * Resolve the exports with GetProcAddress on GetModuleHandleW(L"CanineDrives.dll"). No Papyrus or ESP dependency.
 *
 *   typedef uint32_t (*CDAPI_GetVersion_t)(void);
 *   typedef uint32_t (*CDAPI_GetSnapshot_t)(uint32_t actorRefID, void* output, uint32_t outputSize);
 *
 * CDAPI_GetVersion returns the API version (CDAPI_VERSION for this header). A later version only appends fields to
 * the snapshot, so code written for version 1 keeps working: pass sizeof(CDAPI_SnapshotV1) and read what you know.
 *
 * CDAPI_GetSnapshot copies the latest published snapshot into output and returns 1, or returns 0 (output untouched)
 * when output is NULL, outputSize < sizeof(CDAPI_SnapshotV1), actorRefID is not the tracked dog's reference, or no
 * snapshot has been published since the game started or since the last save load / new game reset.
 * Safe from any thread: it takes one lock and copies plain data. It touches no game objects and changes nothing.
 * Canine Drives publishes a fresh snapshot about once a second while a game is running.
 */
#ifndef CANINE_DRIVES_API_H
#define CANINE_DRIVES_API_H

#include <stdint.h>

#define CDAPI_VERSION 1u

/* Activity: what the dog is doing. Values are frozen; new ones are only ever added. Treat any value you don't
 * recognize as a generic activity. */
#define CDAPI_ACTIVITY_UNKNOWN    0u  /* not loaded, not found, or none of the below */
#define CDAPI_ACTIVITY_AWAKE      1u  /* awake, following or idle */
#define CDAPI_ACTIVITY_EATING     2u  /* eating: food handed over or found, his catch, or a carcass */
#define CDAPI_ACTIVITY_NAPPING    3u  /* napping (asleep, lying down) */
#define CDAPI_ACTIVITY_HUNTING    4u  /* stalking or chasing prey */
#define CDAPI_ACTIVITY_SNIFFING   5u  /* following a scent */
#define CDAPI_ACTIVITY_PLAYING    6u  /* restless play: running about, zoomies */
#define CDAPI_ACTIVITY_FISHING    7u  /* going after fish in the water */
#define CDAPI_ACTIVITY_SCAVENGING 8u  /* going for food lying about, snatching food, or heading to a carcass */
#define CDAPI_ACTIVITY_GREETING   9u  /* going to greet someone */
#define CDAPI_ACTIVITY_WARMING    10u /* keeping the player warm against the cold */
#define CDAPI_ACTIVITY_FIGHTING   11u /* in combat */

/* Need tier: Canine Drives' own tiers, the same ones that put a need into his prompt. A tier rises as soon as the
 * value reaches its threshold and falls only once the value is hysteresis below it. */
#define CDAPI_TIER_CONTENT    0u
#define CDAPI_TIER_NOTICEABLE 1u
#define CDAPI_TIER_STRONG     2u
#define CDAPI_TIER_URGENT     3u

/* Flags. A warning flag is set while that need's tier is noticeable or higher (the point at which the need reaches
 * his prompt), with the hysteresis above, so it doesn't flicker at the threshold. */
#define CDAPI_FLAG_HUNGER_WARNING 0x00000001u
#define CDAPI_FLAG_TIRED_WARNING  0x00000002u

/* Every field is 4 bytes: no padding, the same layout for any compiler. Booleans are uint32_t 0 or 1. */
typedef struct CDAPI_SnapshotV1
{
	uint32_t structSize;           /* sizeof(CDAPI_SnapshotV1) as built into the DLL: 56 for version 1 */
	uint32_t apiVersion;           /* CDAPI_VERSION of the DLL */
	uint32_t dogRefID;             /* the tracked dog's reference FormID (runtime, load-order resolved) */
	uint32_t enabled;              /* 1 while Canine Drives is enabled in its settings (drives move), else 0 */
	float    hunger;               /* 0-100, unrounded; higher = hungrier */
	float    tiredness;            /* 0-100, unrounded; higher = more tired */
	float    thresholdNoticeable;  /* tier thresholds (0-100), shared by every drive */
	float    thresholdStrong;
	float    thresholdUrgent;
	float    hysteresis;           /* how far below a threshold a value must fall before its tier drops */
	uint32_t hungerTier;           /* CDAPI_TIER_* */
	uint32_t tiredTier;            /* CDAPI_TIER_* */
	uint32_t flags;                /* CDAPI_FLAG_* */
	uint32_t activity;             /* CDAPI_ACTIVITY_* */
} CDAPI_SnapshotV1;

#ifdef __cplusplus
static_assert(sizeof(CDAPI_SnapshotV1) == 56, "CDAPI_SnapshotV1 layout changed");
extern "C" {
#endif

#ifdef CANINE_DRIVES_EXPORTS
#	define CDAPI_EXPORT __declspec(dllexport)
#else
#	define CDAPI_EXPORT __declspec(dllimport)
#endif

CDAPI_EXPORT uint32_t CDAPI_GetVersion(void);
CDAPI_EXPORT uint32_t CDAPI_GetSnapshot(uint32_t actorRefID, void* output, uint32_t outputSize);

#ifdef __cplusplus
}
#endif

#endif /* CANINE_DRIVES_API_H */
