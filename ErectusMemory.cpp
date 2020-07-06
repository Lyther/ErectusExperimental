#include "ErectusMemory.h"

#include "common.h"

#include "settings.h"
#include "renderer.h"
#include "utils.h"

#include "fmt/format.h"
#include <array>
#include <set>
#include <unordered_set>

#include "ErectusProcess.h"

namespace
{
	enum class FormTypes : BYTE
	{
		TesObjectArmo = 0x26,
		TesObjectBook = 0x27,
		TesObjectCont = 0x28,
		TesObjectMisc = 0x2C,
		CurrencyObject = 0x2F,
		TesFlora = 0x35,
		TesObjectWeap = 0x37,
		TesAmmo = 0x38,
		TesNpc = 0x39,
		TesKey = 0x3C,
		AlchemyItem = 0x3D,
		TesUtilityItem = 0x3E,
		BgsNote = 0x40,
		TesLevItem = 0x48,
		TesObjectRefr = 0x50,  //used in REFR objects, ref to item
		TesActor = 0x51, //used in REFR objects, ref to npc
		PlayerCharacter = 0xB5 //also used in REFR objects, ref to player
	};

	DWORD legendaryFormIdArray[]
	{
		0x00425E28, 0x004392CD, 0x0037F7D9, 0x001A7B80, 0x001A7AF6, 0x001A7BE2, 0x001A7BD3, 0x001A7AB2, 0x001A7B88,
		0x001A7BDA, 0x001A7C39, 0x0052BDC7, 0x0052BDC5, 0x0052BDC2, 0x0052BDC8, 0x0052BDB4, 0x0052BDB5, 0x0052BDB6,
		0x0052BDB7, 0x0052BDBA, 0x0052BDBC, 0x0052BDBF, 0x005299F5, 0x005299ED, 0x00529A14, 0x005299FE, 0x00529A0F,
		0x00529A0C, 0x00529A09, 0x005299F9, 0x005299FA, 0x005299FC, 0x00529A05, 0x00529A04, 0x005299FB, 0x00529A03,
		0x005299FD, 0x00529A02, 0x005281B8, 0x005281B4, 0x00527F6F, 0x00527F72, 0x00527F6E, 0x00527F7D, 0x00527F75,
		0x00527F6C, 0x00527F6D, 0x00527F74, 0x00527F84, 0x00527F82, 0x00527F8B, 0x00527F81, 0x00527F78, 0x00527F76,
		0x00527F7F, 0x00527F77, 0x00527F79, 0x00527F7A, 0x00527F7B, 0x00525400, 0x00525401, 0x005253FB, 0x0052414C,
		0x00524143, 0x0052414E, 0x0052414F, 0x00524150, 0x00524152, 0x00524153, 0x00524154, 0x00524146, 0x00524147,
		0x0052414A, 0x0052414B, 0x00521914, 0x00521915, 0x004F6D77, 0x004F6D7C, 0x004F6D86, 0x004F6D76, 0x004F6D85,
		0x004F6D84, 0x004F6D82, 0x004F6D83, 0x004F6D81, 0x004F6D80, 0x004F6D7F, 0x004F6D78, 0x004F6D7E, 0x004F6D7D,
		0x004F6AAE, 0x004F6AAB, 0x004F6AA1, 0x004F6AA0, 0x004F6AA7, 0x004F6AA5, 0x004F6AB1, 0x004F5772, 0x004F5778,
		0x004F5770, 0x004F5773, 0x004F577C, 0x004F5771, 0x004F5777, 0x004F5776, 0x004F577D, 0x004F577B, 0x004F577A,
		0x004F5779, 0x004EE548, 0x004EE54B, 0x004EE54C, 0x004EE54E, 0x004ED02B, 0x004ED02E, 0x004ED02C, 0x004ED02F,
		0x004E89B3, 0x004E89B2, 0x004E89AC, 0x004E89B4, 0x004E89B0, 0x004E89AF, 0x004E89AE, 0x004E89B6, 0x004E89AD,
		0x004E89B5, 0x003C4E27, 0x003C3458, 0x00357FBF, 0x001142A8, 0x0011410E, 0x0011410D, 0x0011410C, 0x0011410B,
		0x0011410A, 0x00114109, 0x00114108, 0x00114107, 0x00114106, 0x00114105, 0x00114104, 0x00114103, 0x00114101,
		0x001140FF, 0x001140FD, 0x001140FC, 0x001140FB, 0x001140FA, 0x001140F8, 0x001140F2, 0x001140F1, 0x001140F0,
		0x001140EF, 0x001140EE, 0x001140ED, 0x001140EC, 0x001140EB, 0x001140EA, 0x00113FC0, 0x001138DD, 0x0011384A,
		0x0011374F, 0x0011371F, 0x0010F599, 0x0010F598, 0x0010F596, 0x00226436, 0x001F81EB, 0x001F7A75, 0x001F1E47,
		0x001F1E0C, 0x001F1E0B, 0x001E73BD,
	};
}

DWORD64 ErectusMemory::GetAddress(const DWORD formId)
{
	//this is basically hashmap.find(key)
	//v1+24 == hashmap start (_entries)
	//v1+32 == capacity (_capacity)
	//
	//here the hashmap is hashmap<formId, TesObjectRefr*>
	//item/entry is { std::pair<formId, TesObjectRefr*> value, Entry* next }
	//
	//all this is doing is:
	//find(key) {
	//	if(!_entries)
	//		return 0;
	//
	//	item = _entries[calc_idx(key)]; //ideal position
	//
	//	if(!item->next)
	//		return 0;
	//	do{
	//		if(item->value.first == key)
	//			return item->value.second;
	//		else
	//			item = item->next;
	//	}
	//}
	//
	//calc_idx(key) {
	//	return crc32hash(key) & (_capacity - 1); //note, this is a homegrown crc32 implementation
	//}
	//
	//==> should be able to refactor this into reading an array of struct {DWORD64 formId, DWORD64 address, DWORD64 pad}
	//==> then just loop over it and check formid
	//==> the array will be huge but probably still worth it


	DWORD64 v1;
	if (!Rpm(ErectusProcess::exe + OFFSET_GET_PTR_A1, &v1, sizeof v1))
		return 0;
	if (!Utils::Valid(v1))
		return 0;

	DWORD _capacity;
	if (!Rpm(v1 + 32, &_capacity, sizeof _capacity))
		return 0;
	if (!_capacity)
		return 0;

	//hash = crc32hash(formId)
	DWORD hash = 0;
	for (auto i = 0; i < sizeof formId; i++)
	{
		const auto v4 = (hash ^ formId >> i * 0x8) & 0xFF;

		DWORD v5;
		if (!Rpm(ErectusProcess::exe + OFFSET_GET_PTR_A2 + v4 * 0x4, &v5, sizeof v5)) //OFFSET_GET_PTR_A2 is just the start of a crc32 lookup table
			return 0;

		hash = hash >> 0x8 ^ v5;
	}

	auto _index = hash & _capacity - 1;

	DWORD64 entries;

	//sanity check: array exists
	if (!Rpm(v1 + 24, &entries, sizeof entries))
		return 0;
	if (!Utils::Valid(entries))
		return 0;

	//check item->next != -1
	DWORD next;
	if (!Rpm(entries + _index * 24 + 16, &next, sizeof next))
		return 0;
	if (next == 0xFFFFFFFF)
		return 0;

	auto v9 = _capacity;


	for (auto i = 0; i < 100; i++)
	{
		DWORD v10;
		if (!Rpm(entries + _index * 24, &v10, sizeof v10)) //item->value.first
			return 0;
		if (v10 == formId)
		{
			v9 = _index; //item->value
			if (v9 != _capacity)
				break;
		}
		else
		{
			if (!Rpm(entries + _index * 24 + 16, &_index, sizeof _index)) //item = item->next
				return 0;
			if (_index == _capacity)
				break;
		}
	}

	if (v9 == _capacity) return 0;

	return entries + v9 * 24 + 8; //item->value.second
}

DWORD64 ErectusMemory::GetPtr(const DWORD formId)
{
	const auto address = GetAddress(formId);
	if (!address)
		return 0;

	DWORD64 ptr;
	if (!Rpm(address, &ptr, sizeof ptr))
		return 0;

	return ptr;
}

DWORD64 ErectusMemory::GetCameraPtr()
{
	DWORD64 cameraPtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_CAMERA, &cameraPtr, sizeof cameraPtr))
		return 0;

	return cameraPtr;
}

BYTE ErectusMemory::CheckHealthFlag(const BYTE healthFlag)
{
	auto flag = healthFlag;
	flag &= ~(1 << 7);
	flag &= ~(1 << 6);
	flag &= ~(1 << 5);
	switch (flag)
	{
	case 0x00: //Alive
		return 0x01;
	case 0x02: //Dead
	case 0x04: //Dead
		return 0x03;
	case 0x10: //Downed
	case 0x12: //Downed
		return 0x02;
	default: //Unknown
		return 0x00;
	}
}

DWORD64 ErectusMemory::GetLocalPlayerPtr(const bool checkMainMenu)
{
	DWORD64 localPlayerPtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_LOCAL_PLAYER, &localPlayerPtr, sizeof localPlayerPtr))
		return 0;
	if (!Utils::Valid(localPlayerPtr))
		return 0;

	if (checkMainMenu)
	{
		TesObjectRefr localPlayerData{};
		if (!Rpm(localPlayerPtr, &localPlayerData, sizeof localPlayerData))
			return 0;
		if (localPlayerData.formId == 0x00000014)
			return 0;
	}

	return localPlayerPtr;
}

std::vector<DWORD64> ErectusMemory::GetEntityPtrList()
{
	std::vector<DWORD64> result;

	DWORD64 entityListTypePtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_ENTITY_LIST, &entityListTypePtr, sizeof entityListTypePtr))
		return result;
	if (!Utils::Valid(entityListTypePtr))
		return result;

	//1) Get LoadedAreaManager
	LoadedAreaManager manager{};
	if (!Rpm(entityListTypePtr, &manager, sizeof manager))
		return result;
	if (!Utils::Valid(manager.interiorCellArrayPtr) || !Utils::Valid(manager.interiorCellArrayPtr2) || !Utils::Valid(manager.exteriorCellArrayPtr) || !Utils::Valid(manager.exteriorCellArrayPtr2))
		return result;

	DWORD64 cellPtrArrayPtr;
	int cellPtrArraySize;

	//2) Select  interior or exterior objectlist
	if (manager.interiorCellArrayPtr != manager.interiorCellArrayPtr2)
	{
		cellPtrArrayPtr = manager.interiorCellArrayPtr;
		cellPtrArraySize = 2;
	}
	else if (manager.exteriorCellArrayPtr != manager.exteriorCellArrayPtr2)
	{
		cellPtrArrayPtr = manager.exteriorCellArrayPtr;
		cellPtrArraySize = 50;
	}
	else return result; // sthg went wrong

	//3) Read the array of pointers to cells
	auto cellPtrArray = std::make_unique<DWORD64[]>(cellPtrArraySize);
	if (!Rpm(cellPtrArrayPtr, cellPtrArray.get(), cellPtrArraySize * sizeof DWORD64))
		return result;

	//4) Read each cell and push object pointers into objectPtrs
	for (auto i = 0; i < cellPtrArraySize; i++)
	{
		if (i % 2 == 0)
			continue;

		TesObjectCell cell{};
		if (!Rpm(cellPtrArray[i], &cell, sizeof TesObjectCell))
			continue;
		if (!Utils::Valid(cell.objectListBeginPtr) || !Utils::Valid(cell.objectListEndPtr))
			continue;

		auto itemArraySize = (cell.objectListEndPtr - cell.objectListBeginPtr) / sizeof(DWORD64);
		auto objectPtrArray = std::make_unique<DWORD64[]>(itemArraySize);
		if (!Rpm(cell.objectListBeginPtr, objectPtrArray.get(), itemArraySize * sizeof DWORD64))
			continue;

		result.insert(result.end(), objectPtrArray.get(), objectPtrArray.get() + itemArraySize);
	}

	return  result;
}

bool ErectusMemory::IsRecipeKnown(const DWORD formId)
{
	//the list of known recipes is implemented as a set / rb-tree at [localplayer + 0xDB0]+0x8.
	struct SetEntry
	{
		DWORD64 left; //0x0000
		DWORD64  parent; //0x0008
		DWORD64  right; //0x0010
		char pad0018[1]; //0x0018
		BYTE isLeaf; //0x0019
		char pad001A[2]; //0x001A
		DWORD value; //0x001C
	} setEntry = {};

	auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	DWORD64 setPtr;
	if (!Rpm(localPlayerPtr + 0xDB0, &setPtr, sizeof setPtr))
		return false;

	if (!Rpm(setPtr + 0x8, &setPtr, sizeof setPtr))
		return false;

	if (!Rpm(setPtr, &setEntry, sizeof setEntry))
		return false;

	while (!setEntry.isLeaf)
	{
		if (setEntry.value == formId)
			return true;
		if (setEntry.value > formId)
		{
			if (!Rpm(setEntry.left, &setEntry, sizeof setEntry))
				return false;
		}
		else
		{
			if (!Rpm(setEntry.right, &setEntry, sizeof setEntry))
				return false;
		}
	}
	return false;
}

bool ErectusMemory::CheckFormIdArray(const DWORD formId, const bool* enabledArray, const DWORD* formIdArray, const int size)
{
	for (auto i = 0; i < size; i++)
	{
		if (formId == formIdArray[i])
			return enabledArray[i];
	}

	return false;
}

bool ErectusMemory::CheckReferenceJunk(const TesItem& referenceData)
{
	if (referenceData.componentArraySize)
	{
		if (!(referenceData.recordFlagA >> 7 & 1))
			return true;
	}

	return false;
}

bool ErectusMemory::CheckReferenceMod(const TesItem& referenceData)
{
	if (referenceData.recordFlagA >> 7 & 1)
		return true;

	return false;
}

bool ErectusMemory::CheckReferencePlan(const TesItem& referenceData)
{
	if (referenceData.planFlag >> 5 & 1)
	{
		return true;
	}

	return false;
}

bool ErectusMemory::CheckScrapList()
{
	for (auto i : Settings::scrapLooter.enabledList)
	{
		if (i)
			return true;
	}

	return false;
}

bool ErectusMemory::CheckItemLooterList()
{
	for (auto i = 0; i < 100; i++)
	{
		if (Settings::itemLooter.formIdList[i] && Settings::itemLooter.enabledList[i])
			return true;
	}

	return false;
}

bool ErectusMemory::CheckItemLooterBlacklist()
{
	if (Settings::itemLooter.blacklistToggle)
	{
		for (auto i = 0; i < 64; i++)
		{
			if (Settings::itemLooter.blacklist[i] && Settings::itemLooter.blacklistEnabled[i])
				return true;
		}
	}

	return false;
}

bool ErectusMemory::CheckEntityLooterList(const EntityLooterSettings& settings)
{
	for (auto i = 0; i < 100; i++)
	{
		if (settings.formIdList[i] && settings.enabledList[i])
			return true;
	}

	return false;
}

bool ErectusMemory::CheckEntityLooterBlacklist(const EntityLooterSettings& settings)
{
	if (!settings.blacklistToggle)
		return false;

	for (auto i = 0; i < 64; i++)
	{
		if (settings.blacklist[i] && settings.blacklistEnabled[i])
			return true;
	}

	return false;
}

bool ErectusMemory::CheckIngredientList()
{
	for (auto i : Settings::harvester.enabledList)
	{
		if (i)
			return true;
	}

	return false;
}

bool ErectusMemory::CheckJunkPileEnabled()
{
	for (auto i = 0; i < 69; i++)
	{
		if (!strcmp(Settings::harvester.nameList[i], "Junk Pile"))
			return Settings::harvester.enabledList[i];
	}

	return false;
}

bool ErectusMemory::CheckComponentArray(const TesItem& referenceData)
{
	if (!referenceData.componentArraySize || referenceData.componentArraySize > 0x7FFF)
		return false;

	if (!Utils::Valid(referenceData.componentArrayPtr))
		return false;

	auto* componentArray = new Component[referenceData.componentArraySize];
	if (!Rpm(referenceData.componentArrayPtr, &*componentArray, referenceData.componentArraySize * sizeof(Component)))
	{
		delete[]componentArray;
		componentArray = nullptr;
		return false;
	}

	for (auto i = 0; i < referenceData.componentArraySize; i++)
	{
		if (!Utils::Valid(componentArray[i].componentReferencePtr))
			continue;
		if (!Utils::Valid(componentArray[i].componentCountReferencePtr))
			continue;

		TesItem componentData{};
		if (!Rpm(componentArray[i].componentReferencePtr, &componentData, sizeof componentData))
			continue;
		if (CheckFormIdArray(componentData.formId, Settings::scrapLooter.enabledList, Settings::scrapLooter.formIdList, 40))
		{
			delete[]componentArray;
			componentArray = nullptr;
			return true;
		}
	}

	delete[]componentArray;
	componentArray = nullptr;
	return false;
}

bool ErectusMemory::CheckReferenceKeywordBook(const TesItem& referenceData, const DWORD formId)
{
	if (!referenceData.keywordArrayData01C0 || referenceData.keywordArrayData01C0 > 0x7FFF)
		return false;
	if (!Utils::Valid(referenceData.keywordArrayData01B8))
		return false;

	auto* keywordArray = new DWORD64[referenceData.keywordArrayData01C0];
	if (!Rpm(referenceData.keywordArrayData01B8, &*keywordArray, referenceData.keywordArrayData01C0 * sizeof(DWORD64)))
	{
		delete[]keywordArray;
		keywordArray = nullptr;
		return false;
	}

	for (DWORD64 i = 0; i < referenceData.keywordArrayData01C0; i++)
	{
		if (!Utils::Valid(keywordArray[i]))
			continue;

		DWORD formIdCheck;
		if (!Rpm(keywordArray[i] + 0x20, &formIdCheck, sizeof formIdCheck))
			continue;
		if (formIdCheck != formId)
			continue;

		delete[]keywordArray;
		keywordArray = nullptr;
		return true;
	}

	delete[]keywordArray;
	keywordArray = nullptr;
	return false;
}

bool ErectusMemory::CheckReferenceKeywordMisc(const TesItem& referenceData, const DWORD formId)
{
	if (!referenceData.keywordArrayData01B8 || referenceData.keywordArrayData01B8 > 0x7FFF)
		return false;
	if (!Utils::Valid(referenceData.keywordArrayData01B0))
		return false;

	auto* keywordArray = new DWORD64[referenceData.keywordArrayData01B8];
	if (!Rpm(referenceData.keywordArrayData01B0, &*keywordArray, referenceData.keywordArrayData01B8 * sizeof(DWORD64)))
	{
		delete[]keywordArray;
		keywordArray = nullptr;
		return false;
	}

	for (DWORD64 i = 0; i < referenceData.keywordArrayData01B8; i++)
	{
		if (!Utils::Valid(keywordArray[i]))
			continue;

		DWORD formIdCheck;
		if (!Rpm(keywordArray[i] + 0x20, &formIdCheck, sizeof formIdCheck))
			continue;
		if (formIdCheck != formId)
			continue;

		delete[]keywordArray;
		keywordArray = nullptr;
		return true;
	}

	delete[]keywordArray;
	keywordArray = nullptr;
	return false;
}

