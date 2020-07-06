#include "gui.h"

#include "app.h"
#include "common.h"
#include "settings.h"
#include "renderer.h"

#include "ErectusProcess.h"
#include "ErectusMemory.h"
#include "threads.h"
#include "utils.h"

#include "fmt/format.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_stdlib.h"


void Gui::Render()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ProcessMenu();
	OverlayMenu();
	RenderOverlay();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void Gui::RenderOverlay()
{
	if (!App::overlayActive)
		return;

	Renderer::d3DxSprite->Begin(D3DXSPRITE_ALPHABLEND);

	RenderEntities();
	RenderPlayers();

	RenderInfoBox();

	Renderer::d3DxSprite->End();
}

void Gui::RenderEntities()
{
	auto entities = ErectusMemory::entityDataBuffer;
	for (const auto& entity : entities)
	{
		if (entity.flag & CUSTOM_ENTRY_ENTITY)
			RenderItems(entity, Settings::entitySettings);
		else if (entity.flag & CUSTOM_ENTRY_JUNK)
			RenderItems(entity, Settings::junkSettings);
		else if (entity.flag & CUSTOM_ENTRY_ITEM)
			RenderItems(entity, Settings::itemSettings);
		else if (entity.flag & CUSTOM_ENTRY_CONTAINER)
			RenderItems(entity, Settings::containerSettings);
		else if (entity.flag & CUSTOM_ENTRY_PLAN)
			RenderItems(entity, Settings::planSettings);
		else if (entity.flag & CUSTOM_ENTRY_MAGAZINE)
			RenderItems(entity, Settings::magazineSettings);
		else if (entity.flag & CUSTOM_ENTRY_BOBBLEHEAD)
			RenderItems(entity, Settings::bobbleheadSettings);
		else if (entity.flag & CUSTOM_ENTRY_FLORA)
			RenderItems(entity, Settings::floraSettings);
		else if (entity.flag & CUSTOM_ENTRY_NPC)
			RenderActors(entity, Settings::npcSettings);
	}
}

void Gui::RenderPlayers()
{
	auto players = ErectusMemory::playerDataBuffer;
	for (const auto& player : players) {
		if (player.flag & CUSTOM_ENTRY_PLAYER)
			RenderActors(player, Settings::playerSettings);
	}
}

void Gui::RenderActors(const CustomEntry& entry, const OverlaySettingsA& settings)
{
	auto health = -1;
	BYTE epicRank = 0;
	auto allowNpc = false;
	if (entry.flag & CUSTOM_ENTRY_NPC)
	{
		TesObjectRefr npcData{};
		if (!ErectusMemory::Rpm(entry.entityPtr, &npcData, sizeof npcData))
			return;

		ActorSnapshotComponent actorSnapshotComponentData{};
		if (ErectusMemory::GetActorSnapshotComponentData(npcData, &actorSnapshotComponentData))
		{
			health = static_cast<int>(actorSnapshotComponentData.maxHealth + actorSnapshotComponentData.modifiedHealth + actorSnapshotComponentData.lostHealth);
			epicRank = actorSnapshotComponentData.epicRank;
			if (epicRank)
			{
				switch (ErectusMemory::CheckHealthFlag(npcData.healthFlag))
				{
				case 0x01: //Alive
				case 0x02: //Downed
				case 0x03: //Dead
					switch (epicRank)
					{
					case 1:
						allowNpc = Settings::customLegendarySettings.overrideLivingOneStar;
						break;
					case 2:
						allowNpc = Settings::customLegendarySettings.overrideLivingTwoStar;
						break;
					case 3:
						allowNpc = Settings::customLegendarySettings.overrideLivingThreeStar;
						break;
					default:
						break;
					}
				default:
					break;
				}
			}
		}
	}

	if (!settings.enabled && !allowNpc)
		return;

	if (!settings.drawEnabled && !settings.drawDisabled)
		return;

	if (settings.enabledAlpha == 0.0f && settings.disabledAlpha == 0.0f)
		return;

	if (!settings.drawNamed && !settings.drawUnnamed)
		return;

	TesObjectRefr entityData{};
	if (!ErectusMemory::Rpm(entry.entityPtr, &entityData, sizeof entityData))
		return;

	if (entry.flag & CUSTOM_ENTRY_PLAYER)
	{
		ActorSnapshotComponent actorSnapshotComponentData{};
		if (ErectusMemory::GetActorSnapshotComponentData(entityData, &actorSnapshotComponentData))
			health = static_cast<int>(actorSnapshotComponentData.maxHealth + actorSnapshotComponentData.modifiedHealth + actorSnapshotComponentData.lostHealth);
	}

	if (entry.flag & CUSTOM_ENTRY_UNNAMED)
	{
		if (!settings.drawUnnamed)
			return;
	}
	else
	{
		if (!settings.drawNamed)
			return;
	}

	auto alpha = 0.f;

	if (entityData.spawnFlag == 0x02)
	{
		if (settings.drawEnabled)
			alpha = settings.enabledAlpha;
	}
	else
	{
		if (settings.drawDisabled)
			alpha = settings.disabledAlpha;
	}

	if (alpha == 0.f)
		return;

	auto showHealthText = false;

	const float* color = nullptr;

	auto legendaryAlpha = 1.0f;

	switch (ErectusMemory::CheckHealthFlag(entityData.healthFlag))
	{
	case 0x01: //Alive
		showHealthText = settings.showHealth;
		if (allowNpc)
		{
			switch (epicRank)
			{
			case 1:
				color = Settings::customLegendarySettings.livingOneStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			case 2:
				color = Settings::customLegendarySettings.livingTwoStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			case 3:
				color = Settings::customLegendarySettings.livingThreeStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			default:
				break;
			}
		}
		else if (settings.drawAlive)
			color = settings.aliveColor;
		break;
	case 0x02: //Downed
		showHealthText = settings.showHealth;
		if (allowNpc)
		{
			switch (epicRank)
			{
			case 1:
				color = Settings::customLegendarySettings.livingOneStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			case 2:
				color = Settings::customLegendarySettings.livingTwoStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			case 3:
				color = Settings::customLegendarySettings.livingThreeStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			default:
				break;
			}
		}
		else if (settings.drawDowned)
			color = settings.downedColor;
		break;
	case 0x03: //Dead
		showHealthText = settings.showDeadHealth;
		if (allowNpc)
		{
			switch (epicRank)
			{
			case 1:
				color = Settings::customLegendarySettings.deadOneStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			case 2:
				color = Settings::customLegendarySettings.deadTwoStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			case 3:
				color = Settings::customLegendarySettings.deadThreeStarColor;
				if (entityData.spawnFlag == 0x02)
					alpha = legendaryAlpha;
				break;
			default:
				break;
			}
		}
		else if (settings.drawDead)
			color = settings.deadColor;
		break;
	default: //Unknown
		showHealthText = settings.showHealth;
		if (settings.drawUnknown)
			color = settings.unknownColor;
		break;
	}

	if (color == nullptr)
		return;

	auto cameraData = ErectusMemory::GetCameraInfo();
	auto distance = Utils::GetDistance(entityData.position, cameraData.origin);
	auto normalDistance = static_cast<int>(distance * 0.01f);
	if (normalDistance > settings.enabledDistance)
		return;

	if (entry.entityPtr == ErectusMemory::targetLockingPtr)
		color = Settings::targetting.lockingColor;

	float screen[2] = { 0.0f, 0.0f };
	if (!Utils::WorldToScreen(cameraData.view, entityData.position, screen))
		return;

	std::string itemText;
	if (settings.showName && showHealthText && settings.showDistance) //Name, Health, Distance
		itemText = fmt::format("{0}\n{1:d} 生命值 [{2:d} 米]", entry.name, health, normalDistance);
	else if (settings.showName && showHealthText && !settings.showDistance) //Name, Health
		itemText = fmt::format("{0}\n{1:d} 生命值", entry.name, health);
	else if (settings.showName && !showHealthText && settings.showDistance) //Name, Distance
		itemText = fmt::format("{0}\n[{1:d} 米]", entry.name, normalDistance);
	else if (!settings.showName && showHealthText && settings.showDistance) //Health, Distance
		itemText = fmt::format("{0:d} 生命值 [{1:d} 米]", health, normalDistance);
	else if (settings.showName && !showHealthText && !settings.showDistance) //Name
		itemText = entry.name;
	else if (!settings.showName && showHealthText && !settings.showDistance) //Health
		itemText = fmt::format("{:d} 生命值", health);
	else if (!settings.showName && !showHealthText && settings.showDistance) //Distance
		itemText = fmt::format("[{:d} 米]", normalDistance);

	if (!itemText.empty())
	{
		if (Settings::utilities.debugEsp)
			itemText = fmt::format("{0:16x}\n{1:08x}\n{2:16x}\n{3:08x}", entry.entityPtr, entry.entityFormId, entry.referencePtr, entry.referenceFormId);

		Renderer::DrawTextA(itemText.c_str(), settings.textShadowed, settings.textCentered, screen, color, alpha);
	}
}

void Gui::RenderItems(const CustomEntry& entry, const OverlaySettingsB& settings)
{
	if (!(entry.flag & CUSTOM_ENTRY_WHITELISTED) && !settings.enabled)
		return;

	if (!settings.drawEnabled && !settings.drawDisabled)
		return;

	if (settings.enabledAlpha == 0.0f && settings.disabledAlpha == 0.0f)
		return;

	if (!settings.drawNamed && !settings.drawUnnamed)
		return;

	TesObjectRefr entityData{};
	if (!ErectusMemory::Rpm(entry.entityPtr, &entityData, sizeof entityData))
		return;

	if (entry.flag & CUSTOM_ENTRY_UNNAMED)
	{
		if (!settings.drawUnnamed)
			return;
	}
	else if (!settings.drawNamed)
		return;

	if (entry.flag & CUSTOM_ENTRY_PLAN)
	{
		if (!Settings::recipes.knownRecipesEnabled && !Settings::recipes.unknownRecipesEnabled)
			return;

		if (!Settings::recipes.knownRecipesEnabled && entry.flag & CUSTOM_ENTRY_KNOWN_RECIPE)
			return;
		if (!Settings::recipes.unknownRecipesEnabled && entry.flag & CUSTOM_ENTRY_UNKNOWN_RECIPE)
			return;
	}

	auto alpha = 0.f;

	if (entityData.spawnFlag == 0x02)
	{
		if (settings.drawEnabled)
		{
			if (entry.flag & CUSTOM_ENTRY_FLORA)
			{
				if (!ErectusMemory::IsFloraHarvested(entityData.harvestFlagA, entityData.harvestFlagB))
					alpha = settings.enabledAlpha;
				else if (settings.drawDisabled)
					alpha = settings.disabledAlpha;
			}
			else
				alpha = settings.enabledAlpha;
		}
	}
	else
	{
		if (settings.drawDisabled)
			alpha = settings.disabledAlpha;
	}

	if (alpha == 0.f)
		return;

	auto cameraData = ErectusMemory::GetCameraInfo();

	const auto distance = Utils::GetDistance(entityData.position, cameraData.origin);
	const auto normalDistance = static_cast<int>(distance * 0.01f);
	if (normalDistance > settings.enabledDistance)
		return;

	float screen[2] = { 0.0f, 0.0f };
	if (!Utils::WorldToScreen(cameraData.view, entityData.position, screen))
		return;

	std::string itemText{};
	if (settings.showName && settings.showDistance)
		itemText = fmt::format("{0}\n[{1:d} 米]", entry.name, normalDistance);
	else if (settings.showName && !settings.showDistance)
		itemText = entry.name;
	else if (!settings.showName && settings.showDistance)
		itemText = fmt::format("[{0:d} 米]", normalDistance);

	if (!itemText.empty())
	{
		if (Settings::utilities.debugEsp)
			itemText = fmt::format("{0:16x}\n{1:08x}\n{2:16x}\n{3:08x}", entry.entityPtr, entry.entityFormId, entry.referencePtr, entry.referenceFormId);

		Renderer::DrawTextA(itemText.c_str(), settings.textShadowed, settings.textCentered, screen, settings.color, alpha);
	}
}

void Gui::RenderInfoBox()
{
	std::vector<std::pair<std::string, bool>> infoTexts = {};

	std::string featureText = {};
	auto featureState = false;

	float enabledTextColor[3] = { 0.0f, 1.0f, 0.0f };
	float disabledTextColor[3] = { 1.0f, 0.0f, 0.0f };

	if (Settings::utilities.debugPlayer) {
		auto localPlayer = ErectusMemory::GetLocalPlayerInfo();

		featureText = fmt::format("玩家FormId: {:08x}", localPlayer.formId);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("储物箱FormId: {:08x}", localPlayer.stashFormId);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("区块FormId: {:08x}", localPlayer.cellFormId);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("X: {:f}", localPlayer.position[0]);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("Y: {:f}", localPlayer.position[1]);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("Z: {:f}", localPlayer.position[2]);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("偏转: {:f}", localPlayer.yaw);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("倾斜: {:f}", localPlayer.pitch);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("生命值: {:f}", localPlayer.currentHealth);
		infoTexts.emplace_back(featureText, true);
	}

	if (Settings::infobox.drawScrapLooterStatus)
	{
		featureText = fmt::format("材料搜刮 (自动): {:d}", static_cast<int>(Settings::scrapLooter.autoLootingEnabled));
		featureState = Settings::scrapLooter.autoLootingEnabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawItemLooterStatus)
	{
		featureText = fmt::format("物品搜刮 (自动): {:d}", static_cast<int>(Settings::itemLooter.autoLootingEnabled));
		featureState = Settings::itemLooter.autoLootingEnabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawNpcLooterStatus)
	{
		featureText = fmt::format("NPC搜刮 (76m 距离限制): {:d}", static_cast<int>(Settings::npcLooter.enabled));
		featureState = Settings::npcLooter.enabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawContainerLooterStatus)
	{
		featureText = fmt::format("容器搜刮 (6m 距离限制): {:d}", static_cast<int>(Settings::containerLooter.enabled));
		featureState = Settings::containerLooter.enabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawHarvesterStatus)
	{
		featureText = fmt::format("植物群系收获 (6m 距离限制): {:d}", static_cast<int>(Settings::harvester.enabled));
		featureState = Settings::harvester.enabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawPositionSpoofingStatus)
	{
		featureText = fmt::format("位置伪造 (激活): {0:d} (高度: {1:d})", static_cast<int>(Threads::positionSpoofingToggle), Settings::customLocalPlayerSettings.positionSpoofingHeight);
		featureState = ErectusMemory::InsideInteriorCell() ? false : Settings::customLocalPlayerSettings.positionSpoofingEnabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawNukeCodes)
	{
		featureText = format("{} - A点", fmt::join(ErectusMemory::alphaCode, " "));
		featureState = ErectusMemory::alphaCode == std::array<int, 8>{} ? false : true;
		infoTexts.emplace_back(featureText, featureState);

		featureText = format("{} - B点", fmt::join(ErectusMemory::bravoCode, " "));
		featureState = ErectusMemory::bravoCode == std::array<int, 8>{} ? false : true;
		infoTexts.emplace_back(featureText, featureState);

		featureText = format("{} - C点", fmt::join(ErectusMemory::charlieCode, " "));
		featureState = ErectusMemory::charlieCode == std::array<int, 8>{} ? false : true;
		infoTexts.emplace_back(featureText, featureState);
	}

	auto spacing = 0;
	for (const auto& item : infoTexts)
	{
		float textPosition[2] = { 0.0f, static_cast<float>(spacing) * 16.0f };
		auto* textColor = item.second ? enabledTextColor : disabledTextColor;
		Renderer::DrawTextA(item.first.c_str(), true, false, textPosition, textColor, 1.f);
		spacing++;
	}
}


void Gui::ProcessMenu()
{
	if (!ErectusProcess::processMenuActive)
		return;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(App::windowSize[0]), static_cast<float>(App::windowSize[1])));
	ImGui::SetNextWindowCollapsed(false);

	if (ImGui::Begin((const char*)u8"Erectus - 主菜单", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::MenuItem((const char*)u8"退出"))
				App::CloseWnd();
			if (ImGui::MenuItem((const char*)u8"图层菜单"))
				App::SetOverlayMenu();
			if (!ErectusProcess::pid)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

				ImGui::MenuItem((const char*)u8"图层");

				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			else
			{
				if (ImGui::MenuItem((const char*)u8"图层"))
					App::SetOverlayPosition(false, true);
			}
			ImGui::EndMenuBar();
		}

		ImGui::SetNextItemWidth(-16.f);

		auto processText = ErectusProcess::pid ? "Fallout76.exe - " + std::to_string(ErectusProcess::pid) : (const char*)u8"未选择进程.";
		if (ImGui::BeginCombo((const char*)u8"###进程列表", processText.c_str()))
		{
			for (auto item : ErectusProcess::GetProcesses())
			{
				processText = item ? "Fallout76.exe - " + std::to_string(item) : (const char*)u8"无";
				if (ImGui::Selectable(processText.c_str()))
					ErectusProcess::AttachToProcess(item);
			}

			ImGui::EndCombo();
		}

		ImGui::Separator();
		switch (ErectusProcess::processErrorId)
		{
		case 0:
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ErectusProcess::processError.c_str());
			break;
		case 1:
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ErectusProcess::processError.c_str());
			break;
		case 2:
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ErectusProcess::processError.c_str());
			break;
		default:
			ImGui::Text(ErectusProcess::processError.c_str());
			break;
		}

		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::Columns(2);
		ImGui::Separator();
		ImGui::Text((const char*)u8"图层菜单快捷键");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"CTRL+ENTER");
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"刷新率");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%.1f", ImGui::GetIO().Framerate);
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"进程ID");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%lu", ErectusProcess::pid);
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"硬件识别码");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%p", ErectusProcess::hWnd);
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"基址");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%016llX", ErectusProcess::exe);
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"句柄");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%p", ErectusProcess::handle);
		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopItemFlag();
	}
	ImGui::End();
}

void Gui::ButtonToggle(const char* label, bool* state)
{
	if (*state)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
		if (ImGui::Button(label, ImVec2(224.0f, 0.0f)))
			*state = false;
		ImGui::PopStyleColor(3);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
		if (ImGui::Button(label, ImVec2(224.0f, 0.0f)))
			*state = true;
		ImGui::PopStyleColor(3);
	}
}

void Gui::LargeButtonToggle(const char* label, bool* state)
{
	if (*state)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
		if (ImGui::Button(label, ImVec2(451.0f, 0.0f)))
			*state = false;
		ImGui::PopStyleColor(3);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
		if (ImGui::Button(label, ImVec2(451.0f, 0.0f)))
			*state = true;
		ImGui::PopStyleColor(3);
	}
}