bool ErectusMemory::CheckWhitelistedFlux(const TesItem& referenceData)
{
	if (!Utils::Valid(referenceData.harvestedPtr))
		return false;

	DWORD formIdCheck;
	if (!Rpm(referenceData.harvestedPtr + 0x20, &formIdCheck, sizeof formIdCheck))
		return false;

	switch (formIdCheck)
	{
	case 0x002DDD45: //Raw Crimson Flux
		return Settings::customFluxSettings.crimsonFluxEnabled;
	case 0x002DDD46: //Raw Cobalt Flux
		return Settings::customFluxSettings.cobaltFluxEnabled;
	case 0x002DDD49: //Raw Yellowcake Flux
		return Settings::customFluxSettings.yellowcakeFluxEnabled;
	case 0x002DDD4B: //Raw Fluorescent Flux
		return Settings::customFluxSettings.fluorescentFluxEnabled;
	case 0x002DDD4D: //Raw Violet Flux
		return Settings::customFluxSettings.violetFluxEnabled;
	default:
		return false;
	}
}

bool ErectusMemory::FloraLeveledListValid(const LeveledList& leveledListData)
{
	if (!Utils::Valid(leveledListData.listEntryArrayPtr) || !leveledListData.listEntryArraySize)
		return false;

	auto* listEntryData = new ListEntry[leveledListData.listEntryArraySize];
	if (!Rpm(leveledListData.listEntryArrayPtr, &*listEntryData, leveledListData.listEntryArraySize * sizeof(ListEntry)))
	{
		delete[]listEntryData;
		listEntryData = nullptr;
		return false;
	}

	for (BYTE i = 0; i < leveledListData.listEntryArraySize; i++)
	{
		if (!Utils::Valid(listEntryData[i].referencePtr))
			continue;

		TesItem referenceData{};
		if (!Rpm(listEntryData[i].referencePtr, &referenceData, sizeof referenceData))
			continue;
		if (referenceData.formType == static_cast<BYTE>(FormTypes::TesLevItem))
		{
			LeveledList recursiveLeveledListData{};
			memcpy(&recursiveLeveledListData, &referenceData, sizeof recursiveLeveledListData);
			if (FloraLeveledListValid(recursiveLeveledListData))
			{
				delete[]listEntryData;
				listEntryData = nullptr;
				return true;
			}
		}
		else if (CheckFormIdArray(referenceData.formId, Settings::harvester.enabledList, Settings::harvester.formIdList, 69))
		{
			delete[]listEntryData;
			listEntryData = nullptr;
			return true;
		}
	}

	delete[]listEntryData;
	listEntryData = nullptr;
	return false;
}

bool ErectusMemory::FloraValid(const TesItem& referenceData)
{
	if (referenceData.formId == 0x000183C6)
		return CheckJunkPileEnabled();

	if (!Utils::Valid(referenceData.harvestedPtr))
		return false;

	TesItem harvestedData{};
	if (!Rpm(referenceData.harvestedPtr, &harvestedData, sizeof harvestedData))
		return false;
	if (referenceData.formType == static_cast<BYTE>(FormTypes::TesLevItem))
	{
		LeveledList leveledListData{};
		memcpy(&leveledListData, &harvestedData, sizeof leveledListData);
		return FloraLeveledListValid(leveledListData);
	}
	return CheckFormIdArray(harvestedData.formId, Settings::harvester.enabledList, Settings::harvester.formIdList, 69);
}

bool ErectusMemory::IsTreasureMap(const DWORD formId)
{
	switch (formId)
	{
	case 0x0051B8CD: //Cranberry Bog Treasure Map #01
	case 0x0051B8D6: //Cranberry Bog Treasure Map #02
	case 0x0051B8D9: //Cranberry Bog Treasure Map #03
	case 0x0051B8DE: //Cranberry Bog Treasure Map #04
	case 0x0051B8CE: //Mire Treasure Map #01
	case 0x0051B8D2: //Mire Treasure Map #02
	case 0x0051B8D7: //Mire Treasure Map #03
	case 0x0051B8D8: //Mire Treasure Map #04
	case 0x0051B8DB: //Mire Treasure Map #05
	case 0x0051B8BA: //Savage Divide Treasure Map #01
	case 0x0051B8C0: //Savage Divide Treasure Map #02
	case 0x0051B8C2: //Savage Divide Treasure Map #03
	case 0x0051B8C4: //Savage Divide Treasure Map #04
	case 0x0051B8C6: //Savage Divide Treasure Map #05
	case 0x0051B8C7: //Savage Divide Treasure Map #06
	case 0x0051B8C8: //Savage Divide Treasure Map #07
	case 0x0051B8CA: //Savage Divide Treasure Map #08
	case 0x0051B8CC: //Savage Divide Treasure Map #09
	case 0x0051B8D4: //Savage Divide Treasure Map #10
	case 0x0051B8B1: //Toxic Valley Treasure Map #01
	case 0x0051B8B8: //Toxic Valley Treasure Map #02
	case 0x0051B8BC: //Toxic Valley Treasure Map #03
	case 0x0051B8C1: //Toxic Valley Treasure Map #04
	case 0x0051B7A2: //Forest Treasure Map #01
	case 0x0051B8A6: //Forest Treasure Map #02
	case 0x0051B8A7: //Forest Treasure Map #03
	case 0x0051B8A9: //Forest Treasure Map #04
	case 0x0051B8AA: //Forest Treasure Map #05
	case 0x0051B8AE: //Forest Treasure Map #06
	case 0x0051B8B0: //Forest Treasure Map #07
	case 0x0051B8B2: //Forest Treasure Map #08
	case 0x0051B8B6: //Forest Treasure Map #09
	case 0x0051B8B9: //Forest Treasure Map #10
	case 0x0051B8A8: //Ash Heap Treasure Map #01
	case 0x0051B8AC: //Ash Heap Treasure Map #02
		return true;
	default:
		return false;
	}
}

bool ErectusMemory::CheckFactionFormId(const DWORD formId)
{
	switch (formId)
	{
	case 0x003FC008: //W05_SettlerFaction
		return Settings::customExtraNpcSettings.hideSettlerFaction;
	case 0x003FE94A: //W05_CraterRaiderFaction
		return Settings::customExtraNpcSettings.hideCraterRaiderFaction;
	case 0x003FBC00: //W05_DieHardFaction
		return Settings::customExtraNpcSettings.hideDieHardFaction;
	case 0x005427B2: //W05_SecretServiceFaction
		return Settings::customExtraNpcSettings.hideSecretServiceFaction;
	default:
		return false;
	}
}

bool ErectusMemory::BlacklistedNpcFaction(const TesItem& referenceData)
{
	if (!Utils::Valid(referenceData.factionArrayPtr) || !referenceData.factionArraySize || referenceData.factionArraySize > 0x7FFF)
		return false;

	auto* factionArray = new DWORD64[referenceData.factionArraySize * 2];
	if (!Rpm(referenceData.factionArrayPtr, &*factionArray, referenceData.factionArraySize * 2 * sizeof(DWORD64)))
	{
		delete[]factionArray;
		factionArray = nullptr;
		return false;
	}

	auto blacklistedFactionFound = false;
	for (auto i = 0; i < referenceData.factionArraySize; i++)
	{
		if (!Utils::Valid(factionArray[i * 2]))
			continue;

		DWORD formId;
		if (!Rpm(factionArray[i * 2] + 0x20, &formId, sizeof formId))
			continue;
		if (CheckFactionFormId(formId))
		{
			blacklistedFactionFound = true;
			break;
		}
	}

	delete[]factionArray;
	factionArray = nullptr;
	return blacklistedFactionFound;
}

bool ErectusMemory::CheckReferenceItem(const TesItem& referenceData)
{
	switch (referenceData.formType)
	{
	case (static_cast<BYTE>(FormTypes::TesUtilityItem)):
	case (static_cast<BYTE>(FormTypes::BgsNote)):
	case (static_cast<BYTE>(FormTypes::TesKey)):
	case (static_cast<BYTE>(FormTypes::CurrencyObject)):
		return true;
	default:
		return false;
	}
}