void Gui::SmallButtonToggle(const char* label, bool* state)
{
	if (*state)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
		if (ImGui::Button(label, ImVec2(110.0f, 0.0f)))
			*state = false;
		ImGui::PopStyleColor(3);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
		if (ImGui::Button(label, ImVec2(110.0f, 0.0f)))
			*state = true;
		ImGui::PopStyleColor(3);
	}
}

void Gui::OverlayMenuTabEsp()
{
	if (ImGui::BeginTabItem((const char*)u8"透视###透视页面"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"玩家透视设置"))
		{
			ButtonToggle((const char*)u8"玩家透视已启动", &Settings::playerSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###玩家透视距离", &Settings::playerSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"绘制存活玩家", &Settings::playerSettings.drawAlive);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###存活玩家颜色", Settings::playerSettings.aliveColor);
			Utils::ValidateRgb(Settings::playerSettings.aliveColor);

			ButtonToggle((const char*)u8"绘制倒地玩家", &Settings::playerSettings.drawDowned);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###倒地玩家颜色", Settings::playerSettings.downedColor);
			Utils::ValidateRgb(Settings::playerSettings.downedColor);

			ButtonToggle((const char*)u8"绘制死亡玩家", &Settings::playerSettings.drawDead);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###死亡玩家颜色", Settings::playerSettings.deadColor);
			Utils::ValidateRgb(Settings::playerSettings.deadColor);

			ButtonToggle((const char*)u8"绘制未知玩家", &Settings::playerSettings.drawUnknown);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###未知玩家颜色", Settings::playerSettings.unknownColor);
			Utils::ValidateRgb(Settings::playerSettings.unknownColor);

			ButtonToggle((const char*)u8"绘制已启用玩家", &Settings::playerSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用玩家不透明度", &Settings::playerSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用玩家", &Settings::playerSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用玩家不透明度", &Settings::playerSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称玩家", &Settings::playerSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称玩家", &Settings::playerSettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示玩家名称", &Settings::playerSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示玩家距离", &Settings::playerSettings.showDistance);

			ButtonToggle((const char*)u8"显示玩家血量", &Settings::playerSettings.showHealth);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示死亡玩家血量", &Settings::playerSettings.showDeadHealth);

			ButtonToggle((const char*)u8"玩家信息阴影", &Settings::playerSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"玩家信息居中", &Settings::playerSettings.textCentered);
		}

		if (ImGui::CollapsingHeader((const char*)u8"NPC透视设置"))
		{
			ButtonToggle((const char*)u8"NPC透视已启用", &Settings::npcSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###NPC透视距离", &Settings::npcSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"绘制存活NPC", &Settings::npcSettings.drawAlive);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###存活NPC颜色", Settings::npcSettings.aliveColor);
			Utils::ValidateRgb(Settings::npcSettings.aliveColor);

			ButtonToggle((const char*)u8"绘制倒地NPC", &Settings::npcSettings.drawDowned);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###倒地NPC颜色", Settings::npcSettings.downedColor);
			Utils::ValidateRgb(Settings::npcSettings.downedColor);

			ButtonToggle((const char*)u8"绘制死亡NPC", &Settings::npcSettings.drawDead);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###死亡NPC颜色", Settings::npcSettings.deadColor);
			Utils::ValidateRgb(Settings::npcSettings.deadColor);

			ButtonToggle((const char*)u8"绘制未知NPC", &Settings::npcSettings.drawUnknown);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###未知NPC颜色", Settings::npcSettings.unknownColor);
			Utils::ValidateRgb(Settings::npcSettings.unknownColor);

			ButtonToggle((const char*)u8"绘制已启用NPC", &Settings::npcSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用NPC不透明度", &Settings::npcSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用NPC", &Settings::npcSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用NPC不透明度", &Settings::npcSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称NPC", &Settings::npcSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称NPC", &Settings::npcSettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示NPC名称", &Settings::npcSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示NPC距离", &Settings::npcSettings.showDistance);

			ButtonToggle((const char*)u8"显示NPC血量", &Settings::npcSettings.showHealth);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示死亡NPC血量", &Settings::npcSettings.showDeadHealth);

			ButtonToggle((const char*)u8"NPC信息阴影", &Settings::npcSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"NPC信息居中", &Settings::npcSettings.textCentered);

			ButtonToggle((const char*)u8"总是绘制一星传奇NPC", &Settings::customLegendarySettings.overrideLivingOneStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###存活一星传奇颜色", Settings::customLegendarySettings.livingOneStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.livingOneStarColor);

			ButtonToggle((const char*)u8"总是绘制死亡一星传奇NPC", &Settings::customLegendarySettings.overrideDeadOneStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###死亡一星传奇颜色", Settings::customLegendarySettings.deadOneStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.deadOneStarColor);

			ButtonToggle((const char*)u8"总是绘制二星传奇NPC", &Settings::customLegendarySettings.overrideLivingTwoStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###存活二星传奇颜色", Settings::customLegendarySettings.livingTwoStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.livingTwoStarColor);

			ButtonToggle((const char*)u8"总是绘制死亡二星传奇NPC", &Settings::customLegendarySettings.overrideDeadTwoStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###死亡二星传奇颜色", Settings::customLegendarySettings.deadTwoStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.deadTwoStarColor);

			ButtonToggle((const char*)u8"总是绘制三星传奇NPC", &Settings::customLegendarySettings.overrideLivingThreeStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###存活三星传奇颜色", Settings::customLegendarySettings.livingThreeStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.livingThreeStarColor);

			ButtonToggle((const char*)u8"总是绘制死亡三星传奇NPC", &Settings::customLegendarySettings.overrideDeadThreeStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###死亡三星传奇颜色", Settings::customLegendarySettings.deadThreeStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.deadThreeStarColor);

			LargeButtonToggle((const char*)u8"隐藏基金会NPC", &Settings::customExtraNpcSettings.hideSettlerFaction);
			LargeButtonToggle((const char*)u8"隐藏火山口NPC", &Settings::customExtraNpcSettings.hideCraterRaiderFaction);
			LargeButtonToggle((const char*)u8"隐藏死硬帮NPC", &Settings::customExtraNpcSettings.hideDieHardFaction);
			LargeButtonToggle((const char*)u8"隐藏秘密服务NPC", &Settings::customExtraNpcSettings.hideSecretServiceFaction);

			LargeButtonToggle((const char*)u8"NPC黑名单已启用", &Settings::customExtraNpcSettings.useNpcBlacklist);
			if (ImGui::CollapsingHeader((const char*)u8"NPC黑名单"))
			{
				for (auto i = 0; i < 64; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"NPC黑名单: {:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::customExtraNpcSettings.npcBlacklistEnabled[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###NPC黑名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::customExtraNpcSettings.npcBlacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"容器透视设置"))
		{
			ButtonToggle((const char*)u8"容器透视已启用", &Settings::containerSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###容器启用距离", &Settings::containerSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###容器颜色", Settings::containerSettings.color);
			Utils::ValidateRgb(Settings::containerSettings.color);

			ButtonToggle((const char*)u8"绘制已启用容器", &Settings::containerSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用容器不透明度", &Settings::containerSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用容器", &Settings::containerSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用容器不透明度", &Settings::containerSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称容器", &Settings::containerSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称容器", &Settings::containerSettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示容器名称", &Settings::containerSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示容器距离", &Settings::containerSettings.showDistance);

			ButtonToggle((const char*)u8"容器信息阴影", &Settings::containerSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"容器信息居中", &Settings::containerSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"容器白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"容器白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::containerSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###容器白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::containerSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"垃圾透视设置"))
		{
			ButtonToggle((const char*)u8"垃圾透视已启用", &Settings::junkSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###垃圾透视距离", &Settings::junkSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###垃圾颜色", Settings::junkSettings.color);
			Utils::ValidateRgb(Settings::junkSettings.color);

			ButtonToggle((const char*)u8"绘制已启用垃圾", &Settings::junkSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用垃圾不透明度", &Settings::junkSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			Utils::ValidateFloat(Settings::junkSettings.enabledAlpha, 0.0f, 1.0f);

			ButtonToggle((const char*)u8"绘制已禁用垃圾", &Settings::junkSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用垃圾不透明度", &Settings::junkSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称垃圾", &Settings::junkSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称垃圾", &Settings::junkSettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示垃圾名称", &Settings::junkSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示垃圾距离", &Settings::junkSettings.showDistance);

			ButtonToggle((const char*)u8"垃圾信息阴影", &Settings::junkSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"垃圾信息居中", &Settings::junkSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"垃圾白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"垃圾白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::junkSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###垃圾白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::junkSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"图纸透视设置"))
		{
			ButtonToggle((const char*)u8"图纸透视已启用", &Settings::planSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###图纸透视距离", &Settings::planSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###图纸颜色", Settings::planSettings.color);
			Utils::ValidateRgb(Settings::planSettings.color);

			ButtonToggle((const char*)u8"绘制已启用图纸", &Settings::planSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用图纸不透明度", &Settings::planSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用图纸", &Settings::planSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用图纸不透明度", &Settings::planSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已知图纸", &Settings::recipes.knownRecipesEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制未知图纸", &Settings::recipes.unknownRecipesEnabled);

			ButtonToggle((const char*)u8"绘制有名称图纸", &Settings::planSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称图纸", &Settings::planSettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示图纸名称", &Settings::planSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示图纸距离", &Settings::planSettings.showDistance);

			ButtonToggle((const char*)u8"图纸信息阴影", &Settings::planSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"图纸信息居中", &Settings::planSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"图纸白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"图纸白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::planSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###图纸白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::planSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"杂志透视设置"))
		{
			ButtonToggle((const char*)u8"杂志透视已启用", &Settings::magazineSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###杂志透视距离", &Settings::magazineSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###杂志颜色", Settings::magazineSettings.color);
			Utils::ValidateRgb(Settings::magazineSettings.color);

			ButtonToggle((const char*)u8"绘制已启用杂志", &Settings::magazineSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用杂志不透明度", &Settings::magazineSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用杂志", &Settings::magazineSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用杂志不透明度", &Settings::magazineSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称杂志", &Settings::magazineSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称杂志", &Settings::magazineSettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示杂志名称", &Settings::magazineSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示杂志距离", &Settings::magazineSettings.showDistance);

			ButtonToggle((const char*)u8"杂志信息阴影", &Settings::magazineSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"杂志信息居中", &Settings::magazineSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"杂志白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"杂志白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::magazineSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###杂志白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::magazineSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"娃娃透视设置"))
		{
			ButtonToggle((const char*)u8"娃娃透视已启用", &Settings::bobbleheadSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###娃娃透视距离", &Settings::bobbleheadSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###娃娃颜色", Settings::bobbleheadSettings.color);
			Utils::ValidateRgb(Settings::bobbleheadSettings.color);

			ButtonToggle((const char*)u8"绘制已启用娃娃", &Settings::bobbleheadSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用娃娃不透明度", &Settings::bobbleheadSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用娃娃", &Settings::bobbleheadSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用娃娃不透明度", &Settings::bobbleheadSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称娃娃", &Settings::bobbleheadSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称娃娃", &Settings::bobbleheadSettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示娃娃名称", &Settings::bobbleheadSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示娃娃距离", &Settings::bobbleheadSettings.showDistance);

			ButtonToggle((const char*)u8"娃娃信息阴影", &Settings::bobbleheadSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"娃娃信息居中", &Settings::bobbleheadSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"娃娃白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"娃娃白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::bobbleheadSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###娃娃白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::bobbleheadSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"物品透视设置"))
		{
			ButtonToggle((const char*)u8"物品透视已启用", &Settings::itemSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###物品透视距离", &Settings::itemSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###物品颜色", Settings::itemSettings.color);
			Utils::ValidateRgb(Settings::itemSettings.color);

			ButtonToggle((const char*)u8"绘制已启用物品", &Settings::itemSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用物品不透明度", &Settings::itemSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用物品", &Settings::itemSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用物品不透明度", &Settings::itemSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称物品", &Settings::itemSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称物品", &Settings::itemSettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示物品名称", &Settings::itemSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示物品距离", &Settings::itemSettings.showDistance);

			ButtonToggle((const char*)u8"物品信息阴影", &Settings::itemSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"物品信息居中", &Settings::itemSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"物品白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"物品白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::itemSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###物品白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::itemSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"植物群系透视设置"))
		{
			ButtonToggle((const char*)u8"植物群系透视已启用", &Settings::floraSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###植物群系透视距离", &Settings::floraSettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###植物群系颜色", Settings::floraSettings.color);
			Utils::ValidateRgb(Settings::floraSettings.color);

			ButtonToggle((const char*)u8"绘制已启用植物群系", &Settings::floraSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用植物群系不透明度", &Settings::floraSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用植物群系", &Settings::floraSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用植物群系不透明度", &Settings::floraSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称植物群系", &Settings::floraSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称植物群系", &Settings::floraSettings.drawUnnamed);

			LargeButtonToggle((const char*)u8"绘制猩红色原料溶剂", &Settings::customFluxSettings.crimsonFluxEnabled);
			LargeButtonToggle((const char*)u8"绘制钴蓝色原料溶剂", &Settings::customFluxSettings.cobaltFluxEnabled);
			LargeButtonToggle((const char*)u8"绘制黄饼原料溶剂", &Settings::customFluxSettings.yellowcakeFluxEnabled);
			LargeButtonToggle((const char*)u8"绘制荧光原料溶剂", &Settings::customFluxSettings.fluorescentFluxEnabled);
			LargeButtonToggle((const char*)u8"绘制紫色原料溶剂", &Settings::customFluxSettings.violetFluxEnabled);

			ButtonToggle((const char*)u8"显示植物群系名称", &Settings::floraSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"显示植物群系距离", &Settings::floraSettings.showDistance);

			ButtonToggle((const char*)u8"植物群系信息阴影", &Settings::floraSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"植物群系信息居中", &Settings::floraSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"植物群系白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"植物群系白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::floraSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###植物群系白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::floraSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"实体透视设置"))
		{
			ButtonToggle((const char*)u8"实体透视已启用", &Settings::entitySettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###实体透视距离", &Settings::entitySettings.enabledDistance, 0, 3000, (const char*)u8"距离: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###实体颜色", Settings::entitySettings.color);
			Utils::ValidateRgb(Settings::entitySettings.color);

			ButtonToggle((const char*)u8"绘制已启用实体", &Settings::entitySettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已启用实体不透明度", &Settings::entitySettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制已禁用实体", &Settings::entitySettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###已禁用实体不透明度", &Settings::entitySettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"不透明度: %.2f");

			ButtonToggle((const char*)u8"绘制有名称实体", &Settings::entitySettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"绘制无名称实体", &Settings::entitySettings.drawUnnamed);

			ButtonToggle((const char*)u8"显示实体名称", &Settings::entitySettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"现实实体距离", &Settings::entitySettings.showDistance);

			ButtonToggle((const char*)u8"实体信息阴影", &Settings::entitySettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"实体信息居中", &Settings::entitySettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"实体白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"实体白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::floraSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###实体白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::entitySettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}
		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabInfoBox()
{
	if (ImGui::BeginTabItem((const char*)u8"信息窗口###信息窗口页面"))
	{
		LargeButtonToggle((const char*)u8"绘制自动材料搜刮状态", &Settings::infobox.drawScrapLooterStatus);
		LargeButtonToggle((const char*)u8"绘制自动物品搜刮状态", &Settings::infobox.drawItemLooterStatus);
		LargeButtonToggle((const char*)u8"绘制NPC搜刮状态", &Settings::infobox.drawNpcLooterStatus);
		LargeButtonToggle((const char*)u8"绘制容器搜刮状态", &Settings::infobox.drawContainerLooterStatus);
		LargeButtonToggle((const char*)u8"绘制植物群系收获状态", &Settings::infobox.drawHarvesterStatus);
		LargeButtonToggle((const char*)u8"绘制位置信息", &Settings::infobox.drawPositionSpoofingStatus);
		LargeButtonToggle((const char*)u8"绘制核弹密码", &Settings::infobox.drawNukeCodes);

		ImGui::EndTabItem();
	}
}
void Gui::OverlayMenuTabLoot()
{
	if (ImGui::BeginTabItem((const char*)u8"搜刮###搜刮页面"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"材料搜刮"))
		{
			if (ErectusMemory::CheckScrapList())
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
				if (ImGui::Button((const char*)u8"材料搜刮 (快捷键: CTRL+E)###搜刮选定材料已启用", ImVec2(224.0f, 0.0f)))
					ErectusMemory::LootScrap();
				ImGui::PopStyleColor(3);
			}
			else
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
				ImGui::Button((const char*)u8"材料搜刮 (快捷键: CTRL+E)###搜刮选中材料已禁用", ImVec2(224.0f, 0.0f));
				ImGui::PopStyleColor(3);
				ImGui::PopItemFlag();
			}

			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"材料搜刮快捷键已启用", &Settings::scrapLooter.keybindEnabled);

			ButtonToggle((const char*)u8"使用垃圾透视设置", &Settings::scrapLooter.scrapOverrideEnabled);

			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"自动搜刮已启用###材料自动搜刮已启用", &Settings::scrapLooter.autoLootingEnabled);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"速度 (最小): {0:d} ({1:d} 毫秒)", Settings::scrapLooter.autoLootingSpeedMin, Settings::scrapLooter.autoLootingSpeedMin * 16);
				if (ImGui::SliderInt((const char*)u8"###材料搜刮最小速度", &Settings::scrapLooter.autoLootingSpeedMin, 10, 60, sliderText.c_str()))
				{
					if (Settings::scrapLooter.autoLootingSpeedMax < Settings::scrapLooter.autoLootingSpeedMin)
						Settings::scrapLooter.autoLootingSpeedMax = Settings::scrapLooter.autoLootingSpeedMin;
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"速度 (最大): {0:d} ({1:d} 毫秒)", Settings::scrapLooter.autoLootingSpeedMax, Settings::scrapLooter.autoLootingSpeedMax * 16);
				if (ImGui::SliderInt((const char*)u8"###材料搜刮最大速度", &Settings::scrapLooter.autoLootingSpeedMax, 10, 60, sliderText.c_str()))
				{
					if (Settings::scrapLooter.autoLootingSpeedMax < Settings::scrapLooter.autoLootingSpeedMin)
						Settings::scrapLooter.autoLootingSpeedMin = Settings::scrapLooter.autoLootingSpeedMax;
				}
			}

			ImGui::SetNextItemWidth(451.0f);
			ImGui::SliderInt((const char*)u8"###材料搜刮距离", &Settings::scrapLooter.maxDistance, 1, 3000, (const char*)u8"材料搜刮距离: %d");

			for (auto i = 0; i < 40; i++)
			{
				ButtonToggle(Settings::scrapLooter.nameList[i], &Settings::scrapLooter.enabledList[i]);

				ImGui::SameLine(235.0f);
				ImGui::SetNextItemWidth(224.0f);

				auto inputLabel = fmt::format((const char*)u8"###材料只读{:d}", i);
				auto inputText = fmt::format((const char*)u8"{:08X}", Settings::scrapLooter.formIdList[i]);
				ImGui::InputText(inputLabel.c_str(), &inputText, ImGuiInputTextFlags_ReadOnly);
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"物品搜刮"))
		{
			if (ErectusMemory::CheckItemLooterSettings())
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
				if (ImGui::Button((const char*)u8"物品搜刮 (快捷键: CTRL+R)###搜刮选定物品已启用", ImVec2(224.0f, 0.0f)))
					ErectusMemory::LootItems();
				ImGui::PopStyleColor(3);
			}
			else
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
				ImGui::Button((const char*)u8"物品搜刮 (快捷键: CTRL+R)###搜刮选定物品已禁用", ImVec2(224.0f, 0.0f));
				ImGui::PopStyleColor(3);
				ImGui::PopItemFlag();
			}

			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"物品搜刮快捷键已启用", &Settings::itemLooter.keybindEnabled);

			LargeButtonToggle((const char*)u8"自动搜刮已启用###物品自动搜刮已启用", &Settings::itemLooter.autoLootingEnabled);
			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"速度 (最小): {0:d} ({1:d} 毫秒)", Settings::itemLooter.autoLootingSpeedMin, Settings::itemLooter.autoLootingSpeedMin * 16);
				if (ImGui::SliderInt((const char*)u8"###物品搜刮最小速度", &Settings::itemLooter.autoLootingSpeedMin, 10, 60, sliderText.c_str()))
				{
					if (Settings::itemLooter.autoLootingSpeedMax < Settings::itemLooter.autoLootingSpeedMin)
						Settings::itemLooter.autoLootingSpeedMax = Settings::itemLooter.autoLootingSpeedMin;
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"速度 (最大): {0:d} ({1:d} 毫秒)", Settings::itemLooter.autoLootingSpeedMax, Settings::itemLooter.autoLootingSpeedMax * 16);
				if (ImGui::SliderInt((const char*)u8"###物品搜刮最大速度", &Settings::itemLooter.autoLootingSpeedMax, 10, 60, sliderText.c_str()))
				{
					if (Settings::itemLooter.autoLootingSpeedMax < Settings::itemLooter.autoLootingSpeedMin)
						Settings::itemLooter.autoLootingSpeedMin = Settings::itemLooter.autoLootingSpeedMax;
				}
			}

			ButtonToggle((const char*)u8"武器已启用###武器搜刮已启用", &Settings::itemLooter.lootWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###武器搜刮距离", &Settings::itemLooter.lootWeaponsDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"装甲已启用###装甲搜刮已启用", &Settings::itemLooter.lootArmorEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###装甲搜刮距离", &Settings::itemLooter.lootArmorDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"弹药已启用###弹药搜刮已启用", &Settings::itemLooter.lootAmmoEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###弹药搜刮距离", &Settings::itemLooter.lootAmmoDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"改装件已启用###改装件搜刮已启用", &Settings::itemLooter.lootModsEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###改装件搜刮距离", &Settings::itemLooter.lootModsDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"杂志已启用###杂志搜刮已启用", &Settings::itemLooter.lootMagazinesEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###杂志搜刮距离", &Settings::itemLooter.lootMagazinesDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"娃娃已启用###娃娃搜刮已启用", &Settings::itemLooter.lootBobbleheadsEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###娃娃搜刮距离", &Settings::itemLooter.lootBobbleheadsDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"辅助品已启用###辅助品搜刮已启用", &Settings::itemLooter.lootAidEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###辅助品搜刮距离", &Settings::itemLooter.lootAidDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"已知图纸已启用###已知图纸搜刮已启用", &Settings::itemLooter.lootKnownPlansEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###已知图纸搜刮距离", &Settings::itemLooter.lootKnownPlansDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"未知图纸已启用###未知图纸搜刮已启用", &Settings::itemLooter.lootUnknownPlansEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###未知图纸搜刮距离", &Settings::itemLooter.lootUnknownPlansDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"杂项已启用###杂项搜刮已启用", &Settings::itemLooter.lootMiscEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###杂项搜刮距离", &Settings::itemLooter.lootMiscDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"其他已启用###其他搜刮已启用", &Settings::itemLooter.lootUnlistedEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###其他搜刮距离", &Settings::itemLooter.lootUnlistedDistance, 0, 3000, (const char*)u8"距离: %d");

			ButtonToggle((const char*)u8"物品FormId列表已启用###物品搜刮列表已启用", &Settings::itemLooter.lootListEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###物品搜刮列表距离", &Settings::itemLooter.lootListDistance, 0, 3000, (const char*)u8"距离: %d");

			LargeButtonToggle((const char*)u8"物品搜刮黑名单已启用###启用/禁用物品搜刮黑名单", &Settings::itemLooter.blacklistToggle);

			if (ImGui::CollapsingHeader((const char*)u8"物品搜刮FormId列表"))
			{
				for (auto i = 0; i < 100; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"物品搜刮位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::itemLooter.enabledList[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###物品搜刮列表{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::itemLooter.formIdList[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}

			if (ImGui::CollapsingHeader((const char*)u8"物品搜刮黑名单"))
			{
				for (auto i = 0; i < 64; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"物品搜刮黑名单: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::itemLooter.blacklistEnabled[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###物品搜刮黑名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::itemLooter.blacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"NPC搜刮 (76m 距离限制)"))
		{
			LargeButtonToggle((const char*)u8"自动NPC搜刮已启用 (快捷键: CTRL+COMMA)###NPC搜刮已启用", &Settings::npcLooter.enabled);

			ButtonToggle((const char*)u8"所有武器已启用###NPC搜刮所有武器已启用", &Settings::npcLooter.allWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"所有装甲已启用###NPC搜刮所有装甲已启用", &Settings::npcLooter.allArmorEnabled);

			ButtonToggle((const char*)u8"一星传奇武器已启用###NPC搜刮一星传奇武器已启用", &Settings::npcLooter.oneStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"一星传奇装甲已启用###NPC搜刮一星传奇装甲已启用", &Settings::npcLooter.oneStarArmorEnabled);

			ButtonToggle((const char*)u8"二星传奇武器已启用###NPC搜刮二星传奇武器已启用", &Settings::npcLooter.twoStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"二星传奇装甲已启用###NPC搜刮二星传奇装甲已启用", &Settings::npcLooter.twoStarArmorEnabled);

			ButtonToggle((const char*)u8"三星传奇武器已启用###NPC搜刮三星传奇武器已启用", &Settings::npcLooter.threeStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"三星传奇装甲已启用###NPC搜刮三星传奇装甲已启用", &Settings::npcLooter.threeStarArmorEnabled);

			ButtonToggle((const char*)u8"弹药已启用###NPC搜刮弹药已启用", &Settings::npcLooter.ammoEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"改装件已启用###NPC搜刮改装件已启用", &Settings::npcLooter.modsEnabled);

			ButtonToggle((const char*)u8"瓶盖已启用###NPC搜刮瓶盖已启用", &Settings::npcLooter.capsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"垃圾已启用###NPC搜刮垃圾已启用", &Settings::npcLooter.junkEnabled);

			ButtonToggle((const char*)u8"辅助品已启用###NPC搜刮辅助品已启用", &Settings::npcLooter.aidEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"藏宝图已启用###NPC搜刮藏宝图已启用", &Settings::npcLooter.treasureMapsEnabled);

			ButtonToggle((const char*)u8"已知图纸已启用###NPC已知图纸搜刮已启用", &Settings::npcLooter.knownPlansEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"未知图纸已启用###NPC未知图纸搜刮已启用", &Settings::npcLooter.unknownPlansEnabled);

			ButtonToggle((const char*)u8"杂项已启用###NPC杂项搜刮已启用", &Settings::npcLooter.miscEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"其他已启用###NPC其他搜刮已启用", &Settings::npcLooter.unlistedEnabled);

			LargeButtonToggle((const char*)u8"NPC搜刮FormId列表已启用###NPC搜刮列表已启用", &Settings::npcLooter.listEnabled);
			LargeButtonToggle((const char*)u8"NPC搜刮黑名单已启用###启用/禁用NPC搜刮黑名单", &Settings::npcLooter.blacklistToggle);

			if (ImGui::CollapsingHeader((const char*)u8"NPC搜刮FormId列表"))
			{
				for (auto i = 0; i < 100; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"NPC搜刮位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::npcLooter.enabledList[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###NPC搜刮列表{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::npcLooter.formIdList[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}

			if (ImGui::CollapsingHeader((const char*)u8"NPC搜刮黑名单"))
			{
				for (auto i = 0; i < 64; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"NPC搜刮黑名单: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::npcLooter.blacklistEnabled[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###NPC搜刮黑名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::npcLooter.blacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"容器搜刮 (6m 距离限制)"))
		{
			LargeButtonToggle((const char*)u8"自动容器搜刮已启用 (快捷键: CTRL+PERIOD)###容器搜刮已启用", &Settings::containerLooter.enabled);

			ButtonToggle((const char*)u8"所有武器已启用###容器搜刮所有武器已启用", &Settings::containerLooter.allWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"所有装甲已启用###容器搜刮所有装甲已启用", &Settings::containerLooter.allArmorEnabled);

			ButtonToggle((const char*)u8"一星传奇武器已启用###容器搜刮一星传奇武器已启用", &Settings::containerLooter.oneStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"一星传奇护甲已启用###容器搜刮一星传奇护甲已启用", &Settings::containerLooter.oneStarArmorEnabled);

			ButtonToggle((const char*)u8"二星传奇武器已启用###容器搜刮二星传奇武器已启用", &Settings::containerLooter.twoStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"二星传奇护甲已启用###容器搜刮二星传奇护甲已启用", &Settings::containerLooter.twoStarArmorEnabled);

			ButtonToggle((const char*)u8"三星传奇武器已启用###容器搜刮三星传奇武器已启用", &Settings::containerLooter.threeStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"三星传奇护甲已启用###容器搜刮三星传奇护甲已启用", &Settings::containerLooter.threeStarArmorEnabled);

			ButtonToggle((const char*)u8"弹药已启用###容器搜刮弹药已启用", &Settings::containerLooter.ammoEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"改装件已启用###容器搜刮改装件已启用", &Settings::containerLooter.modsEnabled);

			ButtonToggle((const char*)u8"瓶盖已启用###容器搜刮瓶盖已启用", &Settings::containerLooter.capsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"垃圾已启用###容器搜刮垃圾已启用", &Settings::containerLooter.junkEnabled);

			ButtonToggle((const char*)u8"辅助品已启用###容器搜刮辅助品已启用", &Settings::containerLooter.aidEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"藏宝图已启用###容器搜刮藏宝图已启用", &Settings::containerLooter.treasureMapsEnabled);

			ButtonToggle((const char*)u8"已知图纸已启用###容器搜刮已知图纸已启用", &Settings::containerLooter.knownPlansEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"未知图纸已启用###容器搜刮未知图纸已启用", &Settings::containerLooter.unknownPlansEnabled);

			ButtonToggle((const char*)u8"杂项已启用###容器搜刮杂项已启用", &Settings::containerLooter.miscEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"其他已启用###容器搜刮其他已启用", &Settings::containerLooter.unlistedEnabled);

			LargeButtonToggle((const char*)u8"容器搜刮FormId列表已启用###容器搜刮列表已启用", &Settings::containerLooter.listEnabled);

			LargeButtonToggle((const char*)u8"容器搜刮黑名单已启用###启用/禁用容器搜刮黑名单", &Settings::containerLooter.blacklistToggle);

			if (ImGui::CollapsingHeader((const char*)u8"容器搜刮FormId列表"))
			{
				for (auto i = 0; i < 100; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"容器搜刮位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::containerLooter.enabledList[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###容器搜刮列表{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::containerLooter.formIdList[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}

			if (ImGui::CollapsingHeader((const char*)u8"容器搜刮黑名单"))
			{
				for (auto i = 0; i < 64; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"容器搜刮黑名单: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::containerLooter.blacklistEnabled[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###容器搜刮黑名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::containerLooter.blacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"植物群系收获 (6m 距离限制)"))
		{
			LargeButtonToggle((const char*)u8"自动植物群系收获已启用 (快捷键: CTRL+P])###植物群系收获已启用", &Settings::harvester.enabled);
			LargeButtonToggle((const char*)u8"植物群系收获越过设置 (使用植物群系透视设置)", &Settings::harvester.overrideEnabled);

			for (auto i = 0; i < 69; i++)
			{
				ButtonToggle(Settings::harvester.nameList[i], &Settings::harvester.enabledList[i]);

				ImGui::SameLine(235.0f);
				ImGui::SetNextItemWidth(224.0f);

				auto inputLabel = fmt::format((const char*)u8"###植物群系只读{:d}", i);
				auto inputText = fmt::format((const char*)u8"{:08X}", Settings::harvester.formIdList[i]);
				ImGui::InputText(inputLabel.c_str(), &inputText, ImGuiInputTextFlags_ReadOnly);
			}
		}

		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabCombat()
{
	if (ImGui::BeginTabItem((const char*)u8"战斗###战斗页面"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"武器编辑器"))
		{
			ButtonToggle((const char*)u8"无后坐力", &Settings::weapons.noRecoil);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"无限子弹", &Settings::weapons.infiniteAmmo);

			ButtonToggle((const char*)u8"无扩散", &Settings::weapons.noSpread);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"瞬间换弹", &Settings::weapons.instantReload);

			ButtonToggle((const char*)u8"无晃动", &Settings::weapons.noSway);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"全自动标识###武器全自动", &Settings::weapons.automaticflag);

			ButtonToggle((const char*)u8"弹匣容量###武器弹匣容量已启用", &Settings::weapons.capacityEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###武器弹匣容量", &Settings::weapons.capacity, 0, 999, (const char*)u8"弹匣容量: %d");

			ButtonToggle((const char*)u8"射速###武器射速已启用", &Settings::weapons.speedEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###武器射速", &Settings::weapons.speed, 0.0f, 100.0f, (const char*)u8"射速: %.2f");

			ButtonToggle((const char*)u8"射程###武器射程已启用", &Settings::weapons.reachEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###武器射程", &Settings::weapons.reach, 0.0f, 999.0f, (const char*)u8"射程: %.2f");
		}

		if (ImGui::CollapsingHeader((const char*)u8"目标设置"))
		{
			ButtonToggle((const char*)u8"锁定玩家 (快捷键: T)", &Settings::targetting.lockPlayers);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"锁定NPC (快捷键: T)", &Settings::targetting.lockNpCs);

			ButtonToggle((const char*)u8"伤害重定向 (玩家)", &Settings::targetting.indirectPlayers);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"伤害重定向 (NPC)", &Settings::targetting.indirectNpCs);

			ButtonToggle((const char*)u8"发送伤害 (玩家)", &Settings::targetting.directPlayers);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"发送伤害 (NPC)", &Settings::targetting.directNpCs);

			SmallButtonToggle((const char*)u8"存活###锁定存活", &Settings::targetting.targetLiving);
			ImGui::SameLine(122.0f);
			SmallButtonToggle((const char*)u8"倒地###锁定倒地", &Settings::targetting.targetDowned);
			ImGui::SameLine(235.0f);
			SmallButtonToggle((const char*)u8"死亡###锁定死亡", &Settings::targetting.targetDead);
			ImGui::SameLine(349.0f);
			SmallButtonToggle((const char*)u8"未知###锁定未知", &Settings::targetting.targetUnknown);

			ButtonToggle((const char*)u8"忽略超远距离###忽略超远距离", &Settings::targetting.ignoreRenderDistance);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###目标锁定角度", &Settings::targetting.lockingFov, 5.0f, 40.0f, (const char*)u8"锁定角度: %.2f");

			ButtonToggle((const char*)u8"忽略基本NPC###忽略基本NPC", &Settings::targetting.ignoreEssentialNpCs);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###目标锁定颜色", Settings::targetting.lockingColor);
			Utils::ValidateRgb(Settings::playerSettings.unknownColor);

			ButtonToggle((const char*)u8"自动切换目标###目标锁定切换", &Settings::targetting.retargeting);

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"冷却: {0:d} ({1:d} 毫秒)", Settings::targetting.cooldown, Settings::targetting.cooldown * 16);
				ImGui::SliderInt((const char*)u8"###目标锁定冷却", &Settings::targetting.cooldown, 0, 120, sliderText.c_str());
			}

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"发送伤害 (最小): {0:d} ({1:d} 毫秒)", Settings::targetting.sendDamageMin, Settings::targetting.sendDamageMin * 16);
				if (ImGui::SliderInt((const char*)u8"###最小发送伤害", &Settings::targetting.sendDamageMin, 1, 60, sliderText.c_str()))
				{
					if (Settings::targetting.sendDamageMax < Settings::targetting.sendDamageMin)
						Settings::targetting.sendDamageMax = Settings::targetting.sendDamageMin;
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"发送伤害 (最大): {0:d} ({1:d} 毫秒)", Settings::targetting.sendDamageMax, Settings::targetting.sendDamageMax * 16);
				if (ImGui::SliderInt((const char*)u8"###最大发送伤害", &Settings::targetting.sendDamageMax, 1, 60, sliderText.c_str()))
				{
					if (Settings::targetting.sendDamageMax < Settings::targetting.sendDamageMin)
						Settings::targetting.sendDamageMin = Settings::targetting.sendDamageMax;
				}
			}

			{
				std::string favoritedWeaponsPreview = (const char*)u8"[?] 未选定武器";
				if (Settings::targetting.favoriteIndex < 12)
				{
					favoritedWeaponsPreview = ErectusMemory::GetFavoritedWeaponText(BYTE(Settings::targetting.favoriteIndex));
					if (favoritedWeaponsPreview.empty())
					{
						favoritedWeaponsPreview = fmt::format((const char*)u8"[{}] 无效的最爱武器", ErectusMemory::FavoriteIndex2Slot(BYTE(Settings::targetting.favoriteIndex)));
					}
				}

				ImGui::SetNextItemWidth(451.0f);
				if (ImGui::BeginCombo((const char*)u8"###启动连续攻击", favoritedWeaponsPreview.c_str()))
				{
					for (const auto& item : ErectusMemory::GetFavoritedWeapons())
					{
						if (ImGui::Selectable(item.second.c_str()))
						{
							if (item.first)
								Settings::targetting.favoriteIndex = item.first - 1;
							else
								Settings::targetting.favoriteIndex = 12;
						}
					}
					Utils::ValidateInt(Settings::targetting.favoriteIndex, 0, 12);

					ImGui::EndCombo();
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"近战设置"))
		{
			LargeButtonToggle((const char*)u8"近战已启用 (快捷键: U)", &Settings::melee.enabled);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"近战速度 (最小): {0:d} ({1:d} 毫秒)", Settings::melee.speedMin, Settings::melee.speedMin * 16);
				if (ImGui::SliderInt((const char*)u8"###最小近战速度", &Settings::melee.speedMin, 1, 60, sliderText.c_str()))
				{
					if (Settings::melee.speedMax < Settings::melee.speedMin)
						Settings::melee.speedMax = Settings::melee.speedMin;
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"近战速度 (最大): {0:d} ({1:d} 毫秒)", Settings::melee.speedMax, Settings::melee.speedMax * 16);
				if (ImGui::SliderInt((const char*)u8"###最大近战速度", &Settings::melee.speedMax, 1, 60, sliderText.c_str()))
				{
					if (Settings::melee.speedMax < Settings::melee.speedMin)
						Settings::melee.speedMin = Settings::melee.speedMax;
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"一击必杀"))
		{
			LargeButtonToggle((const char*)u8"一击必杀玩家 (快捷键: CTRL+B)", &Settings::opk.playersEnabled);
			LargeButtonToggle((const char*)u8"一击必杀NPC (快捷键: CTRL+N)", &Settings::opk.npcsEnabled);
		}

		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabPlayer()
{
	if (ImGui::BeginTabItem((const char*)u8"玩家###玩家页面"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"本地玩家设置"))
		{
			ButtonToggle((const char*)u8"位置伪造 (快捷键 CTRL+L)##本地玩家位置伪造已启用", &Settings::customLocalPlayerSettings.positionSpoofingEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###本地玩家位置伪造高度", &Settings::customLocalPlayerSettings.positionSpoofingHeight, -524287, 524287, (const char*)u8"伪造高度: %d");

			ButtonToggle((const char*)u8"穿墙 (快捷键 CTRL+Y)###穿墙已启动", &Settings::customLocalPlayerSettings.noclipEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderFloat((const char*)u8"###穿墙速度", &Settings::customLocalPlayerSettings.noclipSpeed, 0.0f, 2.0f, (const char*)u8"速度: %.5f");

			ButtonToggle((const char*)u8"客户端声明", &Settings::customLocalPlayerSettings.clientState);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"自动客户端声明", &Settings::customLocalPlayerSettings.automaticClientState);

			LargeButtonToggle((const char*)u8"锁定行动点数###本地玩家锁定AP点数已启用", &Settings::customLocalPlayerSettings.freezeApEnabled);

			ButtonToggle((const char*)u8"行动点数###本地玩家行动点数已启用", &Settings::customLocalPlayerSettings.actionPointsEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###本地玩家行动点数", &Settings::customLocalPlayerSettings.actionPoints, 0, 99999, (const char*)u8"行动点数: %d");

			ButtonToggle((const char*)u8"力量###本地玩家力量已启用", &Settings::customLocalPlayerSettings.strengthEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###本地玩家力量", &Settings::customLocalPlayerSettings.strength, 0, 99999, (const char*)u8"力量: %d");

			ButtonToggle((const char*)u8"感知###本地玩家感知已启用", &Settings::customLocalPlayerSettings.perceptionEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###本地玩家感知", &Settings::customLocalPlayerSettings.perception, 0, 99999, (const char*)u8"感知: %d");

			ButtonToggle((const char*)u8"耐力###本地玩家耐力已启用", &Settings::customLocalPlayerSettings.enduranceEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###本地玩家耐力", &Settings::customLocalPlayerSettings.endurance, 0, 99999, (const char*)u8"耐力: %d");

			ButtonToggle((const char*)u8"魅力###本地玩家魅力已启用", &Settings::customLocalPlayerSettings.charismaEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###本地玩家魅力", &Settings::customLocalPlayerSettings.charisma, 0, 99999, (const char*)u8"魅力: %d");

			ButtonToggle((const char*)u8"智力###本地玩家智力已启用", &Settings::customLocalPlayerSettings.intelligenceEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###本地玩家智力", &Settings::customLocalPlayerSettings.intelligence, 0, 99999, (const char*)u8"智力: %d");

			ButtonToggle((const char*)u8"敏捷###本地玩家敏捷已启用", &Settings::customLocalPlayerSettings.agilityEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###本地玩家敏捷", &Settings::customLocalPlayerSettings.agility, 0, 99999, (const char*)u8"敏捷: %d");

			ButtonToggle((const char*)u8"幸运###本地玩家幸运已启用", &Settings::customLocalPlayerSettings.luckEnabled);					ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###本地玩家幸运", &Settings::customLocalPlayerSettings.luck, 0, 99999, (const char*)u8"幸运: %d");
		}

		if (ImGui::CollapsingHeader((const char*)u8"角色设置"))
		{
			LargeButtonToggle((const char*)u8"角色外观编辑器已启用###角色编辑器已启用", &Settings::characterEditor.enabled);
			ImGui::SetNextItemWidth(451.0f);
			ImGui::SliderFloat((const char*)u8"###角色胖瘦", &Settings::characterEditor.thin, 0.0f, 1.0f, (const char*)u8"角色外观 (胖瘦): %f");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::SliderFloat((const char*)u8"###角色肌肉", &Settings::characterEditor.muscular, 0.0f, 1.0f, (const char*)u8"角色外观 (肌肉): %f");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::SliderFloat((const char*)u8"###角色大小", &Settings::characterEditor.large, 0.0f, 1.0f, (const char*)u8"角色外观 (大小): %f");
		}
		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabUtilities()
{
	if (ImGui::BeginTabItem((const char*)u8"工具###工具页面"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"工具"))
		{
			ButtonToggle((const char*)u8"绘制本地玩家数据", &Settings::utilities.debugPlayer);

			ImGui::SameLine(235.0f);

			ButtonToggle((const char*)u8"透视DEBUG模式", &Settings::utilities.debugEsp);

			{
				if (Settings::utilities.ptrFormId)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

					if (ImGui::Button((const char*)u8"获取指针###获取指针已启用", ImVec2(224.0f, 0.0f)))
						getPtrResult = ErectusMemory::GetPtr(Settings::utilities.ptrFormId);

					ImGui::PopStyleColor(3);
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::Button((const char*)u8"获取指针###获取指针已禁用", ImVec2(224.0f, 0.0f));
					ImGui::PopItemFlag();

					ImGui::PopStyleColor(3);
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(80.0f);
				if (ImGui::InputScalar((const char*)u8"###指针FormId数据", ImGuiDataType_U32, &Settings::utilities.ptrFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal))
					getPtrResult = 0;
			}

			ImGui::SameLine(318.0f);

			{
				ImGui::SetNextItemWidth(141.0f);
				auto inputText = fmt::format("{:16X}", getPtrResult);
				ImGui::InputText((const char*)u8"###指针数据", &inputText, ImGuiInputTextFlags_ReadOnly);
			}

			{
				if (Settings::utilities.addressFormId)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

					if (ImGui::Button((const char*)u8"获取地址###获取地址已启用", ImVec2(224.0f, 0.0f)))
						getAddressResult = ErectusMemory::GetAddress(Settings::utilities.addressFormId);

					ImGui::PopStyleColor(3);
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::Button((const char*)u8"获取地址###获取地址已禁用", ImVec2(224.0f, 0.0f));
					ImGui::PopItemFlag();

					ImGui::PopStyleColor(3);
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(80.0f);

				if (ImGui::InputScalar((const char*)u8"###地址FormId数据", ImGuiDataType_U32, &Settings::utilities.addressFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal))
					getAddressResult = 0;
			}

			ImGui::SameLine(318.0f);

			{
				ImGui::SetNextItemWidth(141.0f);

				auto inputText = fmt::format("{:16X}", getAddressResult);
				ImGui::InputText((const char*)u8"###地址指针数据", &inputText, ImGuiInputTextFlags_ReadOnly);
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"引用编辑器"))
		{
			ButtonToggle((const char*)u8"源FormId###切换交换源FormId", &swapperSourceToggle);

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				ImGui::InputScalar((const char*)u8"###交换源FormId数据", ImGuiDataType_U32, &Settings::swapper.sourceFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
			}

			ButtonToggle((const char*)u8"目标FormId###切换交换目标FormId", &swapperDestinationToggle);

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				ImGui::InputScalar((const char*)u8"###交换目标FormId数据", ImGuiDataType_U32, &Settings::swapper.destinationFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
			}

			if (swapperSourceToggle && Settings::swapper.sourceFormId && swapperDestinationToggle && Settings::swapper.destinationFormId)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

				if (ImGui::Button((const char*)u8"编辑引用 (重写目标)###编辑引用已启用", ImVec2(451.0f, 0.0f)))
				{
					if (ErectusMemory::ReferenceSwap(Settings::swapper.sourceFormId, Settings::swapper.destinationFormId))
					{
						Settings::swapper.destinationFormId = Settings::swapper.sourceFormId;
						swapperSourceToggle = false;
						swapperDestinationToggle = false;
					}
				}

				ImGui::PopStyleColor(3);
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::Button((const char*)u8"编辑引用 (重写目标)###编辑引用已禁用", ImVec2(451.0f, 0.0f));
				ImGui::PopItemFlag();

				ImGui::PopStyleColor(3);
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"物品传输"))
		{
			SmallButtonToggle((const char*)u8"源###切换源物品FormId", &transferSourceToggle);

			ImGui::SameLine(122.0f);

			{
				ImGui::SetNextItemWidth(110.0f);
				ImGui::InputScalar((const char*)u8"###源物品FormId数据", ImGuiDataType_U32, &Settings::customTransferSettings.sourceFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

				if (ImGui::Button((const char*)u8"获取玩家###本地玩家源物品", ImVec2(110.0f, 0.0f)))
					Settings::customTransferSettings.sourceFormId = ErectusMemory::GetLocalPlayerFormId();

				ImGui::SameLine(349.0f);

				if (ImGui::Button((const char*)u8"获取储物箱###源物品储物箱", ImVec2(110.0f, 0.0f)))
					Settings::customTransferSettings.sourceFormId = ErectusMemory::GetStashFormId();

				ImGui::PopStyleColor(3);
			}

			SmallButtonToggle((const char*)u8"目标###切换目标物品FormId", &transferDestinationToggle);

			ImGui::SameLine(122.0f);

			{
				ImGui::SetNextItemWidth(110.0f);
				ImGui::InputScalar((const char*)u8"###目标物品FormId数据", ImGuiDataType_U32, &Settings::customTransferSettings.destinationFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
			}

			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
				ImGui::SameLine(235.0f);
				if (ImGui::Button((const char*)u8"获取玩家###本地玩家目标物品", ImVec2(110.0f, 0.0f)))
					Settings::customTransferSettings.destinationFormId = ErectusMemory::GetLocalPlayerFormId();

				ImGui::SameLine(349.0f);
				if (ImGui::Button((const char*)u8"获取储物箱###目标物品储物箱", ImVec2(110.0f, 0.0f)))
					Settings::customTransferSettings.destinationFormId = ErectusMemory::GetStashFormId();
				ImGui::PopStyleColor(3);
			}

			auto allowTransfer = false;

			if (transferSourceToggle && Settings::customTransferSettings.sourceFormId && transferDestinationToggle && Settings::customTransferSettings.destinationFormId)
			{
				if (Settings::customTransferSettings.useWhitelist)
					allowTransfer = ErectusMemory::CheckItemTransferList();
				else
					allowTransfer = true;
			}

			if (allowTransfer)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

				if (ImGui::Button((const char*)u8"传输物品###传输物品已启用", ImVec2(451.0f, 0.0f)))
					ErectusMemory::TransferItems(Settings::customTransferSettings.sourceFormId, Settings::customTransferSettings.destinationFormId);

				ImGui::PopStyleColor(3);
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::Button((const char*)u8"传输物品###传输物品已禁用", ImVec2(451.0f, 0.0f));
				ImGui::PopItemFlag();

				ImGui::PopStyleColor(3);
			}

			LargeButtonToggle((const char*)u8"使用物品传输白名单", &Settings::customTransferSettings.useWhitelist);
			LargeButtonToggle((const char*)u8"使用物品传输黑名单", &Settings::customTransferSettings.useBlacklist);

			if (ImGui::CollapsingHeader((const char*)u8"物品传输白名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"传输白名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::customTransferSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###物品传输白名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::customTransferSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}

			if (ImGui::CollapsingHeader((const char*)u8"物品传输黑名单设置"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"传输黑名单位置: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::customTransferSettings.blacklisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###物品传输黑名单{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::customTransferSettings.blacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"核弹密码"))
		{
			ButtonToggle((const char*)u8"自动核弹密码", &Settings::customNukeCodeSettings.automaticNukeCodes);

			ImGui::SameLine(235.0f);

			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

				if (ImGui::Button((const char*)u8"获取核弹密码", ImVec2(224.0f, 0.0f)))
					ErectusMemory::UpdateNukeCodes();

				ImGui::PopStyleColor(3);
			}

			auto text = format((const char*)u8"{} - A点", fmt::join(ErectusMemory::alphaCode, " "));
			ImGui::Text(text.c_str());

			text = format((const char*)u8"{} - B点", fmt::join(ErectusMemory::bravoCode, " "));
			ImGui::Text(text.c_str());

			text = format((const char*)u8"{} - C点", fmt::join(ErectusMemory::charlieCode, " "));
			ImGui::Text(text.c_str());
		}

		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabTeleporter()
{
	if (ImGui::BeginTabItem((const char*)u8"传送###传送页面"))
	{
		for (auto i = 0; i < 16; i++)
		{
			auto teleportHeaderText = fmt::format((const char*)u8"传送位置: {0:d}", i);
			if (ImGui::CollapsingHeader(teleportHeaderText.c_str()))
			{
				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###传送目标X{:d}", i);
					ImGui::InputFloat(inputLabel.c_str(), &Settings::teleporter.entries[i].destination[0]);
				}

				ImGui::SameLine(122.0f);

				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###传送目标Y{:d}", i);
					ImGui::InputFloat(inputLabel.c_str(), &Settings::teleporter.entries[i].destination[1]);
				}

				ImGui::SameLine(235.0f);

				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###传送目标Z{:d}", i);
					ImGui::InputFloat(inputLabel.c_str(), &Settings::teleporter.entries[i].destination[2]);
				}

				ImGui::SameLine(349.0f);

				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###传送目标W{:d}", i);
					ImGui::InputFloat(inputLabel.c_str(), &Settings::teleporter.entries[i].destination[3]);
				}

				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###传送区块FormId{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::teleporter.entries[i].cellFormId,
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}

				ImGui::SameLine(122.0f);

				{
					auto buttonLabel = fmt::format((const char*)u8"设定位置###传送目标{:d}", i);
					if (!Settings::teleporter.entries[i].disableSaving)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

						if (ImGui::Button(buttonLabel.c_str(), ImVec2(110.0f, 0.0f)))
							ErectusMemory::GetTeleportPosition(i);

						ImGui::PopStyleColor(3);
					}
					else
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

						ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
						ImGui::Button(buttonLabel.c_str(), ImVec2(110.0f, 0.0f));
						ImGui::PopItemFlag();

						ImGui::PopStyleColor(3);
					}
				}

				ImGui::SameLine(235.0f);

				{
					auto buttonLabel = fmt::format((const char*)u8"锁定###禁用存档{:d}", i);
					SmallButtonToggle(buttonLabel.c_str(), &Settings::teleporter.entries[i].disableSaving);
				}

				ImGui::SameLine(349.0f);

				if (Settings::teleporter.entries[i].cellFormId)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

					auto buttonLabel = fmt::format((const char*)u8"传送###传送请求已启用{:d}", i);
					if (ImGui::Button(buttonLabel.c_str(), ImVec2(110.0f, 0.0f)))
						ErectusMemory::RequestTeleport(i);
					ImGui::PopStyleColor(3);
				}
				else
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

					auto buttonLabel = fmt::format((const char*)u8"传送###传送请求已禁用{:d}", i);
					ImGui::Button(buttonLabel.c_str(), ImVec2(110.0f, 0.0f));
					ImGui::PopStyleColor(3);
					ImGui::PopItemFlag();
				}
			}
		}
		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabBitMsgWriter()
{
	if (ImGui::BeginTabItem((const char*)u8"位信息写入###位信息写入页面"))
	{
		LargeButtonToggle((const char*)u8"信息发送已启用", &ErectusMemory::allowMessages);

		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenu()
{
	if (!App::overlayMenuActive)
		return;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(App::windowSize[0]), static_cast<float>(App::windowSize[1])));
	ImGui::SetNextWindowCollapsed(false);

	if (ImGui::Begin((const char*)u8"Erectus - 图层菜单", nullptr,
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysVerticalScrollbar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::MenuItem((const char*)u8"退出"))
				App::CloseWnd();

			if (ImGui::MenuItem((const char*)u8"主菜单"))
				ErectusProcess::SetProcessMenu();

			if (ImGui::MenuItem((const char*)u8"图层"))
			{
				if (!App::SetOverlayPosition(false, true))
					ErectusProcess::SetProcessMenu();
			}

			ImGui::EndMenuBar();
		}

		if (ImGui::BeginTabBar((const char*)u8"###图层菜单选项卡", ImGuiTabBarFlags_None))
		{
			OverlayMenuTabEsp();
			OverlayMenuTabLoot();
			OverlayMenuTabCombat();
			OverlayMenuTabPlayer();
			OverlayMenuTabInfoBox();
			OverlayMenuTabUtilities();
			OverlayMenuTabTeleporter();
			OverlayMenuTabBitMsgWriter();

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

bool Gui::Init()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImFont* font = io.Fonts->AddFontFromFileTTF((const char*)u8"D:/simyou.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

	if (!ImGui_ImplWin32_Init(App::appHwnd))
		return false;

	if (!ImGui_ImplDX9_Init(Renderer::d3D9Device))
		return false;

	return true;
}

void Gui::Shutdown()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