void ErectusMemory::GetCustomEntityData(const TesItem& referenceData, DWORD64* entityFlag, DWORD64* entityNamePtr,
	int* enabledDistance, const bool checkScrap, const bool checkIngredient)
{
	switch (referenceData.formType)
	{
	case (static_cast<byte>(FormTypes::TesNpc)):
		*entityFlag |= CUSTOM_ENTRY_NPC;
		*entityNamePtr = referenceData.namePtr0160;
		*enabledDistance = Settings::npcSettings.enabledDistance;
		break;
	case (static_cast<byte>(FormTypes::TesObjectCont)):
		*entityFlag |= CUSTOM_ENTRY_CONTAINER;
		*entityNamePtr = referenceData.namePtr00B0;
		*enabledDistance = Settings::containerSettings.enabledDistance;
		if (CheckFormIdArray(referenceData.formId, Settings::containerSettings.whitelisted, Settings::containerSettings.whitelist, 32))
			*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
		else if (!Settings::containerSettings.enabled)
			*entityFlag |= CUSTOM_ENTRY_INVALID;
		break;
	case (static_cast<byte>(FormTypes::TesObjectMisc)):
		*entityFlag |= CUSTOM_ENTRY_VALID_ITEM;
		*entityNamePtr = referenceData.namePtr0098;
		if (CheckReferenceJunk(referenceData))
		{
			*entityFlag |= CUSTOM_ENTRY_JUNK;
			*enabledDistance = Settings::junkSettings.enabledDistance;
			if (checkScrap)
			{
				if (CheckComponentArray(referenceData))
					*entityFlag |= CUSTOM_ENTRY_VALID_SCRAP;
				else
					*entityFlag |= CUSTOM_ENTRY_INVALID;
			}
			else
			{
				if (CheckFormIdArray(referenceData.formId, Settings::junkSettings.whitelisted, Settings::junkSettings.whitelist, 32))
					*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
				else if (!Settings::junkSettings.enabled)
					*entityFlag |= CUSTOM_ENTRY_INVALID;
			}
		}
		else if (CheckReferenceMod(referenceData))
		{
			*entityFlag |= CUSTOM_ENTRY_MOD;
			*entityFlag |= CUSTOM_ENTRY_ITEM;
			*enabledDistance = Settings::itemSettings.enabledDistance;
			if (CheckFormIdArray(referenceData.formId, Settings::itemSettings.whitelisted, Settings::itemSettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::itemSettings.enabled)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		else if (CheckReferenceKeywordMisc(referenceData, 0x00135E6C))
		{
			*entityFlag |= CUSTOM_ENTRY_BOBBLEHEAD;
			*enabledDistance = Settings::bobbleheadSettings.enabledDistance;
			if (CheckFormIdArray(referenceData.formId, Settings::bobbleheadSettings.whitelisted, Settings::bobbleheadSettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::bobbleheadSettings.enabled)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		else
		{
			*entityFlag |= CUSTOM_ENTRY_MISC;
			*entityFlag |= CUSTOM_ENTRY_ITEM;
			*enabledDistance = Settings::itemSettings.enabledDistance;
			if (CheckFormIdArray(referenceData.formId, Settings::itemSettings.whitelisted, Settings::itemSettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::itemSettings.enabled)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		break;
	case (static_cast<byte>(FormTypes::TesObjectBook)):
		*entityFlag |= CUSTOM_ENTRY_VALID_ITEM;
		*entityNamePtr = referenceData.namePtr0098;
		if (CheckReferencePlan(referenceData))
		{
			*entityFlag |= CUSTOM_ENTRY_PLAN;
			*enabledDistance = Settings::planSettings.enabledDistance;

			if (IsRecipeKnown(referenceData.formId))
				*entityFlag |= CUSTOM_ENTRY_KNOWN_RECIPE;
			else
				*entityFlag |= CUSTOM_ENTRY_UNKNOWN_RECIPE;

			if (CheckFormIdArray(referenceData.formId, Settings::planSettings.whitelisted, Settings::planSettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::planSettings.enabled)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		else if (CheckReferenceKeywordBook(referenceData, 0x001D4A70))
		{
			*entityFlag |= CUSTOM_ENTRY_MAGAZINE;
			*enabledDistance = Settings::magazineSettings.enabledDistance;
			if (CheckFormIdArray(referenceData.formId, Settings::magazineSettings.whitelisted, Settings::magazineSettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::magazineSettings.enabled)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		else
		{
			*entityFlag |= CUSTOM_ENTRY_ITEM;
			*enabledDistance = Settings::itemSettings.enabledDistance;
			if (IsTreasureMap(referenceData.formId))
				*entityFlag |= CUSTOM_ENTRY_TREASURE_MAP;
			if (CheckFormIdArray(referenceData.formId, Settings::itemSettings.whitelisted, Settings::itemSettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::itemSettings.enabled)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		break;
	case (static_cast<byte>(FormTypes::TesFlora)):
		*entityFlag |= CUSTOM_ENTRY_FLORA;
		*entityNamePtr = referenceData.namePtr0098;
		*enabledDistance = Settings::floraSettings.enabledDistance;
		if (checkIngredient)
		{
			if (FloraValid(referenceData))
				*entityFlag |= CUSTOM_ENTRY_VALID_INGREDIENT;
			else
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		else
		{
			if (CheckWhitelistedFlux(referenceData) || CheckFormIdArray(referenceData.formId, Settings::floraSettings.whitelisted, Settings::floraSettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::floraSettings.enabled)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		break;
	case (static_cast<byte>(FormTypes::TesObjectWeap)):
		*entityFlag |= CUSTOM_ENTRY_WEAPON;
		*entityFlag |= CUSTOM_ENTRY_ITEM;
		*entityFlag |= CUSTOM_ENTRY_VALID_ITEM;
		*entityNamePtr = referenceData.namePtr0098;
		*enabledDistance = Settings::itemSettings.enabledDistance;
		if (CheckFormIdArray(referenceData.formId, Settings::itemSettings.whitelisted, Settings::itemSettings.whitelist, 32))
			*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
		else if (!Settings::itemSettings.enabled)
			*entityFlag |= CUSTOM_ENTRY_INVALID;
		break;
	case (static_cast<byte>(FormTypes::TesObjectArmo)):
		*entityFlag |= CUSTOM_ENTRY_ARMOR;
		*entityFlag |= CUSTOM_ENTRY_ITEM;
		*entityFlag |= CUSTOM_ENTRY_VALID_ITEM;
		*entityNamePtr = referenceData.namePtr0098;
		*enabledDistance = Settings::itemSettings.enabledDistance;
		if (CheckFormIdArray(referenceData.formId, Settings::itemSettings.whitelisted, Settings::itemSettings.whitelist, 32))
			*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
		else if (!Settings::itemSettings.enabled)
			*entityFlag |= CUSTOM_ENTRY_INVALID;
		break;
	case (static_cast<byte>(FormTypes::TesAmmo)):
		*entityFlag |= CUSTOM_ENTRY_AMMO;
		*entityFlag |= CUSTOM_ENTRY_ITEM;
		*entityFlag |= CUSTOM_ENTRY_VALID_ITEM;
		*entityNamePtr = referenceData.namePtr0098;
		*enabledDistance = Settings::itemSettings.enabledDistance;
		if (CheckFormIdArray(referenceData.formId, Settings::itemSettings.whitelisted, Settings::itemSettings.whitelist, 32))
			*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
		else if (!Settings::itemSettings.enabled)
			*entityFlag |= CUSTOM_ENTRY_INVALID;
		break;
	case (static_cast<byte>(FormTypes::AlchemyItem)):
		*entityFlag |= CUSTOM_ENTRY_AID;
		*entityFlag |= CUSTOM_ENTRY_ITEM;
		*entityFlag |= CUSTOM_ENTRY_VALID_ITEM;
		*entityNamePtr = referenceData.namePtr0098;
		*enabledDistance = Settings::itemSettings.enabledDistance;
		if (CheckFormIdArray(referenceData.formId, Settings::itemSettings.whitelisted, Settings::itemSettings.whitelist, 32))
			*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
		else if (!Settings::itemSettings.enabled)
			*entityFlag |= CUSTOM_ENTRY_INVALID;
		break;
	default:
		if (CheckReferenceItem(referenceData))
		{
			*entityFlag |= CUSTOM_ENTRY_ITEM;
			*entityFlag |= CUSTOM_ENTRY_VALID_ITEM;
			*entityNamePtr = referenceData.namePtr0098;
			*enabledDistance = Settings::itemSettings.enabledDistance;
			if (CheckFormIdArray(referenceData.formId, Settings::itemSettings.whitelisted, Settings::itemSettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::itemSettings.enabled)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		else
		{
			*entityFlag |= CUSTOM_ENTRY_ENTITY;
			*entityNamePtr = 0;
			*enabledDistance = Settings::entitySettings.enabledDistance;
			if (CheckFormIdArray(referenceData.formId, Settings::entitySettings.whitelisted, Settings::entitySettings.whitelist, 32))
				*entityFlag |= CUSTOM_ENTRY_WHITELISTED;
			else if (!Settings::entitySettings.enabled || !Settings::entitySettings.drawUnnamed)
				*entityFlag |= CUSTOM_ENTRY_INVALID;
		}
		break;
	}
}

bool ErectusMemory::GetActorSnapshotComponentData(const TesObjectRefr& entityData, ActorSnapshotComponent* actorSnapshotComponentData)
{
	if (!Utils::Valid(entityData.actorCorePtr))
		return false;

	DWORD64 actorCoreBufferA;
	if (!Rpm(entityData.actorCorePtr + 0x70, &actorCoreBufferA, sizeof actorCoreBufferA))
		return false;
	if (!Utils::Valid(actorCoreBufferA))
		return false;

	DWORD64 actorCoreBufferB;
	if (!Rpm(actorCoreBufferA + 0x8, &actorCoreBufferB, sizeof actorCoreBufferB))
		return false;
	if (!Utils::Valid(actorCoreBufferB))
		return false;

	return Rpm(actorCoreBufferB, &*actorSnapshotComponentData, sizeof(ActorSnapshotComponent));
}

std::string ErectusMemory::GetEntityName(const DWORD64 ptr)
{
	std::string result{};

	if (!Utils::Valid(ptr))
		return result;

	auto nameLength = 0;
	auto namePtr = ptr;

	if (!Rpm(namePtr + 0x10, &nameLength, sizeof nameLength))
		return result;
	if (nameLength <= 0 || nameLength > 0x7FFF)
	{
		DWORD64 buffer;
		if (!Rpm(namePtr + 0x10, &buffer, sizeof buffer))
			return result;
		if (!Utils::Valid(buffer))
			return result;
		if (!Rpm(buffer + 0x10, &nameLength, sizeof nameLength))
			return result;
		if (nameLength <= 0 || nameLength > 0x7FFF)
			return result;
		namePtr = buffer;
	}

	const auto nameSize = nameLength + 1;
	auto name = std::make_unique<char[]>(nameSize);

	if (!Rpm(namePtr + 0x18, name.get(), nameSize))
		return result;

	result = Utils::UTF8ToGBK(name.get());
	return result;
}

bool ErectusMemory::UpdateBufferEntityList()
{
	std::vector<CustomEntry> entities{};

	auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return false;

	auto bufferList = GetEntityPtrList();
	if (bufferList.empty())
		return false;

	for (auto entityPtr : bufferList)
	{
		if (!Utils::Valid(entityPtr))
			continue;
		if (entityPtr == localPlayerPtr)
			continue;

		TesObjectRefr entityData{};
		if (!Rpm(entityPtr, &entityData, sizeof entityData))
			continue;
		if (!Utils::Valid(entityData.referencedItemPtr))
			continue;

		if (entityData.formType == static_cast<BYTE>(FormTypes::PlayerCharacter))
			continue;

		TesItem referenceData{};
		if (!Rpm(entityData.referencedItemPtr, &referenceData, sizeof referenceData))
			continue;

		auto entityFlag = CUSTOM_ENTRY_DEFAULT;
		DWORD64 entityNamePtr = 0;
		auto enabledDistance = 0;

		GetCustomEntityData(referenceData, &entityFlag, &entityNamePtr, &enabledDistance, Settings::scrapLooter.scrapOverrideEnabled, Settings::harvester.overrideEnabled);
		if (entityFlag & CUSTOM_ENTRY_INVALID)
			continue;

		auto distance = Utils::GetDistance(entityData.position, localPlayer.position);
		auto normalDistance = static_cast<int>(distance * 0.01f);
		if (normalDistance > enabledDistance)
			continue;

		auto entityName = GetEntityName(entityNamePtr);
		if (entityName.empty())
		{
			entityFlag |= CUSTOM_ENTRY_UNNAMED;
			entityName = fmt::format("{0:X} [{1:X}]", entityData.formId, referenceData.formType);
		}

		CustomEntry entry{
			.entityPtr = entityPtr,
			.referencePtr = entityData.referencedItemPtr,
			.entityFormId = entityData.formId,
			.referenceFormId = referenceData.formId,
			.flag = entityFlag,
			.name = entityName
		};

		entities.push_back(entry);
	}
	entityDataBuffer = entities;

	return entityDataBuffer.empty() ? false : true;
}

std::string ErectusMemory::GetPlayerName(const ClientAccount& clientAccountData)
{
	std::string result = {};
	if (!clientAccountData.nameLength)
		return result;

	const auto playerNameSize = clientAccountData.nameLength + 1;
	auto* playerName = new char[playerNameSize];

	if (clientAccountData.nameLength > 15)
	{
		DWORD64 buffer;
		memcpy(&buffer, clientAccountData.nameData, sizeof buffer);
		if (!Utils::Valid(buffer))
		{
			delete[]playerName;
			playerName = nullptr;
			return result;
		}

		if (!Rpm(buffer, &*playerName, playerNameSize))
		{
			delete[]playerName;
			playerName = nullptr;
			return result;
		}
	}
	else
		memcpy(playerName, clientAccountData.nameData, playerNameSize);

	if (Utils::GetTextLength(playerName) != clientAccountData.nameLength)
	{
		delete[]playerName;
		playerName = nullptr;
		return result;
	}
	result.assign(playerName);

	delete[]playerName;
	playerName = nullptr;

	return result;
}

bool ErectusMemory::UpdateBufferPlayerList()
{
	std::vector<CustomEntry> players{};

	auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	DWORD64 falloutMainDataPtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_MAIN, &falloutMainDataPtr, sizeof falloutMainDataPtr))
		return false;
	if (!Utils::Valid(falloutMainDataPtr))
		return false;

	FalloutMain falloutMainData{};
	if (!Rpm(falloutMainDataPtr, &falloutMainData, sizeof falloutMainData))
		return false;
	if (!Utils::Valid(falloutMainData.platformSessionManagerPtr))
		return false;

	PlatformSessionManager platformSessionManagerData{};
	if (!Rpm(falloutMainData.platformSessionManagerPtr, &platformSessionManagerData, sizeof platformSessionManagerData))
		return false;
	if (!Utils::Valid(platformSessionManagerData.clientAccountManagerPtr))
		return false;

	ClientAccountManager clientAccountManagerData{};
	if (!Rpm(platformSessionManagerData.clientAccountManagerPtr, &clientAccountManagerData, sizeof clientAccountManagerData))
		return false;
	if (!Utils::Valid(clientAccountManagerData.clientAccountArrayPtr))
		return false;

	auto clientAccountArraySize = 0;
	clientAccountArraySize += clientAccountManagerData.clientAccountArraySizeA;
	clientAccountArraySize += clientAccountManagerData.clientAccountArraySizeB;
	if (!clientAccountArraySize)
		return false;

	auto* clientAccountArray = new DWORD64[clientAccountArraySize];
	if (!Rpm(clientAccountManagerData.clientAccountArrayPtr, &*clientAccountArray, clientAccountArraySize * sizeof(DWORD64)))
	{
		delete[]clientAccountArray;
		clientAccountArray = nullptr;
		return false;
	}

	for (auto i = 0; i < clientAccountArraySize; i++)
	{
		if (!Utils::Valid(clientAccountArray[i]))
			continue;

		ClientAccountBuffer clientAccountBufferData{};
		if (!Rpm(clientAccountArray[i], &clientAccountBufferData, sizeof clientAccountBufferData))
			continue;
		if (!Utils::Valid(clientAccountBufferData.clientAccountPtr))
			continue;

		ClientAccount clientAccountData{};
		if (!Rpm(clientAccountBufferData.clientAccountPtr, &clientAccountData, sizeof clientAccountData))
			continue;
		if (clientAccountData.formId == 0xFFFFFFFF)
			continue;

		auto entityPtr = GetPtr(clientAccountData.formId);
		if (!Utils::Valid(entityPtr))
			continue;
		if (entityPtr == localPlayerPtr)
			continue;

		TesObjectRefr entityData{};
		if (!Rpm(entityPtr, &entityData, sizeof entityData))
			continue;
		if (!Utils::Valid(entityData.referencedItemPtr))
			continue;

		TesItem referenceData{};
		if (!Rpm(entityData.referencedItemPtr, &referenceData, sizeof referenceData))
			continue;
		if (referenceData.formId != 0x00000007)
			continue;

		auto entityFlag = CUSTOM_ENTRY_PLAYER;
		auto entityName = GetPlayerName(clientAccountData);
		if (entityName.empty())
		{
			entityFlag |= CUSTOM_ENTRY_UNNAMED;
			entityName = fmt::format("{:x}", entityData.formId);
		}

		CustomEntry entry{
			.entityPtr = entityPtr,
			.referencePtr = entityData.referencedItemPtr,
			.entityFormId = entityData.formId,
			.referenceFormId = referenceData.formId,
			.flag = entityFlag,
			.name = GetPlayerName(clientAccountData),
		};
		players.push_back(entry);
	}

	delete[]clientAccountArray;
	clientAccountArray = nullptr;

	playerDataBuffer = players;

	return playerDataBuffer.empty() ? false : true;
}

bool ErectusMemory::TargetValid(const TesObjectRefr& entityData)
{
	if (entityData.formType != static_cast<BYTE>(FormTypes::PlayerCharacter) && entityData.formType != static_cast<BYTE>(FormTypes::TesActor))
		return false;

	if (entityData.spawnFlag != 0x02 && !Settings::targetting.ignoreRenderDistance)
		return false;

	ActorSnapshotComponent actorSnapshotComponentData{};
	if (GetActorSnapshotComponentData(entityData, &actorSnapshotComponentData))
	{
		if (Settings::targetting.ignoreEssentialNpCs && actorSnapshotComponentData.isEssential)
			return false;
	}

	switch (CheckHealthFlag(entityData.healthFlag))
	{
	case 0x01: //Alive
		return Settings::targetting.targetLiving;
	case 0x02: //Downed
		return Settings::targetting.targetDowned;
	case 0x03: //Dead
		return Settings::targetting.targetDead;
	default: //Unknown
		return Settings::targetting.targetUnknown;
	}
}

bool ErectusMemory::IsFloraHarvested(const BYTE harvestFlagA, const BYTE harvestFlagB)
{
	return harvestFlagA >> 5 & 1 || harvestFlagB >> 5 & 1;
}

bool ErectusMemory::MessagePatcher(const bool state)
{
	BYTE fakeMessagesCheck[2];
	if (!Rpm(ErectusProcess::exe + OFFSET_FAKE_MESSAGE, &fakeMessagesCheck, sizeof fakeMessagesCheck))
		return false;

	BYTE fakeMessagesEnabled[] = { 0xB2, 0x00 };
	BYTE fakeMessagesDisabled[] = { 0xB2, 0xFF };

	if (!memcmp(fakeMessagesCheck, fakeMessagesEnabled, sizeof fakeMessagesEnabled))
	{
		if (state)
			return true;

		return Wpm(ErectusProcess::exe + OFFSET_FAKE_MESSAGE, &fakeMessagesDisabled, sizeof fakeMessagesDisabled);
	}

	if (!memcmp(fakeMessagesCheck, fakeMessagesDisabled, sizeof fakeMessagesDisabled))
	{
		if (state)
			return Wpm(ErectusProcess::exe + OFFSET_FAKE_MESSAGE, &fakeMessagesEnabled, sizeof fakeMessagesEnabled);
		return true;
	}

	return false;
}

bool ErectusMemory::SendMessageToServer(void* message, const size_t size)
{
	if (!MessagePatcher(allowMessages))
		return false;

	if (!allowMessages)
		return false;

	const auto allocSize = size + sizeof(ExternalFunction);
	const auto allocAddress = AllocEx(allocSize);
	if (allocAddress == 0)
		return false;

	ExternalFunction externalFunctionData;
	externalFunctionData.address = ErectusProcess::exe + OFFSET_MESSAGE_SENDER;
	externalFunctionData.rcx = allocAddress + sizeof(ExternalFunction);
	externalFunctionData.rdx = 0;
	externalFunctionData.r8 = 0;
	externalFunctionData.r9 = 0;

	auto* pageData = new BYTE[allocSize];
	memset(pageData, 0x00, allocSize);
	memcpy(pageData, &externalFunctionData, sizeof externalFunctionData);
	memcpy(&pageData[sizeof(ExternalFunction)], message, size);
	const auto written = Wpm(allocAddress, &*pageData, allocSize);

	delete[]pageData;
	pageData = nullptr;

	if (!written)
	{
		FreeEx(allocAddress);
		return false;
	}

	const auto paramAddress = allocAddress + sizeof ExternalFunction::ASM;
	auto* const thread = CreateRemoteThread(ErectusProcess::handle, nullptr, 0, LPTHREAD_START_ROUTINE(allocAddress),
		LPVOID(paramAddress), 0, nullptr);

	if (thread == nullptr)
	{
		FreeEx(allocAddress);
		return false;
	}

	const auto threadResult = WaitForSingleObject(thread, 3000);
	CloseHandle(thread);

	if (threadResult == WAIT_TIMEOUT)
		return false;

	FreeEx(allocAddress);
	return true;
}

bool ErectusMemory::LootScrap()
{
	if (!MessagePatcher(allowMessages))
		return false;

	if (!allowMessages)
		return false;

	if (!CheckScrapList())
		return false;

	auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
	{
		Settings::scrapLooter.autoLootingEnabled = false;
		return false;
	}

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return false;

	auto bufferList = GetEntityPtrList();
	if (bufferList.empty()) return false;

	for (auto item : bufferList)
	{
		if (!Utils::Valid(item))
			continue;
		if (item == localPlayerPtr)
			continue;

		TesObjectRefr entityData{};
		if (!Rpm(item, &entityData, sizeof entityData))
			continue;
		if (!Utils::Valid(entityData.referencedItemPtr))
			continue;

		if (entityData.spawnFlag != 0x02)
			continue;

		TesItem referenceData{};
		if (!Rpm(entityData.referencedItemPtr, &referenceData, sizeof referenceData))
			continue;
		if (referenceData.formId == 0x00000007)
			continue;

		auto entityFlag = CUSTOM_ENTRY_DEFAULT;
		DWORD64 entityNamePtr = 0;
		auto enabledDistance = 0;

		GetCustomEntityData(referenceData, &entityFlag, &entityNamePtr, &enabledDistance, true, false);
		if (!(entityFlag & CUSTOM_ENTRY_VALID_SCRAP))
			continue;

		auto distance = Utils::GetDistance(entityData.position, localPlayer.position);
		auto normalDistance = static_cast<int>(distance * 0.01f);

		if (normalDistance > Settings::scrapLooter.maxDistance)
			continue;

		RequestActivateRefMessage requestActivateRefMessageData{};
		requestActivateRefMessageData.vtable = ErectusProcess::exe + VTABLE_REQUESTACTIVATEREFMSG;
		requestActivateRefMessageData.formId = entityData.formId;
		requestActivateRefMessageData.choice = 0xFF;
		requestActivateRefMessageData.forceActivate = 0;
		SendMessageToServer(&requestActivateRefMessageData, sizeof requestActivateRefMessageData);
	}

	return true;
}

bool ErectusMemory::CheckItemLooterSettings()
{
	if (Settings::itemLooter.lootWeaponsEnabled && Settings::itemLooter.lootWeaponsDistance > 0)
		return true;
	if (Settings::itemLooter.lootArmorEnabled && Settings::itemLooter.lootArmorDistance > 0)
		return true;
	if (Settings::itemLooter.lootAmmoEnabled && Settings::itemLooter.lootAmmoDistance > 0)
		return true;
	if (Settings::itemLooter.lootModsEnabled && Settings::itemLooter.lootModsDistance > 0)
		return true;
	if (Settings::itemLooter.lootMagazinesEnabled && Settings::itemLooter.lootMagazinesDistance > 0)
		return true;
	if (Settings::itemLooter.lootBobbleheadsEnabled && Settings::itemLooter.lootBobbleheadsDistance > 0)
		return true;
	if (Settings::itemLooter.lootAidEnabled && Settings::itemLooter.lootAidDistance > 0)
		return true;
	if (Settings::itemLooter.lootKnownPlansEnabled && Settings::itemLooter.lootKnownPlansDistance > 0)
		return true;
	if (Settings::itemLooter.lootUnknownPlansEnabled && Settings::itemLooter.lootUnknownPlansDistance > 0)
		return true;
	if (Settings::itemLooter.lootMiscEnabled && Settings::itemLooter.lootMiscDistance > 0)
		return true;
	if (Settings::itemLooter.lootUnlistedEnabled && Settings::itemLooter.lootUnlistedDistance > 0)
		return true;
	if (Settings::itemLooter.lootListEnabled && Settings::itemLooter.lootListDistance > 0)
		return CheckItemLooterList();

	return false;
}

bool ErectusMemory::CheckOnlyUseItemLooterList()
{
	if (Settings::itemLooter.lootWeaponsEnabled && Settings::itemLooter.lootWeaponsDistance > 0)
		return false;
	if (Settings::itemLooter.lootArmorEnabled && Settings::itemLooter.lootArmorDistance > 0)
		return false;
	if (Settings::itemLooter.lootAmmoEnabled && Settings::itemLooter.lootAmmoDistance > 0)
		return false;
	if (Settings::itemLooter.lootModsEnabled && Settings::itemLooter.lootModsDistance > 0)
		return false;
	if (Settings::itemLooter.lootMagazinesEnabled && Settings::itemLooter.lootMagazinesDistance > 0)
		return false;
	if (Settings::itemLooter.lootBobbleheadsEnabled && Settings::itemLooter.lootBobbleheadsDistance > 0)
		return false;
	if (Settings::itemLooter.lootAidEnabled && Settings::itemLooter.lootAidDistance > 0)
		return false;
	if (Settings::itemLooter.lootKnownPlansEnabled && Settings::itemLooter.lootKnownPlansDistance > 0)
		return false;
	if (Settings::itemLooter.lootUnknownPlansEnabled && Settings::itemLooter.lootUnknownPlansDistance > 0)
		return false;
	if (Settings::itemLooter.lootMiscEnabled && Settings::itemLooter.lootMiscDistance > 0)
		return false;
	if (Settings::itemLooter.lootUnlistedEnabled && Settings::itemLooter.lootUnlistedDistance > 0)
		return false;
	if (Settings::itemLooter.lootListEnabled && Settings::itemLooter.lootListDistance > 0)
		return CheckItemLooterList();

	return false;
}

bool ErectusMemory::CheckEnabledItem(const DWORD formId, const DWORD64 entityFlag, const int normalDistance)
{
	if (Settings::itemLooter.lootListEnabled)
	{
		if (CheckFormIdArray(formId, Settings::itemLooter.enabledList, Settings::itemLooter.formIdList, 100))
		{
			if (normalDistance <= Settings::itemLooter.lootListDistance)
				return true;
		}
	}

	if (entityFlag & CUSTOM_ENTRY_WEAPON && normalDistance <= Settings::itemLooter.lootWeaponsDistance)
		return Settings::itemLooter.lootWeaponsEnabled;
	if (entityFlag & CUSTOM_ENTRY_ARMOR && normalDistance <= Settings::itemLooter.lootArmorDistance)
		return Settings::itemLooter.lootArmorEnabled;
	if (entityFlag & CUSTOM_ENTRY_AMMO && normalDistance <= Settings::itemLooter.lootAmmoDistance)
		return Settings::itemLooter.lootAmmoEnabled;
	if (entityFlag & CUSTOM_ENTRY_MOD && normalDistance <= Settings::itemLooter.lootModsDistance)
		return Settings::itemLooter.lootModsEnabled;
	if (entityFlag & CUSTOM_ENTRY_MAGAZINE && normalDistance <= Settings::itemLooter.lootMagazinesDistance)
		return Settings::itemLooter.lootMagazinesEnabled;
	if (entityFlag & CUSTOM_ENTRY_BOBBLEHEAD && normalDistance <= Settings::itemLooter.lootBobbleheadsDistance)
		return Settings::itemLooter.lootBobbleheadsEnabled;
	if (entityFlag & CUSTOM_ENTRY_AID && normalDistance <= Settings::itemLooter.lootAidDistance)
		return Settings::itemLooter.lootAidEnabled;
	if (entityFlag & CUSTOM_ENTRY_KNOWN_RECIPE && normalDistance <= Settings::itemLooter.lootKnownPlansDistance)
		return Settings::itemLooter.lootKnownPlansEnabled;
	if (entityFlag & CUSTOM_ENTRY_UNKNOWN_RECIPE && normalDistance <= Settings::itemLooter.lootUnknownPlansDistance)
		return Settings::itemLooter.lootUnknownPlansEnabled;
	if (entityFlag & CUSTOM_ENTRY_MISC && normalDistance <= Settings::itemLooter.lootMiscDistance)
		return Settings::itemLooter.lootMiscEnabled;
	if (Settings::itemLooter.lootUnlistedEnabled && normalDistance <= Settings::itemLooter.lootUnlistedDistance)
		return true;

	return false;
}

bool ErectusMemory::LootItems()
{
	if (!MessagePatcher(allowMessages))
		return false;

	if (!allowMessages)
		return false;

	if (!CheckItemLooterSettings())
		return false;

	auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
	{
		Settings::itemLooter.autoLootingEnabled = false;
		return false;
	}

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return false;


	auto bufferList = GetEntityPtrList();
	if (bufferList.empty())
		return false;

	auto onlyUseItemLooterList = CheckOnlyUseItemLooterList();
	auto useItemLooterBlacklist = CheckItemLooterBlacklist();

	for (auto item : bufferList)
	{
		if (!Utils::Valid(item))
			continue;
		if (item == localPlayerPtr)
			continue;

		TesObjectRefr entityData{};
		if (!Rpm(item, &entityData, sizeof entityData))
			continue;
		if (!Utils::Valid(entityData.referencedItemPtr))
			continue;

		if (entityData.spawnFlag != 0x02)
			continue;

		TesItem referenceData{};
		if (!Rpm(entityData.referencedItemPtr, &referenceData, sizeof referenceData))
			continue;

		if (useItemLooterBlacklist)
		{
			if (CheckFormIdArray(referenceData.formId, Settings::itemLooter.blacklistEnabled, Settings::itemLooter.blacklist, 64))
				continue;
		}

		if (onlyUseItemLooterList)
		{
			if (!CheckFormIdArray(referenceData.formId, Settings::itemLooter.enabledList, Settings::itemLooter.formIdList, 100))
				continue;
		}

		auto entityFlag = CUSTOM_ENTRY_DEFAULT;
		DWORD64 entityNamePtr = 0;
		auto enabledDistance = 0;

		GetCustomEntityData(referenceData, &entityFlag, &entityNamePtr, &enabledDistance, false, false);
		if (!(entityFlag & CUSTOM_ENTRY_VALID_ITEM))
			continue;
		if (entityFlag & CUSTOM_ENTRY_JUNK)
			continue;

		auto distance = Utils::GetDistance(entityData.position, localPlayer.position);
		auto normalDistance = static_cast<int>(distance * 0.01f);

		if (!onlyUseItemLooterList)
		{
			if (!CheckEnabledItem(referenceData.formId, entityFlag, normalDistance))
				continue;
		}
		else
		{
			if (normalDistance > Settings::itemLooter.lootListDistance)
				continue;
		}

		RequestActivateRefMessage requestActivateRefMessageData{};
		requestActivateRefMessageData.vtable = ErectusProcess::exe + VTABLE_REQUESTACTIVATEREFMSG;
		requestActivateRefMessageData.formId = entityData.formId;
		requestActivateRefMessageData.choice = 0xFF;
		requestActivateRefMessageData.forceActivate = 0;
		SendMessageToServer(&requestActivateRefMessageData, sizeof requestActivateRefMessageData);
	}

	return true;
}

void ErectusMemory::DeleteOldWeaponList()
{
	if (oldWeaponList != nullptr)
	{
		if (oldWeaponListSize)
		{
			for (auto i = 0; i < oldWeaponListSize; i++)
			{
				if (oldWeaponList[i].weaponData != nullptr)
				{
					delete[]oldWeaponList[i].weaponData;
					oldWeaponList[i].weaponData = nullptr;
				}

				if (oldWeaponList[i].aimModelData != nullptr)
				{
					delete[]oldWeaponList[i].aimModelData;
					oldWeaponList[i].aimModelData = nullptr;
				}
			}
		}

		delete[]oldWeaponList;
		oldWeaponList = nullptr;
	}

	oldWeaponListSize = 0;
	oldWeaponListUpdated = false;
}

bool ErectusMemory::UpdateOldWeaponData()
{
	DWORD64 dataHandlerPtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_DATA_HANDLER, &dataHandlerPtr, sizeof dataHandlerPtr))
		return false;
	if (!Utils::Valid(dataHandlerPtr))
		return false;

	ReferenceList weaponList{};
	if (!Rpm(dataHandlerPtr + 0x598, &weaponList, sizeof weaponList))
		return false;
	if (!Utils::Valid(weaponList.arrayPtr) || !weaponList.arraySize || weaponList.arraySize > 0x7FFF)
		return false;

	auto* weaponPtrArray = new DWORD64[weaponList.arraySize];
	if (!Rpm(weaponList.arrayPtr, &*weaponPtrArray, weaponList.arraySize * sizeof(DWORD64)))
	{
		delete[]weaponPtrArray;
		weaponPtrArray = nullptr;
		return false;
	}

	oldWeaponList = new OldWeapon[weaponList.arraySize];
	oldWeaponListSize = weaponList.arraySize;

	for (auto i = 0; i < weaponList.arraySize; i++)
	{
		oldWeaponList[i].weaponData = nullptr;
		oldWeaponList[i].aimModelData = nullptr;

		if (!Utils::Valid(weaponPtrArray[i]))
			continue;

		Weapon weaponData{};
		if (!Rpm(weaponPtrArray[i], &weaponData, sizeof weaponData))
			continue;

		oldWeaponList[i].weaponData = new Weapon;
		memcpy(&*oldWeaponList[i].weaponData, &weaponData, sizeof weaponData);

		if (!Utils::Valid(weaponData.aimModelPtr))
			continue;

		AimModel aimModelData{};
		if (!Rpm(weaponData.aimModelPtr, &aimModelData, sizeof aimModelData))
			continue;

		oldWeaponList[i].aimModelData = new AimModel;
		memcpy(&*oldWeaponList[i].aimModelData, &aimModelData, sizeof aimModelData);
	}

	delete[]weaponPtrArray;
	weaponPtrArray = nullptr;
	return true;
}

int ErectusMemory::GetOldWeaponIndex(const DWORD formId)
{
	for (auto i = 0; i < oldWeaponListSize; i++)
	{
		if (oldWeaponList[i].weaponData != nullptr && oldWeaponList[i].weaponData->formId == formId)
			return i;
	}

	return -1;
}

bool ErectusMemory::WeaponEditingEnabled()
{
	auto bufferSettings = Settings::weapons;
	bufferSettings.capacity = Settings::defaultWeaponSettings.capacity;
	bufferSettings.speed = Settings::defaultWeaponSettings.speed;
	bufferSettings.reach = Settings::defaultWeaponSettings.reach;
	if (!memcmp(&bufferSettings, &Settings::defaultWeaponSettings, sizeof(WeaponSettings)))
		return false;

	return true;
}

bool ErectusMemory::EditWeapon(const int index, const bool revertWeaponData)
{
	DWORD64 dataHandlerPtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_DATA_HANDLER, &dataHandlerPtr, sizeof dataHandlerPtr))
		return false;
	if (!Utils::Valid(dataHandlerPtr))
		return false;

	ReferenceList weaponList{};
	if (!Rpm(dataHandlerPtr + 0x598, &weaponList, sizeof weaponList))
		return false;
	if (!Utils::Valid(weaponList.arrayPtr) || !weaponList.arraySize || weaponList.arraySize > 0x7FFF)
		return false;

	DWORD64 weaponPtr;
	if (!Rpm(weaponList.arrayPtr + index * sizeof(DWORD64), &weaponPtr, sizeof weaponPtr))
		return false;
	if (!Utils::Valid(weaponPtr)) return false;

	Weapon weaponData{};
	if (!Rpm(weaponPtr, &weaponData, sizeof weaponData))
		return false;
	if (oldWeaponList[index].weaponData == nullptr)
		return false;

	auto currentWeaponIndex = index;
	if (oldWeaponList[currentWeaponIndex].weaponData->formId != weaponData.formId)
	{
		const auto bufferIndex = GetOldWeaponIndex(weaponData.formId);
		if (bufferIndex == -1)
			return false;
		currentWeaponIndex = bufferIndex;
	}

	auto writeWeaponData = false;
	auto writeAimModelData = false;

	if (!revertWeaponData && Settings::weapons.instantReload)
	{
		if (weaponData.reloadSpeed != 100.0f)
		{
			weaponData.reloadSpeed = 100.0f;
			writeWeaponData = true;
		}
	}
	else
	{
		if (weaponData.reloadSpeed != oldWeaponList[currentWeaponIndex].weaponData->reloadSpeed)
		{
			weaponData.reloadSpeed = oldWeaponList[currentWeaponIndex].weaponData->reloadSpeed;
			writeWeaponData = true;
		}
	}

	if (!revertWeaponData && Settings::weapons.automaticflag)
	{
		if (!(weaponData.flagB >> 7 & 1))
		{
			weaponData.flagB |= 1 << 7;
			writeWeaponData = true;
		}
	}
	else
	{
		if (weaponData.flagB != oldWeaponList[currentWeaponIndex].weaponData->flagB)
		{
			weaponData.flagB = oldWeaponList[currentWeaponIndex].weaponData->flagB;
			writeWeaponData = true;
		}
	}

	if (!revertWeaponData && Settings::weapons.capacityEnabled)
	{
		if (weaponData.capacity != static_cast<short>(Settings::weapons.capacity))
		{
			weaponData.capacity = static_cast<short>(Settings::weapons.capacity);
			writeWeaponData = true;
		}
	}
	else
	{
		if (weaponData.capacity != oldWeaponList[currentWeaponIndex].weaponData->capacity)
		{
			weaponData.capacity = oldWeaponList[currentWeaponIndex].weaponData->capacity;
			writeWeaponData = true;
		}
	}

	if (!revertWeaponData && Settings::weapons.speedEnabled)
	{
		if (weaponData.speed != Settings::weapons.speed)
		{
			weaponData.speed = Settings::weapons.speed;
			writeWeaponData = true;
		}
	}
	else
	{
		if (weaponData.speed != oldWeaponList[currentWeaponIndex].weaponData->speed)
		{
			weaponData.speed = oldWeaponList[currentWeaponIndex].weaponData->speed;
			writeWeaponData = true;
		}
	}

	if (!revertWeaponData && Settings::weapons.reachEnabled)
	{
		if (weaponData.reach != Settings::weapons.reach)
		{
			weaponData.reach = Settings::weapons.reach;
			writeWeaponData = true;
		}
	}
	else
	{
		if (weaponData.reach != oldWeaponList[currentWeaponIndex].weaponData->reach)
		{
			weaponData.reach = oldWeaponList[currentWeaponIndex].weaponData->reach;
			writeWeaponData = true;
		}
	}

	if (writeWeaponData)
		Wpm(weaponPtr, &weaponData, sizeof weaponData);

	if (!Utils::Valid(weaponData.aimModelPtr))
		return true;

	if (oldWeaponList[currentWeaponIndex].aimModelData == nullptr)
		return false;

	AimModel aimModelData{};
	if (!Rpm(weaponData.aimModelPtr, &aimModelData, sizeof aimModelData))
		return false;

	BYTE noRecoil[sizeof AimModel::recoilData];
	memset(noRecoil, 0x00, sizeof noRecoil);

	BYTE noSpread[sizeof AimModel::spreadData];
	memset(noSpread, 0x00, sizeof noSpread);

	if (!revertWeaponData && Settings::weapons.noRecoil)
	{
		if (memcmp(aimModelData.recoilData, noRecoil, sizeof AimModel::recoilData) != 0)
		{
			memcpy(aimModelData.recoilData, noRecoil, sizeof AimModel::recoilData);
			writeAimModelData = true;
		}
	}
	else
	{
		if (memcmp(aimModelData.recoilData, oldWeaponList[currentWeaponIndex].aimModelData->recoilData, sizeof AimModel::recoilData) != 0)
		{
			memcpy(aimModelData.recoilData, oldWeaponList[currentWeaponIndex].aimModelData->recoilData, sizeof AimModel::recoilData);
			writeAimModelData = true;
		}
	}

	if (!revertWeaponData && Settings::weapons.noSpread)
	{
		if (memcmp(aimModelData.spreadData, noSpread, sizeof AimModel::spreadData) != 0)
		{
			memcpy(aimModelData.spreadData, noSpread, sizeof AimModel::spreadData);
			writeAimModelData = true;
		}
	}
	else
	{
		if (memcmp(aimModelData.spreadData, oldWeaponList[currentWeaponIndex].aimModelData->spreadData, sizeof AimModel::spreadData) != 0)
		{
			memcpy(aimModelData.spreadData, oldWeaponList[currentWeaponIndex].aimModelData->spreadData, sizeof AimModel::spreadData);
			writeAimModelData = true;
		}
	}

	if (!revertWeaponData && Settings::weapons.noSway)
	{
		if (aimModelData.sway != 100.0f)
		{
			aimModelData.sway = 100.0f;
			writeAimModelData = true;
		}
	}
	else
	{
		if (aimModelData.sway != oldWeaponList[currentWeaponIndex].aimModelData->sway)
		{
			aimModelData.sway = oldWeaponList[currentWeaponIndex].aimModelData->sway;
			writeAimModelData = true;
		}
	}

	if (writeAimModelData)
		return Wpm(weaponData.aimModelPtr, &aimModelData, sizeof aimModelData);

	return true;
}

bool ErectusMemory::InfiniteAmmo(const bool state)
{
	BYTE infiniteAmmoOn[] = { 0x66, 0xB8, 0xE7, 0x03 };
	BYTE infiniteAmmoOff[] = { 0x8B, 0x44, 0x24, 0x50 };
	BYTE infiniteAmmoCheck[sizeof infiniteAmmoOff];

	if (!Rpm(ErectusProcess::exe + OFFSET_INFINITE_AMMO, &infiniteAmmoCheck, sizeof infiniteAmmoCheck))
		return false;

	if (state && !memcmp(infiniteAmmoCheck, infiniteAmmoOff, sizeof infiniteAmmoOff))
		return Wpm(ErectusProcess::exe + OFFSET_INFINITE_AMMO, &infiniteAmmoOn, sizeof infiniteAmmoOn);
	if (!state && !memcmp(infiniteAmmoCheck, infiniteAmmoOn, sizeof infiniteAmmoOn))
		return Wpm(ErectusProcess::exe + OFFSET_INFINITE_AMMO, &infiniteAmmoOff, sizeof infiniteAmmoOff);
	return false;
}

bool ErectusMemory::LockedTargetValid(bool* isPlayer)
{
	if (!Utils::Valid(targetLockingPtr))
		return false;

	TesObjectRefr entityData{};
	if (!Rpm(targetLockingPtr, &entityData, sizeof entityData))
		return false;
	if (!Utils::Valid(entityData.referencedItemPtr))
		return false;

	TesItem referenceData{};
	if (!Rpm(entityData.referencedItemPtr, &referenceData, sizeof referenceData))
		return false;
	const auto result = TargetValid(entityData);

	if (referenceData.formId == 0x00000007)
		*isPlayer = true;
	else
		*isPlayer = false;

	return result;
}

bool ErectusMemory::DamageRedirection(DWORD64* targetingPage, bool* targetingPageValid, const bool isExiting, const bool state)
{
	BYTE pageJmpOn[] = { 0x48, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE3 };
	BYTE pageJmpOff[] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };
	BYTE pageJmpCheck[sizeof pageJmpOff];

	BYTE redirectionOn[] = { 0xE9, 0x69, 0xFE, 0xFF, 0xFF };
	BYTE redirectionOff[] = { 0x48, 0x8B, 0x5C, 0x24, 0x50 };
	BYTE redirectionCheck[sizeof redirectionOff];

	if (!Rpm(ErectusProcess::exe + OFFSET_REDIRECTION_JMP, &pageJmpCheck, sizeof pageJmpCheck))
		return false;

	DWORD64 pageCheck;
	memcpy(&pageCheck, &pageJmpCheck[2], sizeof pageCheck);
	if (Utils::Valid(pageCheck) && pageCheck != *targetingPage)
	{
		BYTE pageOpcode[] = { 0x48, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00 };
		BYTE pageOpcodeCheck[sizeof pageOpcode];
		if (!Rpm(pageCheck, &pageOpcodeCheck, sizeof pageOpcodeCheck))
			return false;
		if (memcmp(pageOpcodeCheck, pageOpcode, sizeof pageOpcode) != 0)
			return false;
		if (!Wpm(ErectusProcess::exe + OFFSET_REDIRECTION_JMP, &pageJmpOff, sizeof pageJmpOff))
			return false;
		if (!FreeEx(pageCheck))
			return false;
	}

	if (!*targetingPage)
	{
		const auto page = AllocEx(sizeof(Opk));
		if (!page)
			return false;
		*targetingPage = page;
	}

	if (!*targetingPageValid)
	{
		TargetLocking targetLockingData;
		targetLockingData.targetLockingPtr = targetLockingPtr;
		auto originalFunction = ErectusProcess::exe + OFFSET_REDIRECTION + sizeof redirectionOff;
		DWORD64 originalFunctionCheck;
		if (!Rpm(*targetingPage + 0x30, &originalFunctionCheck, sizeof originalFunctionCheck))
			return false;

		if (originalFunctionCheck != originalFunction)
			memcpy(&targetLockingData.redirectionAsm[0x30], &originalFunction, sizeof originalFunction);

		if (!Wpm(*targetingPage, &targetLockingData, sizeof targetLockingData))
			return false;

		*targetingPageValid = true;
		return false;
	}
	TargetLocking targetLockingData;
	if (!Rpm(*targetingPage, &targetLockingData, sizeof targetLockingData))
		return false;

	if (targetLockingData.targetLockingPtr != targetLockingPtr)
	{
		targetLockingData.targetLockingPtr = targetLockingPtr;
		if (!Wpm(*targetingPage, &targetLockingData, sizeof targetLockingData))
			return false;
		memcpy(&pageJmpOn[2], &*targetingPage, sizeof(DWORD64));
	}
	memcpy(&pageJmpOn[2], &*targetingPage, sizeof(DWORD64));

	auto isPlayer = false;
	auto targetValid = LockedTargetValid(&isPlayer);
	if (!Settings::targetting.indirectPlayers && isPlayer || !Settings::targetting.indirectNpCs && !isPlayer)
		targetValid = false;

	const auto redirection = Rpm(ErectusProcess::exe + OFFSET_REDIRECTION, &redirectionCheck,
		sizeof redirectionCheck);

	if (*targetingPageValid && state && targetValid)
	{
		if (redirection && !memcmp(redirectionCheck, redirectionOff, sizeof redirectionOff))
			Wpm(ErectusProcess::exe + OFFSET_REDIRECTION, &redirectionOn, sizeof redirectionOn);
	}
	else
	{
		if (redirection && !memcmp(redirectionCheck, redirectionOn, sizeof redirectionOn))
			Wpm(ErectusProcess::exe + OFFSET_REDIRECTION, &redirectionOff, sizeof redirectionOff);
	}

	if (*targetingPageValid && !isExiting && !memcmp(pageJmpCheck, pageJmpOff, sizeof pageJmpOff))
		return Wpm(ErectusProcess::exe + OFFSET_REDIRECTION_JMP, &pageJmpOn, sizeof pageJmpOn);
	if (isExiting && !memcmp(pageJmpCheck, pageJmpOn, sizeof pageJmpOn))
		return Wpm(ErectusProcess::exe + OFFSET_REDIRECTION_JMP, &pageJmpOff, sizeof pageJmpOff);
	return true;
}

bool ErectusMemory::MovePlayer()
{
	DWORD64 bhkCharProxyControllerPtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_CHAR_CONTROLLER, &bhkCharProxyControllerPtr, sizeof bhkCharProxyControllerPtr))
		return false;
	if (!Utils::Valid(bhkCharProxyControllerPtr))
		return false;

	BhkCharProxyController bhkCharProxyControllerData{};
	if (!Rpm(bhkCharProxyControllerPtr, &bhkCharProxyControllerData, sizeof bhkCharProxyControllerData))
		return false;
	if (!Utils::Valid(bhkCharProxyControllerData.hknpBsCharacterProxyPtr))
		return false;

	HknpBsCharacterProxy hknpBsCharacterProxyData{};
	if (!Rpm(bhkCharProxyControllerData.hknpBsCharacterProxyPtr, &hknpBsCharacterProxyData, sizeof hknpBsCharacterProxyData))
		return false;


	float velocityA[4];
	memset(velocityA, 0x00, sizeof velocityA);

	float velocityB[4];
	memset(velocityB, 0x00, sizeof velocityB);

	auto speed = Settings::customLocalPlayerSettings.noclipSpeed;
	if (GetAsyncKeyState(VK_SHIFT))
		speed *= 1.5f;

	auto writePosition = false;
	auto cameraData = GetCameraInfo();

	if (GetAsyncKeyState('W'))
	{
		hknpBsCharacterProxyData.position[0] += cameraData.forward[0] * speed;
		hknpBsCharacterProxyData.position[1] += cameraData.forward[1] * speed;
		hknpBsCharacterProxyData.position[2] += cameraData.forward[2] * speed;
		writePosition = true;
	}

	if (GetAsyncKeyState('A'))
	{
		hknpBsCharacterProxyData.position[0] -= cameraData.forward[1] * speed;
		hknpBsCharacterProxyData.position[1] += cameraData.forward[0] * speed;
		writePosition = true;
	}

	if (GetAsyncKeyState('S'))
	{
		hknpBsCharacterProxyData.position[0] -= cameraData.forward[0] * speed;
		hknpBsCharacterProxyData.position[1] -= cameraData.forward[1] * speed;
		hknpBsCharacterProxyData.position[2] -= cameraData.forward[2] * speed;
		writePosition = true;
	}

	if (GetAsyncKeyState('D'))
	{
		hknpBsCharacterProxyData.position[0] += cameraData.forward[1] * speed;
		hknpBsCharacterProxyData.position[1] -= cameraData.forward[0] * speed;
		writePosition = true;
	}

	if (memcmp(hknpBsCharacterProxyData.velocityA, velocityA, sizeof velocityA) != 0)
	{
		memcpy(hknpBsCharacterProxyData.velocityA, velocityA, sizeof velocityA);
		writePosition = true;
	}

	if (memcmp(hknpBsCharacterProxyData.velocityB, velocityB, sizeof velocityB) != 0)
	{
		memcpy(hknpBsCharacterProxyData.velocityB, velocityB, sizeof velocityB);
		writePosition = true;
	}

	if (writePosition)
		return Wpm(bhkCharProxyControllerData.hknpBsCharacterProxyPtr, &hknpBsCharacterProxyData, sizeof hknpBsCharacterProxyData);

	return true;
}

void ErectusMemory::Noclip(const bool state)
{
	BYTE noclipOnA[] = { 0x0F, 0x1F, 0x44, 0x00, 0x00 };
	BYTE noclipOffA[] = { 0xE8, 0xC3, 0xC5, 0xFE, 0xFF };
	BYTE noclipCheckA[sizeof noclipOffA];

	BYTE noclipOnB[] = { 0x0F, 0x1F, 0x40, 0x00 };
	BYTE noclipOffB[] = { 0x41, 0xFF, 0x50, 0x40 };
	BYTE noclipCheckB[sizeof noclipOffB];

	BYTE noclipOnC[] = { 0x0F, 0x1F, 0x44, 0x00, 0x00 };
	BYTE noclipOffC[] = { 0xE8, 0x9A, 0xA1, 0x34, 0x01 };
	BYTE noclipCheckC[sizeof noclipOffC];

	BYTE noclipOnD[] = { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 };
	BYTE noclipOffD[] = { 0xFF, 0x15, 0x59, 0xEC, 0xFF, 0x01 };
	BYTE noclipCheckD[sizeof noclipOffD];

	const auto noclipA = Rpm(ErectusProcess::exe + OFFSET_NOCLIP_A, &noclipCheckA, sizeof noclipCheckA);
	const auto noclipB = Rpm(ErectusProcess::exe + OFFSET_NOCLIP_B, &noclipCheckB, sizeof noclipCheckB);
	const auto noclipC = Rpm(ErectusProcess::exe + OFFSET_NOCLIP_C, &noclipCheckC, sizeof noclipCheckC);
	const auto noclipD = Rpm(ErectusProcess::exe + OFFSET_NOCLIP_D, &noclipCheckD, sizeof noclipCheckD);

	if (state)
	{
		if (noclipA && !memcmp(noclipCheckA, noclipOffA, sizeof noclipOffA))
			Wpm(ErectusProcess::exe + OFFSET_NOCLIP_A, &noclipOnA, sizeof noclipOnA);

		if (noclipB && !memcmp(noclipCheckB, noclipOffB, sizeof noclipOffB))
			Wpm(ErectusProcess::exe + OFFSET_NOCLIP_B, &noclipOnB, sizeof noclipOnB);

		if (noclipC && !memcmp(noclipCheckC, noclipOffC, sizeof noclipOffC))
			Wpm(ErectusProcess::exe + OFFSET_NOCLIP_C, &noclipOnC, sizeof noclipOnC);

		if (noclipD && !memcmp(noclipCheckD, noclipOffD, sizeof noclipOffD))
			Wpm(ErectusProcess::exe + OFFSET_NOCLIP_D, &noclipOnD, sizeof noclipOnD);

		MovePlayer();
	}
	else
	{
		if (noclipA && !memcmp(noclipCheckA, noclipOnA, sizeof noclipOnA))
			Wpm(ErectusProcess::exe + OFFSET_NOCLIP_A, &noclipOffA, sizeof noclipOffA);

		if (noclipB && !memcmp(noclipCheckB, noclipOnB, sizeof noclipOnB))
			Wpm(ErectusProcess::exe + OFFSET_NOCLIP_B, &noclipOffB, sizeof noclipOffB);

		if (noclipC && !memcmp(noclipCheckC, noclipOnC, sizeof noclipOnC))
			Wpm(ErectusProcess::exe + OFFSET_NOCLIP_C, &noclipOffC, sizeof noclipOffC);

		if (noclipD && !memcmp(noclipCheckD, noclipOnD, sizeof noclipOnD))
			Wpm(ErectusProcess::exe + OFFSET_NOCLIP_D, &noclipOffD, sizeof noclipOffD);
	}
}

bool ErectusMemory::ActorValue(DWORD64* actorValuePage, bool* actorValuePageValid, const bool state)
{
	if (!*actorValuePage)
	{
		const auto page = AllocEx(sizeof(ActorValueHook));
		if (!page) return false;
		*actorValuePage = page;
	}

	const auto localPlayerPtr = GetLocalPlayerPtr(false);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return false;
	if (!Utils::Valid(localPlayer.vtable0050))
		return false;

	DWORD64 actorValueFunction;
	if (!Rpm(localPlayer.vtable0050 + 0x48, &actorValueFunction, sizeof actorValueFunction))
		return false;
	if (!Utils::Valid(actorValueFunction))
		return false;

	ActorValueHook actorValueHookData;
	actorValueHookData.actionPointsEnabled = static_cast<int>(Settings::customLocalPlayerSettings.actionPointsEnabled);
	actorValueHookData.actionPoints = static_cast<float>(Settings::customLocalPlayerSettings.actionPoints);
	actorValueHookData.strengthEnabled = static_cast<int>(Settings::customLocalPlayerSettings.strengthEnabled);
	actorValueHookData.strength = static_cast<float>(Settings::customLocalPlayerSettings.strength);
	actorValueHookData.perceptionEnabled = static_cast<int>(Settings::customLocalPlayerSettings.perceptionEnabled);
	actorValueHookData.perception = static_cast<float>(Settings::customLocalPlayerSettings.perception);
	actorValueHookData.enduranceEnabled = static_cast<int>(Settings::customLocalPlayerSettings.enduranceEnabled);
	actorValueHookData.endurance = static_cast<float>(Settings::customLocalPlayerSettings.endurance);
	actorValueHookData.charismaEnabled = static_cast<int>(Settings::customLocalPlayerSettings.charismaEnabled);
	actorValueHookData.charisma = static_cast<float>(Settings::customLocalPlayerSettings.charisma);
	actorValueHookData.intelligenceEnabled = static_cast<int>(Settings::customLocalPlayerSettings.intelligenceEnabled);
	actorValueHookData.intelligence = static_cast<float>(Settings::customLocalPlayerSettings.intelligence);
	actorValueHookData.agilityEnabled = static_cast<int>(Settings::customLocalPlayerSettings.agilityEnabled);
	actorValueHookData.agility = static_cast<float>(Settings::customLocalPlayerSettings.agility);
	actorValueHookData.luckEnabled = static_cast<int>(Settings::customLocalPlayerSettings.luckEnabled);
	actorValueHookData.luck = static_cast<float>(Settings::customLocalPlayerSettings.luck);
	actorValueHookData.originalFunction = ErectusProcess::exe + OFFSET_ACTOR_VALUE;

	if (actorValueFunction != ErectusProcess::exe + OFFSET_ACTOR_VALUE && actorValueFunction != *actorValuePage)
	{
		if (VtableSwap(localPlayer.vtable0050 + 0x48, ErectusProcess::exe + OFFSET_ACTOR_VALUE))
			FreeEx(actorValueFunction);
	}

	if (state)
	{
		if (*actorValuePageValid)
		{
			ActorValueHook actorValueHookCheck;
			if (!Rpm(*actorValuePage, &actorValueHookCheck, sizeof actorValueHookCheck))
				return false;
			if (memcmp(&actorValueHookData, &actorValueHookCheck, sizeof actorValueHookCheck) != 0)
				return Wpm(*actorValuePage, &actorValueHookData, sizeof actorValueHookData);
		}
		else
		{
			if (!Wpm(*actorValuePage, &actorValueHookData, sizeof actorValueHookData))
				return false;
			if (!VtableSwap(localPlayer.vtable0050 + 0x48, *actorValuePage))
				return false;
			*actorValuePageValid = true;
		}
	}
	else
	{
		if (actorValueFunction != ErectusProcess::exe + OFFSET_ACTOR_VALUE)
		{
			if (VtableSwap(localPlayer.vtable0050 + 0x48, ErectusProcess::exe + OFFSET_ACTOR_VALUE))
				FreeEx(actorValueFunction);
		}

		if (*actorValuePage)
		{
			if (FreeEx(*actorValuePage))
			{
				*actorValuePage = 0;
				*actorValuePageValid = false;
			}
		}
	}

	return true;
}

bool ErectusMemory::SetActorValueMaximum(const DWORD formId, const float defaultValue, const float customValue, const bool state)
{
	const auto actorValuePtr = GetPtr(formId);
	if (!Utils::Valid(actorValuePtr))
		return false;

	ActorValueInformation actorValueData{};
	if (!Rpm(actorValuePtr, &actorValueData, sizeof actorValueData))
		return false;

	if (state)
	{
		if (actorValueData.maximumValue != customValue)
		{
			actorValueData.maximumValue = customValue;
			return Wpm(actorValuePtr, &actorValueData, sizeof actorValueData);
		}
	}
	else
	{
		if (actorValueData.maximumValue != defaultValue)
		{
			actorValueData.maximumValue = defaultValue;
			return Wpm(actorValuePtr, &actorValueData, sizeof actorValueData);
		}
	}

	return true;
}

bool ErectusMemory::OnePositionKill(DWORD64* opkPage, bool* opkPageValid, const bool state)
{
	if (!*opkPage)
	{
		const auto page = AllocEx(sizeof(Opk));
		if (!page)
			return false;
		*opkPage = page;
	}

	BYTE opkOn[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0, 0xCC, 0xCC, 0xCC };
	BYTE opkOff[] = { 0x0F, 0x10, 0x87, 0x90, 0x04, 0x00, 0x00, 0x0F, 0x58, 0x45, 0xA7, 0x0F, 0x29, 0x45, 0xF7 };
	BYTE opkCheck[sizeof opkOff];

	if (!Rpm(ErectusProcess::exe + OFFSET_OPK, &opkCheck, sizeof opkCheck))
		return false;

	const auto originalFunction = ErectusProcess::exe + OFFSET_OPK + sizeof opkOff;
	memcpy(&opkOn[2], &*opkPage, sizeof(DWORD64));

	DWORD64 pageCheck;
	memcpy(&pageCheck, &opkCheck[2], sizeof(DWORD64));

	if (Utils::Valid(pageCheck) && pageCheck != *opkPage)
	{
		Opk buffer;
		if (!Rpm(pageCheck, &buffer, sizeof buffer))
			return false;
		if (buffer.originalFunction != originalFunction)
			return false;
		if (!Wpm(ErectusProcess::exe + OFFSET_OPK, &opkOff, sizeof opkOff))
			return false;
		FreeEx(pageCheck);
	}

	if (state)
	{
		if (*opkPageValid)
			return true;

		Opk opkData;
		opkData.opkPlayers = 0;
		opkData.opkNpcs = 0;
		opkData.originalFunction = originalFunction;
		memset(opkData.opkPlayerPosition, 0x00, sizeof opkData.opkPlayerPosition);
		memset(opkData.opkNpcPosition, 0x00, sizeof opkData.opkNpcPosition);

		if (!Wpm(*opkPage, &opkData, sizeof opkData))
			return false;
		if (!Wpm(ErectusProcess::exe + OFFSET_OPK, &opkOn, sizeof opkOn))
			return false;
		*opkPageValid = true;
	}
	else
	{
		if (pageCheck == *opkPage)
			Wpm(ErectusProcess::exe + OFFSET_OPK, &opkOff, sizeof opkOff);

		if (*opkPage && FreeEx(*opkPage))
		{
			*opkPage = 0;
			*opkPageValid = false;
		}

	}

	return true;
}

bool ErectusMemory::CheckOpkDistance(const DWORD64 opkPage, const bool players)
{
	Opk opkData;
	if (!Rpm(opkPage, &opkData, sizeof opkData))
		return false;

	auto cameraData = GetCameraInfo();

	float editedOrigin[3];
	editedOrigin[0] = cameraData.origin[0] / 70.0f;
	editedOrigin[1] = cameraData.origin[1] / 70.0f;
	editedOrigin[2] = cameraData.origin[2] / 70.0f;

	if (players)
	{
		const auto distance = Utils::GetDistance(opkData.opkPlayerPosition, editedOrigin);
		if (distance > 20.0f)
			return false;
	}
	else
	{
		const auto distance = Utils::GetDistance(opkData.opkNpcPosition, editedOrigin);
		if (distance > 20.0f)
			return false;
	}

	return true;
}

bool ErectusMemory::SetOpkData(const DWORD64 opkPage, const bool players, const bool state)
{
	Opk opkData;
	if (!Rpm(opkPage, &opkData, sizeof opkData))
		return false;

	if (!state)
	{
		auto writeData = false;

		if (players && opkData.opkPlayers)
		{
			opkData.opkPlayers = 0;
			memset(opkData.opkPlayerPosition, 0x00, sizeof opkData.opkPlayerPosition);
			writeData = true;
		}
		else if (!players && opkData.opkNpcs)
		{
			opkData.opkNpcs = 0;
			memset(opkData.opkNpcPosition, 0x00, sizeof opkData.opkNpcPosition);
			writeData = true;
		}

		if (writeData)
			Wpm(opkPage, &opkData, sizeof opkData);

		return true;
	}

	if (CheckOpkDistance(opkPage, players))
		return true;

	auto cameraData = GetCameraInfo();

	float editedOrigin[3];
	editedOrigin[0] = cameraData.origin[0] / 70.0f;
	editedOrigin[1] = cameraData.origin[1] / 70.0f;
	editedOrigin[2] = cameraData.origin[2] / 70.0f;

	float opkPosition[3];
	Utils::ProjectView(opkPosition, cameraData.forward, editedOrigin, 3.0f);

	if (players)
	{
		opkData.opkPlayerPosition[0] = opkPosition[0];
		opkData.opkPlayerPosition[1] = opkPosition[1];
		opkData.opkPlayerPosition[2] = opkPosition[2];
		opkData.opkPlayers = 1;
	}
	else
	{
		opkData.opkNpcPosition[0] = opkPosition[0];
		opkData.opkNpcPosition[1] = opkPosition[1];
		opkData.opkNpcPosition[2] = opkPosition[2];
		opkData.opkNpcs = 1;
	}

	return Wpm(opkPage, &opkData, sizeof opkData);
}

bool ErectusMemory::InsideInteriorCell()
{
	DWORD64 entityListTypePtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_ENTITY_LIST, &entityListTypePtr, sizeof entityListTypePtr))
		return false;
	if (!Utils::Valid(entityListTypePtr))
		return false;

	LoadedAreaManager entityListTypeData{};
	if (!Rpm(entityListTypePtr, &entityListTypeData, sizeof entityListTypeData))
		return false;
	if (!Utils::Valid(entityListTypeData.interiorCellArrayPtr))
		return false;
	if (!Utils::Valid(entityListTypeData.interiorCellArrayPtr2))
		return false;
	if (!Utils::Valid(entityListTypeData.exteriorCellArrayPtr))
		return false;
	if (!Utils::Valid(entityListTypeData.exteriorCellArrayPtr2))
		return false;

	if (entityListTypeData.interiorCellArrayPtr != entityListTypeData.interiorCellArrayPtr2)
		return true;

	return false;
}

LocalPlayerInfo ErectusMemory::GetLocalPlayerInfo()
{
	LocalPlayerInfo result = {};

	const auto localPlayerPtr = GetLocalPlayerPtr(false);
	if (!Utils::Valid(localPlayerPtr))
		return result;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return result;

	DWORD cellFormId = 0;
	if (Utils::Valid(localPlayer.cellPtr) && !Rpm(localPlayer.cellPtr + 0x20, &cellFormId, sizeof cellFormId))
		cellFormId = 0;

	auto playerHealth = -1.0f;
	ActorSnapshotComponent actorSnapshotComponentData{};
	if (GetActorSnapshotComponentData(localPlayer, &actorSnapshotComponentData))
		playerHealth = actorSnapshotComponentData.maxHealth + actorSnapshotComponentData.modifiedHealth + actorSnapshotComponentData.lostHealth;

	result.formId = localPlayer.formId;
	result.stashFormId = localPlayer.formId0C24;
	result.cellFormId = cellFormId;
	result.yaw = localPlayer.yaw;
	result.pitch = localPlayer.pitch;
	result.currentHealth = playerHealth;

	result.position[0] = localPlayer.position[0];
	result.position[1] = localPlayer.position[1];
	result.position[2] = localPlayer.position[2];

	return result;
}

bool ErectusMemory::ReferenceSwap(DWORD& sourceFormId, DWORD& destinationFormId)
{
	if (sourceFormId == destinationFormId)
		return true;

	auto sourcePointer = GetPtr(sourceFormId);
	if (!Utils::Valid(sourcePointer))
	{
		sourceFormId = 0x00000000;
		return false;
	}

	const auto destinationAddress = GetAddress(destinationFormId);
	if (!Utils::Valid(destinationAddress))
	{
		destinationFormId = 0x00000000;
		return false;
	}

	return Wpm(destinationAddress, &sourcePointer, sizeof sourcePointer);
}

bool ErectusMemory::CheckItemTransferList()
{
	for (auto i = 0; i < 32; i++)
	{
		if (Settings::customTransferSettings.whitelist[i] && Settings::customTransferSettings.whitelisted[i])
			return true;
	}

	return false;
}

bool ErectusMemory::TransferItems(const DWORD sourceFormId, const DWORD destinationFormId)
{
	auto sourcePtr = GetPtr(sourceFormId);
	if (!Utils::Valid(sourcePtr))
		return false;

	auto destinationPtr = GetPtr(destinationFormId);
	if (!Utils::Valid(destinationPtr))
		return false;

	TesObjectRefr entityData{};
	if (!Rpm(sourcePtr, &entityData, sizeof entityData))
		return false;
	if (!Utils::Valid(entityData.inventoryPtr))
		return false;

	Inventory inventoryData{};
	if (!Rpm(entityData.inventoryPtr, &inventoryData, sizeof inventoryData))
		return false;
	if (!Utils::Valid(inventoryData.itemArrayPtr) || inventoryData.itemArrayEnd < inventoryData.itemArrayPtr)
		return false;

	auto itemArraySize = (inventoryData.itemArrayEnd - inventoryData.itemArrayPtr) / sizeof(Item);
	if (!itemArraySize || itemArraySize > 0x7FFF)
		return false;

	auto* itemData = new Item[itemArraySize];
	if (!Rpm(inventoryData.itemArrayPtr, &*itemData, itemArraySize * sizeof(Item)))
	{
		delete[]itemData;
		itemData = nullptr;
		return false;
	}

	for (DWORD64 i = 0; i < itemArraySize; i++)
	{
		if (!Utils::Valid(itemData[i].referencePtr))
			continue;
		if (!Utils::Valid(itemData[i].displayPtr) || itemData[i].iterations < itemData[i].displayPtr)
			continue;

		if (Settings::customTransferSettings.useWhitelist || Settings::customTransferSettings.useBlacklist)
		{
			TesItem referenceData{};
			if (!Rpm(itemData[i].referencePtr, &referenceData, sizeof referenceData))
				continue;

			if (Settings::customTransferSettings.useWhitelist)
			{
				if (!CheckFormIdArray(referenceData.formId, Settings::customTransferSettings.whitelisted, Settings::customTransferSettings.whitelist, 32))
					continue;
			}

			if (Settings::customTransferSettings.useBlacklist)
			{
				if (CheckFormIdArray(referenceData.formId, Settings::customTransferSettings.blacklisted, Settings::customTransferSettings.blacklist, 32))
					continue;
			}
		}

		auto iterations = (itemData[i].iterations - itemData[i].displayPtr) / sizeof(ItemCount);
		if (!iterations || iterations > 0x7FFF)
			continue;

		auto* itemCountData = new ItemCount[iterations];
		if (!Rpm(itemData[i].displayPtr, &*itemCountData, iterations * sizeof(ItemCount)))
		{
			delete[]itemCountData;
			itemCountData = nullptr;
			continue;
		}

		auto count = 0;
		for (DWORD64 c = 0; c < iterations; c++)
		{
			count += itemCountData[c].count;
		}

		delete[]itemCountData;
		itemCountData = nullptr;

		if (count == 0)
			continue;

		TransferMessage transferMessageData{};
		transferMessageData.vtable = ErectusProcess::exe + VTABLE_REQUESTTRANSFERITEMMSG;
		transferMessageData.srcFormId = sourceFormId;
		transferMessageData.unknownId = 0xE0001F7A;
		transferMessageData.dstFormId = destinationFormId;
		transferMessageData.itemId = itemData[i].itemId;
		transferMessageData.count = count;
		transferMessageData.unknownA = 0x00000000;
		transferMessageData.unknownB = 0x00;
		transferMessageData.unknownC = 0x01;
		transferMessageData.unknownD = 0x00;
		transferMessageData.unknownE = 0x02;
		SendMessageToServer(&transferMessageData, sizeof transferMessageData);
	}

	delete[]itemData;
	itemData = nullptr;
	return true;
}

bool ErectusMemory::GetTeleportPosition(const int index)
{
	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return false;
	if (!Utils::Valid(localPlayer.cellPtr))
		return false;

	DWORD cellFormId;
	if (!Rpm(localPlayer.cellPtr + 0x20, &cellFormId, sizeof cellFormId))
		return false;

	Settings::teleporter.entries[index].destination[0] = localPlayer.position[0];
	Settings::teleporter.entries[index].destination[1] = localPlayer.position[1];
	Settings::teleporter.entries[index].destination[2] = localPlayer.position[2];
	Settings::teleporter.entries[index].destination[3] = localPlayer.yaw;
	Settings::teleporter.entries[index].cellFormId = cellFormId;

	return true;
}

bool ErectusMemory::RequestTeleport(const int index)
{
	const auto cellPtr = GetPtr(Settings::teleporter.entries[index].cellFormId);
	if (!Utils::Valid(cellPtr))
		return false;

	RequestTeleportMessage requestTeleportMessageData =
	{
		.vtable = ErectusProcess::exe + VTABLE_REQUESTTELEPORTTOLOCATIONMSG,
		.positionX = Settings::teleporter.entries[index].destination[0],
		.positionY = Settings::teleporter.entries[index].destination[1],
		.positionZ = Settings::teleporter.entries[index].destination[2],
		.rotationX = 0.0f,
		.rotationY = 0.0f,
		.rotationZ = Settings::teleporter.entries[index].destination[3],
		.cellPtr = cellPtr
	};

	return SendMessageToServer(&requestTeleportMessageData, sizeof requestTeleportMessageData);
}

DWORD ErectusMemory::GetLocalPlayerFormId()
{
	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return 0;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return 0;

	return localPlayer.formId;
}

DWORD ErectusMemory::GetStashFormId()
{
	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return 0;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return 0;

	return localPlayer.formId0C24;
}

void ErectusMemory::UpdateNukeCodes()
{
	GetNukeCode(0x000921AE, alphaCode);
	GetNukeCode(0x00092213, bravoCode);
	GetNukeCode(0x00092214, charlieCode);
}

bool ErectusMemory::FreezeActionPoints(DWORD64* freezeApPage, bool* freezeApPageValid, const bool state)
{
	if (!*freezeApPage)
	{
		const auto page = AllocEx(sizeof(FreezeAp));
		if (!page)
			return false;

		*freezeApPage = page;
	}

	BYTE freezeApOn[]
	{
		0x0F, 0x1F, 0x40, 0x00, //nop [rax+00]
		0x48, 0xBF, //mov rdi (Page)
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //Page (mov rdi)
		0xFF, 0xE7, //jmp rdi
		0x0F, 0x1F, 0x40, 0x00, //nop [rax+00]
	};

	BYTE freezeApOff[]
	{
		0x8B, 0xD6, //mov edx, esi
		0x48, 0x8B, 0xC8, //mov rcx, rax
		0x48, 0x8B, 0x5C, 0x24, 0x30, //mov rbx, [rsp+30]
		0x48, 0x8B, 0x74, 0x24, 0x38, //mov rsi, [rsp+38]
		0x48, 0x83, 0xC4, 0x20, //add rsp, 20
		0x5F, //pop rdi
	};

	BYTE freezeApCheck[sizeof freezeApOff];

	if (!Rpm(ErectusProcess::exe + OFFSET_AV_REGEN, &freezeApCheck, sizeof freezeApCheck))
		return false;

	DWORD64 pageCheck;
	memcpy(&pageCheck, &freezeApCheck[0x6], sizeof(DWORD64));

	if (Utils::Valid(pageCheck) && pageCheck != *freezeApPage)
	{
		for (auto i = 0; i < 0x6; i++) if (freezeApCheck[i] != freezeApOn[i])
			return false;
		if (!Wpm(ErectusProcess::exe + OFFSET_AV_REGEN, &freezeApOff, sizeof freezeApOff))
			return false;
		FreeEx(pageCheck);
	}

	if (state)
	{
		FreezeAp freezeApData;
		freezeApData.freezeApEnabled = Settings::customLocalPlayerSettings.freezeApEnabled;

		if (*freezeApPageValid)
		{
			FreezeAp freezeApPageCheck;
			if (!Rpm(*freezeApPage, &freezeApPageCheck, sizeof freezeApPageCheck))
				return false;
			if (!memcmp(&freezeApData, &freezeApPageCheck, sizeof freezeApPageCheck))
				return true;
			return Wpm(*freezeApPage, &freezeApData, sizeof freezeApData);
		}
		if (!Wpm(*freezeApPage, &freezeApData, sizeof freezeApData))
			return false;
		memcpy(&freezeApOn[0x6], &*freezeApPage, sizeof(DWORD64));
		if (!Wpm(ErectusProcess::exe + OFFSET_AV_REGEN, &freezeApOn, sizeof freezeApOn))
			return false;
		*freezeApPageValid = true;
	}
	else
	{
		if (pageCheck == *freezeApPage)
			Wpm(ErectusProcess::exe + OFFSET_AV_REGEN, &freezeApOff, sizeof freezeApOff);

		if (*freezeApPage)
		{
			if (FreeEx(*freezeApPage))
			{
				*freezeApPage = 0;
				*freezeApPageValid = false;
			}
		}
	}

	return true;
}

bool ErectusMemory::SetClientState(const DWORD64 clientState)
{
	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	if (InsideInteriorCell())
		return false;

	ClientStateMsg clientStateMsgData{ .vtable = ErectusProcess::exe + VTABLE_CLIENTSTATEMSG, .clientState = clientState };

	return SendMessageToServer(&clientStateMsgData, sizeof clientStateMsgData);
}

bool ErectusMemory::PositionSpoofing(const bool state)
{
	BYTE positionSpoofingOn[] = {
		0xBA, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x11, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC
	};
	BYTE positionSpoofingOff[] = {
		0xBA, 0x01, 0x00, 0xF8, 0xFF, 0x3B, 0xC2, 0x7C, 0x0F, 0x8B, 0xD0, 0x41, 0xB8, 0xFF, 0xFF, 0x07, 0x00, 0x41,
		0x3B, 0xC0, 0x41, 0x0F, 0x4F, 0xD0
	};
	BYTE positionSpoofingCheck[sizeof positionSpoofingOff];

	if (!Rpm(ErectusProcess::exe + OFFSET_SERVER_POSITION, &positionSpoofingCheck, sizeof positionSpoofingCheck))
		return false;

	if (!state)
	{
		positionSpoofingCheck[1] = 0x00;
		positionSpoofingCheck[2] = 0x00;
		positionSpoofingCheck[3] = 0x00;
		positionSpoofingCheck[4] = 0x00;

		if (!memcmp(positionSpoofingCheck, positionSpoofingOn, sizeof positionSpoofingOn))
			return Wpm(ErectusProcess::exe + OFFSET_SERVER_POSITION, &positionSpoofingOff, sizeof positionSpoofingOff);

		return true;
	}

	int spoofingHeightCheck;
	memcpy(&spoofingHeightCheck, &positionSpoofingCheck[1], sizeof spoofingHeightCheck);
	memcpy(&positionSpoofingOn[1], &Settings::customLocalPlayerSettings.positionSpoofingHeight,
		sizeof Settings::customLocalPlayerSettings.positionSpoofingHeight);

	if (!memcmp(positionSpoofingCheck, positionSpoofingOn, sizeof positionSpoofingOn))
		return true;
	if (!memcmp(positionSpoofingCheck, positionSpoofingOff, sizeof positionSpoofingOff))
		return Wpm(ErectusProcess::exe + OFFSET_SERVER_POSITION, &positionSpoofingOn, sizeof positionSpoofingOn);
	if (spoofingHeightCheck != Settings::customLocalPlayerSettings.positionSpoofingHeight)
	{
		if (positionSpoofingCheck[0] != 0xBA || spoofingHeightCheck < -524287 || spoofingHeightCheck > 524287)
			return false;

		return Wpm(ErectusProcess::exe + OFFSET_SERVER_POSITION, &positionSpoofingOn,
			sizeof positionSpoofingOn);
	}
	return false;
}

DWORD ErectusMemory::GetEntityId(const TesObjectRefr& entityData)
{
	if (!(entityData.idValue[0] & 1))
		return 0;

	DWORD v1;
	memcpy(&v1, entityData.idValue, sizeof v1);

	const auto v2 = v1 >> 1;
	const auto v3 = v2 + v2;

	DWORD v4;
	if (!Rpm(ErectusProcess::exe + OFFSET_ENTITY_ID + v3 * 0x8, &v4, sizeof v4))
		return 0;

	const auto v5 = v4 & 0x7FF80000;
	const auto v6 = v5 | v2;

	return v6;
}

bool ErectusMemory::SendHitsToServer(Hits* hitsData, const size_t hitsDataSize)
{
	const auto allocSize = sizeof(ExternalFunction) + sizeof(RequestHitsOnActors) + hitsDataSize;
	const auto allocAddress = AllocEx(allocSize);
	if (allocAddress == 0)
		return false;

	ExternalFunction externalFunctionData;
	externalFunctionData.address = ErectusProcess::exe + OFFSET_MESSAGE_SENDER;
	externalFunctionData.rcx = allocAddress + sizeof(ExternalFunction);
	externalFunctionData.rdx = 0;
	externalFunctionData.r8 = 0;
	externalFunctionData.r9 = 0;

	auto* pageData = new BYTE[allocSize];
	memset(pageData, 0x00, allocSize);

	RequestHitsOnActors requestHitsOnActorsData{};
	memset(&requestHitsOnActorsData, 0x00, sizeof(RequestHitsOnActors));

	requestHitsOnActorsData.vtable = ErectusProcess::exe + VTABLE_REQUESTHITSONACTORS;
	requestHitsOnActorsData.hitsArrayPtr = allocAddress + sizeof(ExternalFunction) + sizeof(RequestHitsOnActors);
	requestHitsOnActorsData.hitsArrayEnd = allocAddress + sizeof(ExternalFunction) + sizeof(RequestHitsOnActors) +
		hitsDataSize;

	memcpy(pageData, &externalFunctionData, sizeof externalFunctionData);
	memcpy(&pageData[sizeof(ExternalFunction)], &requestHitsOnActorsData, sizeof requestHitsOnActorsData);
	memcpy(&pageData[sizeof(ExternalFunction) + sizeof(RequestHitsOnActors)], &*hitsData, hitsDataSize);

	const auto pageWritten = Wpm(allocAddress, &*pageData, allocSize);

	delete[]pageData;
	pageData = nullptr;

	if (!pageWritten)
	{
		FreeEx(allocAddress);
		return false;
	}

	const auto paramAddress = allocAddress + sizeof ExternalFunction::ASM;
	auto* const thread = CreateRemoteThread(ErectusProcess::handle, nullptr, 0, LPTHREAD_START_ROUTINE(allocAddress),
		LPVOID(paramAddress), 0, nullptr);

	if (thread == nullptr)
	{
		FreeEx(allocAddress);
		return false;
	}

	const auto threadResult = WaitForSingleObject(thread, 3000);
	CloseHandle(thread);

	if (threadResult == WAIT_TIMEOUT)
		return false;

	FreeEx(allocAddress);

	return true;
}

bool ErectusMemory::SendDamage(const DWORD weaponId, BYTE* shotsHit, BYTE* shotsFired, const BYTE count)
{
	if (!MessagePatcher(allowMessages))
		return false;

	if (!allowMessages)
		return false;

	if (!weaponId)
		return false;

	auto isPlayer = false;
	LockedTargetValid(&isPlayer);
	if (!Settings::targetting.directPlayers && isPlayer || !Settings::targetting.directNpCs && !isPlayer)
		return false;

	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return false;

	const auto localPlayerId = GetEntityId(localPlayer);
	if (!localPlayerId)
		return false;

	TesObjectRefr target{};
	if (!Rpm(targetLockingPtr, &target, sizeof target)) return false;

	const auto targetId = GetEntityId(target);
	if (!targetId)
		return false;

	auto* hitsData = new Hits[count];
	memset(hitsData, 0x00, count * sizeof(Hits));

	if (*shotsHit == 0 || *shotsHit == 255)
		*shotsHit = 1;

	if (*shotsFired == 255)
		*shotsFired = 0;

	for (BYTE i = 0; i < count; i++)
	{
		hitsData[i].valueA = localPlayerId;
		hitsData[i].valueB = targetId;
		hitsData[i].valueC = 0;
		hitsData[i].initializationType = 0x3;
		hitsData[i].uiWeaponServerId = weaponId;
		hitsData[i].limbEnum = 0xFFFFFFFF;
		hitsData[i].hitEffectId = 0;
		hitsData[i].uEquipIndex = 0;
		hitsData[i].uAckIndex = *shotsHit;
		hitsData[i].uFireId = *shotsFired;
		hitsData[i].bPredictedKill = 0;
		hitsData[i].padding0023 = 0;
		hitsData[i].explosionLocationX = 0.0f;
		hitsData[i].explosionLocationY = 0.0f;
		hitsData[i].explosionLocationZ = 0.0f;
		hitsData[i].fProjectilePower = 1.0f;
		hitsData[i].bVatsAttack = 0;
		hitsData[i].bVatsCritical = 0;
		hitsData[i].bTargetWasDead = 0;
		hitsData[i].padding0037 = 0;

		if (Settings::targetting.sendDamageMax < 10)
		{
			if (Utils::GetRangedInt(1, 10) <= static_cast<int>(10 - Settings::targetting.sendDamageMax))
			{
				if (*shotsHit == 0 || *shotsHit == 255)
					*shotsHit = 1;
				else
					*shotsHit += 1;
			}
			else
			{
				*shotsHit = 1;
			}
		}
		else
			*shotsHit = 1;

		for (auto c = 0; c < Utils::GetRangedInt(1, 6); c++)
		{
			if (*shotsFired == 255)
				*shotsFired = 0;
			else
				*shotsFired += 1;
		}
	}

	const auto result = SendHitsToServer(hitsData, count * sizeof(Hits));

	delete[]hitsData;

	return result;
}

DWORD64 ErectusMemory::GetNukeCodePtr(const DWORD formId)
{
	ReferenceList questTextList{};
	if (!Rpm(ErectusProcess::exe + OFFSET_NUKE_CODE, &questTextList, sizeof questTextList))
		return 0;
	if (!Utils::Valid(questTextList.arrayPtr) || !questTextList.arraySize || questTextList.arraySize > 0x7FFF)
		return 0;

	auto* questTextArray = new DWORD64[questTextList.arraySize];
	if (!Rpm(questTextList.arrayPtr, &*questTextArray, questTextList.arraySize * sizeof(DWORD64)))
	{
		delete[]questTextArray;
		questTextArray = nullptr;
		return 0;
	}

	DWORD64 nukeCodePtr = 0;
	for (auto i = 0; i < questTextList.arraySize; i++)
	{
		if (!Utils::Valid(questTextArray[i]))
			continue;

		BgsQuestText bgsQuestTextData{};
		if (!Rpm(questTextArray[i], &bgsQuestTextData, sizeof bgsQuestTextData))
			continue;
		if (!Utils::Valid(bgsQuestTextData.formIdPtr) || !Utils::Valid(bgsQuestTextData.codePtr))
			continue;

		DWORD formIdCheck;
		if (!Rpm(bgsQuestTextData.formIdPtr + 0x4, &formIdCheck, sizeof formIdCheck))
			continue;
		if (formIdCheck != formId)
			continue;

		nukeCodePtr = bgsQuestTextData.codePtr;
		break;
	}

	delete[]questTextArray;
	questTextArray = nullptr;

	return nukeCodePtr;
}

bool ErectusMemory::GetNukeCode(const DWORD formId, std::array<int, 8>& nukeCode)
{
	const auto nukeCodePtr = GetNukeCodePtr(formId);
	if (!nukeCodePtr)
		return false;

	float nukeCodeArray[16];
	if (!Rpm(nukeCodePtr, &nukeCodeArray, sizeof nukeCodeArray))
		return false;

	for (auto i = 0; i < 8; i++)
	{
		if (nukeCodeArray[i * 2 + 1] < 0.0f || nukeCodeArray[i * 2 + 1] > 9.0f)
		{
			nukeCode = {};
			return false;
		}
		nukeCode[i] = static_cast<int>(nukeCodeArray[i * 2 + 1]);
	}

	return true;
}

DWORD ErectusMemory::GetFavoritedWeaponId(const BYTE favouriteIndex)
{

	if (Settings::targetting.favoriteIndex >= 12)
		return 0;

	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return 0;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return 0;
	if (!Utils::Valid(localPlayer.inventoryPtr))
		return 0;

	Inventory inventoryData{};
	if (!Rpm(localPlayer.inventoryPtr, &inventoryData, sizeof inventoryData))
		return 0;
	if (!Utils::Valid(inventoryData.itemArrayPtr) || inventoryData.itemArrayEnd < inventoryData.itemArrayPtr)
		return 0;

	const auto itemArraySize = (inventoryData.itemArrayEnd - inventoryData.itemArrayPtr) / sizeof(Item);
	if (!itemArraySize || itemArraySize > 0x7FFF)
		return 0;

	auto itemData = std::make_unique<Item[]>(itemArraySize);
	if (!Rpm(inventoryData.itemArrayPtr, itemData.get(), itemArraySize * sizeof(Item)))
		return 0;

	for (DWORD64 i = 0; i < itemArraySize; i++)
	{
		if (!Utils::Valid(itemData[i].referencePtr))
			continue;
		if (itemData[i].favoriteIndex != favouriteIndex)
			continue;

		TesItem referenceData{};
		if (!Rpm(itemData[i].referencePtr, &referenceData, sizeof referenceData))
			break;
		if (referenceData.formType != static_cast<BYTE>(FormTypes::TesObjectWeap))
			break;

		return itemData[i].itemId;
	}

	return 0;
}

char ErectusMemory::FavoriteIndex2Slot(const BYTE favoriteIndex)
{
	switch (favoriteIndex)
	{
	case 0x00:
		return '1';
	case 0x01:
		return '2';
	case 0x02:
		return '3';
	case 0x03:
		return '4';
	case 0x04:
		return '5';
	case 0x05:
		return '6';
	case 0x06:
		return '7';
	case 0x07:
		return '8';
	case 0x08:
		return '9';
	case 0x09:
		return '0';
	case 0x0A:
		return '-';
	case 0x0B:
		return '=';
	default:
		return '?';
	}
}

DWORD64 ErectusMemory::RttiGetNamePtr(const DWORD64 vtable)
{
	DWORD64 buffer;
	if (!Rpm(vtable - 0x8, &buffer, sizeof buffer))
		return 0;
	if (!Utils::Valid(buffer))
		return 0;

	DWORD offset;
	if (!Rpm(buffer + 0xC, &offset, sizeof offset))
		return 0;
	if (offset == 0 || offset > 0x7FFFFFFF)
		return 0;

	return ErectusProcess::exe + offset + 0x10;
}

std::string ErectusMemory::GetInstancedItemName(const DWORD64 displayPtr)
{
	std::string result{};

	if (!Utils::Valid(displayPtr))
		return result;

	DWORD64 instancedArrayPtr;
	if (!Rpm(displayPtr, &instancedArrayPtr, sizeof instancedArrayPtr))
		return result;
	if (!Utils::Valid(instancedArrayPtr))
		return result;

	ItemInstancedArray itemInstancedArrayData{};
	if (!Rpm(instancedArrayPtr, &itemInstancedArrayData, sizeof itemInstancedArrayData))
		return result;
	if (!Utils::Valid(itemInstancedArrayData.arrayPtr) || itemInstancedArrayData.arrayEnd < itemInstancedArrayData.arrayPtr)
		return result;

	const auto instancedArraySize = (itemInstancedArrayData.arrayEnd - itemInstancedArrayData.arrayPtr) / sizeof(DWORD64);
	if (!instancedArraySize || instancedArraySize > 0x7FFF)
		return result;

	auto instancedArray = std::make_unique<DWORD64[]>(instancedArraySize);
	if (!Rpm(itemInstancedArrayData.arrayPtr, instancedArray.get(), instancedArraySize * sizeof(DWORD64)))
		return result;

	for (DWORD64 i = 0; i < instancedArraySize; i++)
	{
		if (!Utils::Valid(instancedArray[i]))
			continue;

		ExtraTextDisplayData extraTextDisplayDataData{};
		if (!Rpm(instancedArray[i], &extraTextDisplayDataData, sizeof extraTextDisplayDataData))
			continue;

		const auto rttiNamePtr = RttiGetNamePtr(extraTextDisplayDataData.vtable);
		if (!rttiNamePtr)
			continue;

		char rttiNameCheck[sizeof".?AVExtraTextDisplayData@@"];
		if (!Rpm(rttiNamePtr, &rttiNameCheck, sizeof rttiNameCheck))
			continue;
		if (strcmp(rttiNameCheck, ".?AVExtraTextDisplayData@@") != 0)
			continue;

		result = GetEntityName(extraTextDisplayDataData.instancedNamePtr);
		return result;
	}

	return result;
}

std::unordered_map<int, std::string> ErectusMemory::GetFavoritedWeapons()
{
	std::unordered_map<int, std::string> result = {
		{0, (const char*)u8"[?] δѡ������"},
		{1, (const char*)u8"[1] �������Ч"},
		{2, (const char*)u8"[2] �������Ч"},
		{3, (const char*)u8"[3] �������Ч"},
		{4, (const char*)u8"[4] �������Ч"},
		{5, (const char*)u8"[5] �������Ч"},
		{6, (const char*)u8"[6] �������Ч"},
		{7, (const char*)u8"[7] �������Ч"},
		{8, (const char*)u8"[8] �������Ч"},
		{9, (const char*)u8"[9] �������Ч"},
		{10, (const char*)u8"[0] �������Ч"},
		{11, (const char*)u8"[-] �������Ч"},
		{12, (const char*)u8"[=] �������Ч"},
		{13, (const char*)u8"[?] �������Ч"},
	};

	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return result;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return result;
	if (!Utils::Valid(localPlayer.inventoryPtr))
		return result;

	Inventory inventoryData{};
	if (!Rpm(localPlayer.inventoryPtr, &inventoryData, sizeof inventoryData))
		return result;
	if (!Utils::Valid(inventoryData.itemArrayPtr) || inventoryData.itemArrayEnd < inventoryData.itemArrayPtr)
		return result;

	const auto itemArraySize = (inventoryData.itemArrayEnd - inventoryData.itemArrayPtr) / sizeof(Item);
	if (!itemArraySize || itemArraySize > 0x7FFF)
		return result;

	auto itemData = std::make_unique<Item[]>(itemArraySize);
	if (!Rpm(inventoryData.itemArrayPtr, itemData.get(), itemArraySize * sizeof(Item)))
		return result;

	for (DWORD64 i = 0; i < itemArraySize; i++)
	{
		if (!Utils::Valid(itemData[i].referencePtr))
			continue;
		if (itemData[i].favoriteIndex > 12)
			continue;

		TesItem referenceData{};
		if (!Rpm(itemData[i].referencePtr, &referenceData, sizeof referenceData))
			continue;
		if (referenceData.formType != static_cast<BYTE>(FormTypes::TesObjectWeap))
			continue;

		auto weaponName = GetInstancedItemName(itemData[i].displayPtr);
		if (weaponName.empty())
		{
			weaponName = GetEntityName(referenceData.namePtr0098);
			if (weaponName.empty())
				continue;
		}

		result[itemData[i].favoriteIndex + 1] = fmt::format("[{}] {}", FavoriteIndex2Slot(itemData[i].favoriteIndex), weaponName);
	}

	return result;
}

std::string ErectusMemory::GetFavoritedWeaponText(const BYTE index)
{
	std::string result = {};

	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return result;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return result;
	if (!Utils::Valid(localPlayer.inventoryPtr))
		return result;

	Inventory inventoryData{};
	if (!Rpm(localPlayer.inventoryPtr, &inventoryData, sizeof inventoryData))
		return result;
	if (!Utils::Valid(inventoryData.itemArrayPtr) || inventoryData.itemArrayEnd < inventoryData.itemArrayPtr)
		return result;

	const auto itemArraySize = (inventoryData.itemArrayEnd - inventoryData.itemArrayPtr) / sizeof(Item);
	if (!itemArraySize || itemArraySize > 0x7FFF)
		return result;

	auto itemData = std::make_unique<Item[]>(itemArraySize);
	if (!Rpm(inventoryData.itemArrayPtr, itemData.get(), itemArraySize * sizeof(Item)))
		return result;

	for (DWORD64 i = 0; i < itemArraySize; i++)
	{
		if (!Utils::Valid(itemData[i].referencePtr))
			continue;
		if (itemData[i].favoriteIndex != index)
			continue;

		TesItem referenceData{};
		if (!Rpm(itemData[i].referencePtr, &referenceData, sizeof referenceData))
			break;
		if (referenceData.formType != static_cast<BYTE>(FormTypes::TesObjectWeap))
			break;

		auto tempWeaponName = GetInstancedItemName(itemData[i].displayPtr);
		if (tempWeaponName.empty())
		{
			tempWeaponName = GetEntityName(referenceData.namePtr0098);
			if (tempWeaponName.empty())
				continue;
		}

		result = fmt::format("[{0}] {1}", FavoriteIndex2Slot(itemData[i].favoriteIndex), tempWeaponName);
		return result;
	}
	return result;
}

bool ErectusMemory::EntityInventoryValid(const TesObjectRefr& entityData)
{
	if (!Utils::Valid(entityData.inventoryPtr))
		return false;

	Inventory inventoryData{};
	if (!Rpm(entityData.inventoryPtr, &inventoryData, sizeof inventoryData))
		return false;
	if (!Utils::Valid(inventoryData.itemArrayPtr) || inventoryData.itemArrayEnd < inventoryData.itemArrayPtr)
		return false;

	const auto itemArraySize = (inventoryData.itemArrayEnd - inventoryData.itemArrayPtr) / sizeof(Item);
	if (!itemArraySize || itemArraySize > 0x7FFF)
		return false;

	auto* itemData = new Item[itemArraySize];
	if (!Rpm(inventoryData.itemArrayPtr, &*itemData, itemArraySize * sizeof(Item)))
	{
		delete[]itemData;
		itemData = nullptr;
		return false;
	}

	for (DWORD64 i = 0; i < itemArraySize; i++)
	{
		if (!Utils::Valid(itemData[i].referencePtr))
			continue;

		TesItem referenceData{};
		if (!Rpm(itemData[i].referencePtr, &referenceData, sizeof referenceData))
			continue;
		if (referenceData.recordFlagA >> 2 & 1)
			continue;

		delete[]itemData;
		itemData = nullptr;
		return true;
	}

	delete[]itemData;
	itemData = nullptr;
	return false;
}

bool ErectusMemory::AllowLegendaryWeapons(const EntityLooterSettings& settings)
{
	if (!settings.allWeaponsEnabled)
	{
		if (settings.oneStarWeaponsEnabled)
			return true;
		if (settings.twoStarWeaponsEnabled)
			return true;
		if (settings.threeStarWeaponsEnabled)
			return true;
	}

	return false;
}

bool ErectusMemory::AllowLegendaryArmor(const EntityLooterSettings& settings)
{
	if (settings.allArmorEnabled || settings.oneStarArmorEnabled || settings.twoStarArmorEnabled || settings.threeStarArmorEnabled)
		return true;

	return false;
}

bool ErectusMemory::CheckEntityLooterItem(const DWORD formId, const DWORD64 entityFlag, const EntityLooterSettings& settings, const bool legendaryWeaponsEnabled, const bool legendaryArmorEnabled)
{
	if (settings.capsEnabled && formId == 0x0000000F)
		return true;

	if (settings.listEnabled)
	{
		if (CheckFormIdArray(formId, settings.enabledList, settings.formIdList, 100))
			return true;
	}

	if (entityFlag & CUSTOM_ENTRY_WEAPON)
	{
		if (settings.allWeaponsEnabled)
			return true;

		return legendaryWeaponsEnabled;
	}
	if (entityFlag & CUSTOM_ENTRY_ARMOR)
	{
		if (settings.allArmorEnabled)
			return true;

		return legendaryArmorEnabled;
	}
	if (entityFlag & CUSTOM_ENTRY_AMMO)
		return settings.ammoEnabled;
	if (entityFlag & CUSTOM_ENTRY_MOD)
		return settings.modsEnabled;
	if (entityFlag & CUSTOM_ENTRY_JUNK)
		return settings.junkEnabled;
	if (entityFlag & CUSTOM_ENTRY_AID)
		return settings.aidEnabled;
	if (entityFlag & CUSTOM_ENTRY_TREASURE_MAP)
		return settings.treasureMapsEnabled;
	if (entityFlag & CUSTOM_ENTRY_PLAN)
	{
		if (entityFlag & CUSTOM_ENTRY_KNOWN_RECIPE)
			return settings.knownPlansEnabled;
		if (entityFlag & CUSTOM_ENTRY_UNKNOWN_RECIPE)
			return settings.unknownPlansEnabled;
	}
	else if (entityFlag & CUSTOM_ENTRY_MISC)
		return settings.miscEnabled;

	if (settings.unlistedEnabled)
		return true;

	return false;
}

bool ErectusMemory::IsLegendaryFormId(const DWORD formId)
{
	for (const auto& i : legendaryFormIdArray)
	{
		if (formId == i)
			return true;
	}

	return false;
}

BYTE ErectusMemory::GetLegendaryRank(const DWORD64 displayPtr)
{
	if (!Utils::Valid(displayPtr))
		return 0;

	DWORD64 instancedArrayPtr;
	if (!Rpm(displayPtr, &instancedArrayPtr, sizeof instancedArrayPtr))
		return 0;
	if (!Utils::Valid(instancedArrayPtr))
		return 0;

	ItemInstancedArray itemInstancedArrayData{};
	if (!Rpm(instancedArrayPtr, &itemInstancedArrayData, sizeof itemInstancedArrayData))
		return 0;
	if (!Utils::Valid(itemInstancedArrayData.arrayPtr) || itemInstancedArrayData.arrayEnd < itemInstancedArrayData.arrayPtr)
		return 0;

	const auto instancedArraySize = (itemInstancedArrayData.arrayEnd - itemInstancedArrayData.arrayPtr) / sizeof(DWORD64);
	if (!instancedArraySize || instancedArraySize > 0x7FFF)
		return 0;

	auto* instancedArray = new DWORD64[instancedArraySize];
	if (!Rpm(itemInstancedArrayData.arrayPtr, &*instancedArray, instancedArraySize * sizeof(DWORD64)))
	{
		delete[]instancedArray;
		instancedArray = nullptr;
		return 0;
	}

	DWORD64 objectInstanceExtraPtr = 0;

	for (DWORD64 i = 0; i < instancedArraySize; i++)
	{
		if (!Utils::Valid(instancedArray[i]))
			continue;

		ExtraTextDisplayData extraTextDisplayDataData{};
		if (!Rpm(instancedArray[i], &extraTextDisplayDataData, sizeof extraTextDisplayDataData))
			continue;

		const auto rttiNamePtr = RttiGetNamePtr(extraTextDisplayDataData.vtable);
		if (!rttiNamePtr)
			continue;

		char rttiNameCheck[sizeof".?AVBGSObjectInstanceExtra@@"];
		if (!Rpm(rttiNamePtr, &rttiNameCheck, sizeof rttiNameCheck))
			continue;
		if (strcmp(rttiNameCheck, ".?AVBGSObjectInstanceExtra@@") != 0)
			continue;

		objectInstanceExtraPtr = instancedArray[i];
		break;
	}

	delete[]instancedArray;
	instancedArray = nullptr;

	if (!objectInstanceExtraPtr)
		return 0;

	ObjectInstanceExtra objectInstanceExtraData{};
	if (!Rpm(objectInstanceExtraPtr, &objectInstanceExtraData, sizeof objectInstanceExtraData))
		return 0;
	if (!Utils::Valid(objectInstanceExtraData.modDataPtr))
		return 0;

	ModInstance modInstanceData{};
	if (!Rpm(objectInstanceExtraData.modDataPtr, &modInstanceData, sizeof modInstanceData))
		return 0;
	if (!Utils::Valid(modInstanceData.modListPtr) || !modInstanceData.modListSize)
		return 0;

	const DWORD64 modArraySize = modInstanceData.modListSize / 0x8;
	if (!modArraySize || modArraySize > 0x7FFF)
		return 0;

	auto* modArray = new DWORD[modArraySize * 2];
	if (!Rpm(modInstanceData.modListPtr, &*modArray, modArraySize * 2 * sizeof(DWORD)))
	{
		delete[]modArray;
		return 0;
	}

	BYTE legendaryRank = 0;
	for (DWORD64 i = 0; i < modArraySize; i++)
	{
		if (IsLegendaryFormId(modArray[i * 2]))
		{
			legendaryRank++;
		}
	}

	delete[]modArray;
	return legendaryRank;
}

bool ErectusMemory::ValidLegendary(const BYTE legendaryRank, const DWORD64 entityFlag, const EntityLooterSettings& customEntityLooterSettings, const bool legendaryWeaponsEnabled, const bool legendaryArmorEnabled)
{
	if (entityFlag & CUSTOM_ENTRY_WEAPON)
	{
		if (legendaryWeaponsEnabled)
		{
			switch (legendaryRank)
			{
			case 0x01:
				return customEntityLooterSettings.oneStarWeaponsEnabled;
			case 0x02:
				return customEntityLooterSettings.twoStarWeaponsEnabled;
			case 0x03:
				return customEntityLooterSettings.threeStarWeaponsEnabled;
			default:
				return customEntityLooterSettings.allWeaponsEnabled;
			}
		}
	}
	else if (entityFlag & CUSTOM_ENTRY_ARMOR)
	{
		if (legendaryArmorEnabled)
		{
			switch (legendaryRank)
			{
			case 0x01:
				return customEntityLooterSettings.oneStarArmorEnabled;
			case 0x02:
				return customEntityLooterSettings.twoStarArmorEnabled;
			case 0x03:
				return customEntityLooterSettings.threeStarArmorEnabled;
			default:
				return customEntityLooterSettings.allArmorEnabled;
			}
		}
	}

	return false;
}

bool ErectusMemory::TransferEntityItems(const TesObjectRefr& entityData, const TesItem& referenceData, const TesObjectRefr& localPlayer, const	bool onlyUseEntityLooterList, const bool useEntityLooterBlacklist)
{
	EntityLooterSettings currentEntityLooterSettings = {};
	switch (referenceData.formType)
	{
	case (static_cast<BYTE>(FormTypes::TesNpc)):
		currentEntityLooterSettings = Settings::npcLooter;
		break;
	case (static_cast<BYTE>(FormTypes::TesObjectCont)):
		currentEntityLooterSettings = Settings::containerLooter;
		break;
	default:
		return false;
	}

	if (!Utils::Valid(entityData.inventoryPtr))
		return false;

	Inventory inventoryData{};
	if (!Rpm(entityData.inventoryPtr, &inventoryData, sizeof inventoryData))
		return false;
	if (!Utils::Valid(inventoryData.itemArrayPtr) || inventoryData.itemArrayEnd < inventoryData.itemArrayPtr)
		return false;

	auto itemArraySize = (inventoryData.itemArrayEnd - inventoryData.itemArrayPtr) / sizeof(Item);
	if (!itemArraySize || itemArraySize > 0x7FFF)
		return false;

	auto* itemData = new Item[itemArraySize];
	if (!Rpm(inventoryData.itemArrayPtr, &*itemData, itemArraySize * sizeof(Item)))
	{
		delete[]itemData;
		itemData = nullptr;
		return false;
	}

	auto legendaryWeaponsEnabled = AllowLegendaryWeapons(currentEntityLooterSettings);
	auto legendaryArmorEnabled = AllowLegendaryArmor(currentEntityLooterSettings);

	for (DWORD64 i = 0; i < itemArraySize; i++)
	{
		if (!Utils::Valid(itemData[i].referencePtr))
			continue;
		if (!Utils::Valid(itemData[i].displayPtr) || itemData[i].iterations < itemData[i].displayPtr)
			continue;

		TesItem itemReferenceData{};
		if (!Rpm(itemData[i].referencePtr, &itemReferenceData, sizeof itemReferenceData))
			continue;
		if (itemReferenceData.recordFlagA >> 2 & 1)
			continue;

		if (useEntityLooterBlacklist)
		{
			if (CheckFormIdArray(itemReferenceData.formId, currentEntityLooterSettings.blacklistEnabled, currentEntityLooterSettings.blacklist, 64))
				continue;
		}

		if (onlyUseEntityLooterList)
		{
			if (!CheckFormIdArray(itemReferenceData.formId, currentEntityLooterSettings.enabledList, currentEntityLooterSettings.formIdList, 100))
				continue;
		}

		auto entityFlag = CUSTOM_ENTRY_DEFAULT;
		DWORD64 entityNamePtr = 0;
		auto enabledDistance = 0;

		GetCustomEntityData(itemReferenceData, &entityFlag, &entityNamePtr, &enabledDistance, false, false);
		if (!(entityFlag & CUSTOM_ENTRY_VALID_ITEM))
			continue;

		if (!onlyUseEntityLooterList && !CheckEntityLooterItem(itemReferenceData.formId, entityFlag, currentEntityLooterSettings, legendaryWeaponsEnabled, legendaryArmorEnabled))
			continue;

		auto iterations = (itemData[i].iterations - itemData[i].displayPtr) / sizeof(ItemCount);
		if (!iterations || iterations > 0x7FFF)
			continue;

		auto* itemCountData = new ItemCount[iterations];
		if (!Rpm(itemData[i].displayPtr, &*itemCountData, iterations * sizeof(ItemCount)))
		{
			delete[]itemCountData;
			itemCountData = nullptr;
			continue;
		}

		auto count = 0;
		for (DWORD64 c = 0; c < iterations; c++)
		{
			count += itemCountData[c].count;
		}

		delete[]itemCountData;
		itemCountData = nullptr;

		if (count == 0)
			continue;

		if (entityFlag & CUSTOM_ENTRY_WEAPON)
		{
			if (legendaryWeaponsEnabled)
			{
				auto legendaryRank = GetLegendaryRank(itemData[i].displayPtr);
				if (!ValidLegendary(legendaryRank, entityFlag, currentEntityLooterSettings, legendaryWeaponsEnabled, legendaryArmorEnabled))
					continue;
			}
		}
		else if (entityFlag & CUSTOM_ENTRY_ARMOR)
		{
			if (legendaryArmorEnabled)
			{
				auto legendaryRank = GetLegendaryRank(itemData[i].displayPtr);
				if (!ValidLegendary(legendaryRank, entityFlag, currentEntityLooterSettings, legendaryWeaponsEnabled, legendaryArmorEnabled))
					continue;
			}
		}

		TransferMessage transferMessageData = {
			.vtable = ErectusProcess::exe + VTABLE_REQUESTTRANSFERITEMMSG,
			.srcFormId = entityData.formId,
			.unknownId = 0xE0001F7A,
			.dstFormId = localPlayer.formId,
			.itemId = itemData[i].itemId,
			.count = count,
			.unknownA = 0x00000000,
			.unknownB = 0x00,
			.unknownC = 0x01,
			.unknownD = 0x00,
			.unknownE = 0x02,
		};

		SendMessageToServer(&transferMessageData, sizeof transferMessageData);
	}

	delete[]itemData;
	itemData = nullptr;
	return true;
}

bool ErectusMemory::ContainerValid(const TesItem& referenceData)
{
	if (!Utils::Valid(referenceData.keywordArrayData00C0))
		return false;

	int nifTextLength;
	if (!Rpm(referenceData.keywordArrayData00C0 + 0x10, &nifTextLength, sizeof nifTextLength))
		return false;
	if (nifTextLength == 41)
	{
		char containerMarkerCheck[sizeof"ContainerMarker"];
		if (!Rpm(referenceData.keywordArrayData00C0 + 0x2E, &containerMarkerCheck, sizeof containerMarkerCheck))
			return false;

		containerMarkerCheck[15] = '\0';
		if (!strcmp(containerMarkerCheck, "ContainerMarker"))
			return false;
	}

	if (!Utils::Valid(referenceData.namePtr00B0))
		return false;

	DWORD64 nameBuffer;
	if (!Rpm(referenceData.namePtr00B0 + 0x10, &nameBuffer, sizeof nameBuffer))
		return false;
	if (!nameBuffer)
		return false;

	if (!Utils::Valid(nameBuffer))
		nameBuffer = referenceData.namePtr00B0;

	int nameTextLength;
	if (!Rpm(nameBuffer + 0x10, &nameTextLength, sizeof nameTextLength))
		return false;
	if (!nameTextLength || nameTextLength > 0x7FFF)
		return false;

	return true;
}

bool ErectusMemory::LootEntity(const TesObjectRefr& entityData, const TesItem& referenceData, const TesObjectRefr& localPlayer,
	const bool onlyUseEntityLooterList, const bool useEntityLooterBlacklist)
{
	auto isEntityNpc = false;
	auto isEntityContainer = false;

	float maxDistance;
	switch (referenceData.formType)
	{
	case (static_cast<BYTE>(FormTypes::TesNpc)):
		isEntityNpc = true;
		maxDistance = 76.f;
		break;
	case (static_cast<BYTE>(FormTypes::TesObjectCont)):
		isEntityContainer = true;
		maxDistance = 6.f;
		break;
	default:
		return false;
	}

	if (isEntityNpc && (referenceData.formId == 0x00000007 || CheckHealthFlag(entityData.healthFlag) != 0x3))
		return false;

	if (Utils::GetDistance(entityData.position, localPlayer.position) * 0.01f > maxDistance)
		return false;

	if (isEntityContainer && !ContainerValid(referenceData))
		return false;

	if (!EntityInventoryValid(entityData))
		return false;

	return TransferEntityItems(entityData, referenceData, localPlayer, onlyUseEntityLooterList, useEntityLooterBlacklist);
}

bool ErectusMemory::CheckEntityLooterSettings(const EntityLooterSettings& settings)
{
	if (settings.allWeaponsEnabled)
		return true;
	if (settings.allArmorEnabled)
		return true;
	if (settings.oneStarWeaponsEnabled)
		return true;
	if (settings.oneStarArmorEnabled)
		return true;
	if (settings.twoStarWeaponsEnabled)
		return true;
	if (settings.twoStarArmorEnabled)
		return true;
	if (settings.threeStarWeaponsEnabled)
		return true;
	if (settings.threeStarArmorEnabled)
		return true;
	if (settings.ammoEnabled)
		return true;
	if (settings.modsEnabled)
		return true;
	if (settings.capsEnabled)
		return true;
	if (settings.junkEnabled)
		return true;
	if (settings.aidEnabled)
		return true;
	if (settings.treasureMapsEnabled)
		return true;
	if (settings.knownPlansEnabled)
		return true;
	if (settings.unknownPlansEnabled)
		return true;
	if (settings.miscEnabled)
		return true;
	if (settings.unlistedEnabled)
		return true;
	if (settings.listEnabled)
		return CheckEntityLooterList(settings);

	return false;
}

bool ErectusMemory::CheckOnlyUseEntityLooterList(const EntityLooterSettings& settings)
{
	if (settings.allWeaponsEnabled)
		return false;
	if (settings.allArmorEnabled)
		return false;
	if (settings.oneStarWeaponsEnabled)
		return false;
	if (settings.oneStarArmorEnabled)
		return false;
	if (settings.twoStarWeaponsEnabled)
		return false;
	if (settings.twoStarArmorEnabled)
		return false;
	if (settings.threeStarWeaponsEnabled)
		return false;
	if (settings.threeStarArmorEnabled)
		return false;
	if (settings.ammoEnabled)
		return false;
	if (settings.modsEnabled)
		return false;
	if (settings.capsEnabled)
		return false;
	if (settings.junkEnabled)
		return false;
	if (settings.aidEnabled)
		return false;
	if (settings.treasureMapsEnabled)
		return false;
	if (settings.knownPlansEnabled)
		return false;
	if (settings.unknownPlansEnabled)
		return false;
	if (settings.miscEnabled)
		return false;
	if (settings.unlistedEnabled)
		return false;
	if (settings.listEnabled)
		return CheckEntityLooterList(settings);

	return false;
}

bool ErectusMemory::HarvestFlora(const TesObjectRefr& entityData, const TesItem& referenceData, const TesObjectRefr& localPlayer)
{
	if (IsFloraHarvested(entityData.harvestFlagA, entityData.harvestFlagB))
		return false;

	auto normalDistance = static_cast<int>(Utils::GetDistance(entityData.position, localPlayer.position) * 0.01f);
	if (normalDistance > 6)
		return false;

	if (!FloraValid(referenceData))
		return false;

	RequestActivateRefMessage requestActivateRefMessageData{
		.vtable = ErectusProcess::exe + VTABLE_REQUESTACTIVATEREFMSG,
		.formId = entityData.formId,
		.choice = 0xFF,
		.forceActivate = 0
	};

	return SendMessageToServer(&requestActivateRefMessageData, sizeof requestActivateRefMessageData);
}

bool ErectusMemory::Harvester()
{
	if (!MessagePatcher(allowMessages))
		return false;

	if (!allowMessages)
		return false;

	auto useNpcLooter = Settings::npcLooter.enabled && CheckEntityLooterSettings(Settings::npcLooter);

	auto useContainerLooter = Settings::containerLooter.enabled && CheckEntityLooterSettings(Settings::containerLooter);

	auto useFloraHarvester = Settings::harvester.enabled && CheckIngredientList();

	if (!useNpcLooter && !useContainerLooter && !useFloraHarvester)
		return false;

	auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	TesObjectRefr localPlayer{};
	if (!Rpm(localPlayerPtr, &localPlayer, sizeof localPlayer))
		return false;

	auto onlyUseNpcLooterList = false;
	auto useNpcLooterBlacklist = false;
	if (useNpcLooter)
	{
		onlyUseNpcLooterList = CheckOnlyUseEntityLooterList(Settings::npcLooter);
		useNpcLooterBlacklist = CheckEntityLooterBlacklist(Settings::npcLooter);
	}

	auto onlyUseContainerLooterList = false;
	auto useContainerLooterBlacklist = false;
	if (useContainerLooter)
	{
		onlyUseContainerLooterList = CheckOnlyUseEntityLooterList(Settings::containerLooter);
		useContainerLooterBlacklist = CheckEntityLooterBlacklist(Settings::containerLooter);
	}

	if (useNpcLooter)
	{
		auto temporaryNpcList = GetEntityPtrList();
		if (temporaryNpcList.empty())
			return false;

		for (const auto& npcPtr : temporaryNpcList)
		{
			if (!Utils::Valid(npcPtr))
				continue;
			if (npcPtr == localPlayerPtr)
				continue;

			TesObjectRefr entityData{};
			if (!Rpm(npcPtr, &entityData, sizeof entityData))
				continue;

			if (entityData.formType != static_cast<BYTE>(FormTypes::TesActor)) //FIXME: check if correct
				continue;

			if (!Utils::Valid(entityData.referencedItemPtr))
				continue;

			if (entityData.spawnFlag != 0x02)
				continue;

			TesItem referenceData{};
			if (!Rpm(entityData.referencedItemPtr, &referenceData, sizeof referenceData))
				continue;
			if (referenceData.formType != static_cast<BYTE>(FormTypes::TesNpc))
				continue;
			if (referenceData.formId == 0x00000007)
				continue;

			LootEntity(entityData, referenceData, localPlayer, onlyUseNpcLooterList, useNpcLooterBlacklist);
		}
	}

	if (useContainerLooter || useFloraHarvester)
	{
		auto temporaryEntityList = GetEntityPtrList();
		if (temporaryEntityList.empty())
			return false;

		for (const auto& entityPtr : temporaryEntityList)
		{
			if (!Utils::Valid(entityPtr))
				continue;
			if (entityPtr == localPlayerPtr)
				continue;

			TesObjectRefr entityData{};
			if (!Rpm(entityPtr, &entityData, sizeof entityData))
				continue;
			if (!Utils::Valid(entityData.referencedItemPtr))
				continue;

			if (entityData.spawnFlag != 0x02)
				continue;

			TesItem referenceData{};
			if (!Rpm(entityData.referencedItemPtr, &referenceData, sizeof referenceData))
				continue;

			if (referenceData.formType == static_cast<byte>(FormTypes::TesObjectCont))
			{
				if (useContainerLooter)
					LootEntity(entityData, referenceData, localPlayer, onlyUseContainerLooterList, useContainerLooterBlacklist);
			}
			else if (referenceData.formType == static_cast<byte>(FormTypes::TesFlora))
			{
				if (useFloraHarvester)
					HarvestFlora(entityData, referenceData, localPlayer);
			}
		}
	}

	return true;
}

bool ErectusMemory::MeleeAttack()
{
	if (!MessagePatcher(allowMessages))
		return false;

	if (!allowMessages)
		return false;

	const auto localPlayerPtr = GetLocalPlayerPtr(true);
	if (!Utils::Valid(localPlayerPtr))
		return false;

	const auto allocAddress = AllocEx(sizeof(ExternalFunction));
	if (allocAddress == 0)
		return false;

	ExternalFunction externalFunctionData;
	externalFunctionData.address = ErectusProcess::exe + OFFSET_MELEE_ATTACK;
	externalFunctionData.rcx = localPlayerPtr;
	externalFunctionData.rdx = 0;
	externalFunctionData.r8 = 1;
	externalFunctionData.r9 = 0;

	const auto written = Wpm(allocAddress, &externalFunctionData, sizeof(ExternalFunction));

	if (!written)
	{
		FreeEx(allocAddress);
		return false;
	}

	const auto paramAddress = allocAddress + sizeof ExternalFunction::ASM;
	auto* const thread = CreateRemoteThread(ErectusProcess::handle, nullptr, 0, LPTHREAD_START_ROUTINE(allocAddress),
		LPVOID(paramAddress), 0, nullptr);

	if (thread == nullptr)
	{
		FreeEx(allocAddress);
		return false;
	}

	const auto threadResult = WaitForSingleObject(thread, 3000);
	CloseHandle(thread);

	if (threadResult == WAIT_TIMEOUT)
		return false;

	FreeEx(allocAddress);
	return true;
}

bool ErectusMemory::ChargenEditing()
{
	if (!Settings::characterEditor.enabled)
		return false;

	DWORD64 chargenPtr;
	if (!Rpm(ErectusProcess::exe + OFFSET_CHARGEN, &chargenPtr, sizeof chargenPtr))
		return false;
	if (!Utils::Valid(chargenPtr))
		return false;

	Chargen chargenData{};
	if (!Rpm(chargenPtr, &chargenData, sizeof chargenData))
		return false;

	auto shouldEdit = false;

	if (chargenData.thin != Settings::characterEditor.thin)
	{
		chargenData.thin = Settings::characterEditor.thin;
		shouldEdit = true;
	}

	if (chargenData.muscular != Settings::characterEditor.muscular)
	{
		chargenData.muscular = Settings::characterEditor.muscular;
		shouldEdit = true;
	}

	if (chargenData.large != Settings::characterEditor.large)
	{
		chargenData.large = Settings::characterEditor.large;
		shouldEdit = true;
	}

	if (shouldEdit)
		return Wpm(chargenPtr, &chargenData, sizeof chargenData);

	return true;
}

Camera ErectusMemory::GetCameraInfo()
{
	Camera result = {};
	const auto cameraPtr = GetCameraPtr();
	if (!Utils::Valid(cameraPtr))
		return result;

	if (!Rpm(cameraPtr, &result, sizeof result))
		return result;

	return result;
}

bool ErectusMemory::Rpm(const DWORD64 src, void* dst, const size_t size)
{
	return ReadProcessMemory(ErectusProcess::handle, reinterpret_cast<void*>(src), dst, size, nullptr);
}

bool ErectusMemory::Wpm(const DWORD64 dst, void* src, const size_t size)
{
	return WriteProcessMemory(ErectusProcess::handle, reinterpret_cast<void*>(dst), src, size, nullptr);
}

DWORD64 ErectusMemory::AllocEx(const size_t size)
{
	return 0;

	//this needs to be split, the game scans for PAGE_EXECUTE_READWRITE regions
	//1) alloc with PAGE_READWRITE
	//2) write the data
	//3) switch to PAGE_EXECUTE_READ
	//4) create the remote thread
	//see https://reverseengineering.stackexchange.com/questions/3482/does-code-injected-into-process-memory-always-belong-to-a-page-with-rwx-access
	//return DWORD64(VirtualAllocEx(ErectusProcess::handle, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
}

bool ErectusMemory::FreeEx(const DWORD64 src)
{
	return VirtualFreeEx(ErectusProcess::handle, LPVOID(src), 0, MEM_RELEASE);
}

bool ErectusMemory::VtableSwap(const DWORD64 dst, DWORD64 src)
{
	DWORD oldProtect;
	if (!VirtualProtectEx(ErectusProcess::handle, reinterpret_cast<void*>(dst), sizeof(DWORD64), PAGE_READWRITE, &oldProtect))
		return false;

	const auto result = Wpm(dst, &src, sizeof src);

	DWORD buffer;
	if (!VirtualProtectEx(ErectusProcess::handle, reinterpret_cast<void*>(dst), sizeof(DWORD64), oldProtect, &buffer))
		return false;

	return result;
}