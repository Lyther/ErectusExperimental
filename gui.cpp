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
		itemText = fmt::format("{0}\n{1:d} ����ֵ [{2:d} ��]", entry.name, health, normalDistance);
	else if (settings.showName && showHealthText && !settings.showDistance) //Name, Health
		itemText = fmt::format("{0}\n{1:d} ����ֵ", entry.name, health);
	else if (settings.showName && !showHealthText && settings.showDistance) //Name, Distance
		itemText = fmt::format("{0}\n[{1:d} ��]", entry.name, normalDistance);
	else if (!settings.showName && showHealthText && settings.showDistance) //Health, Distance
		itemText = fmt::format("{0:d} ����ֵ [{1:d} ��]", health, normalDistance);
	else if (settings.showName && !showHealthText && !settings.showDistance) //Name
		itemText = entry.name;
	else if (!settings.showName && showHealthText && !settings.showDistance) //Health
		itemText = fmt::format("{:d} ����ֵ", health);
	else if (!settings.showName && !showHealthText && settings.showDistance) //Distance
		itemText = fmt::format("[{:d} ��]", normalDistance);

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
		itemText = fmt::format("{0}\n[{1:d} ��]", entry.name, normalDistance);
	else if (settings.showName && !settings.showDistance)
		itemText = entry.name;
	else if (!settings.showName && settings.showDistance)
		itemText = fmt::format("[{0:d} ��]", normalDistance);

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

		featureText = fmt::format("���FormId: {:08x}", localPlayer.formId);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("������FormId: {:08x}", localPlayer.stashFormId);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("����FormId: {:08x}", localPlayer.cellFormId);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("X: {:f}", localPlayer.position[0]);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("Y: {:f}", localPlayer.position[1]);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("Z: {:f}", localPlayer.position[2]);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("ƫת: {:f}", localPlayer.yaw);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("��б: {:f}", localPlayer.pitch);
		infoTexts.emplace_back(featureText, true);

		featureText = fmt::format("����ֵ: {:f}", localPlayer.currentHealth);
		infoTexts.emplace_back(featureText, true);
	}

	if (Settings::infobox.drawScrapLooterStatus)
	{
		featureText = fmt::format("�����ѹ� (�Զ�): {:d}", static_cast<int>(Settings::scrapLooter.autoLootingEnabled));
		featureState = Settings::scrapLooter.autoLootingEnabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawItemLooterStatus)
	{
		featureText = fmt::format("��Ʒ�ѹ� (�Զ�): {:d}", static_cast<int>(Settings::itemLooter.autoLootingEnabled));
		featureState = Settings::itemLooter.autoLootingEnabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawNpcLooterStatus)
	{
		featureText = fmt::format("NPC�ѹ� (76m ��������): {:d}", static_cast<int>(Settings::npcLooter.enabled));
		featureState = Settings::npcLooter.enabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawContainerLooterStatus)
	{
		featureText = fmt::format("�����ѹ� (6m ��������): {:d}", static_cast<int>(Settings::containerLooter.enabled));
		featureState = Settings::containerLooter.enabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawHarvesterStatus)
	{
		featureText = fmt::format("ֲ��Ⱥϵ�ջ� (6m ��������): {:d}", static_cast<int>(Settings::harvester.enabled));
		featureState = Settings::harvester.enabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawPositionSpoofingStatus)
	{
		featureText = fmt::format("λ��α�� (����): {0:d} (�߶�: {1:d})", static_cast<int>(Threads::positionSpoofingToggle), Settings::customLocalPlayerSettings.positionSpoofingHeight);
		featureState = ErectusMemory::InsideInteriorCell() ? false : Settings::customLocalPlayerSettings.positionSpoofingEnabled;
		infoTexts.emplace_back(featureText, featureState);
	}

	if (Settings::infobox.drawNukeCodes)
	{
		featureText = format("{} - A��", fmt::join(ErectusMemory::alphaCode, " "));
		featureState = ErectusMemory::alphaCode == std::array<int, 8>{} ? false : true;
		infoTexts.emplace_back(featureText, featureState);

		featureText = format("{} - B��", fmt::join(ErectusMemory::bravoCode, " "));
		featureState = ErectusMemory::bravoCode == std::array<int, 8>{} ? false : true;
		infoTexts.emplace_back(featureText, featureState);

		featureText = format("{} - C��", fmt::join(ErectusMemory::charlieCode, " "));
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

	if (ImGui::Begin((const char*)u8"Erectus - ���˵�", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::MenuItem((const char*)u8"�˳�"))
				App::CloseWnd();
			if (ImGui::MenuItem((const char*)u8"ͼ��˵�"))
				App::SetOverlayMenu();
			if (!ErectusProcess::pid)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

				ImGui::MenuItem((const char*)u8"ͼ��");

				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			else
			{
				if (ImGui::MenuItem((const char*)u8"ͼ��"))
					App::SetOverlayPosition(false, true);
			}
			ImGui::EndMenuBar();
		}

		ImGui::SetNextItemWidth(-16.f);

		auto processText = ErectusProcess::pid ? "Fallout76.exe - " + std::to_string(ErectusProcess::pid) : (const char*)u8"δѡ�����.";
		if (ImGui::BeginCombo((const char*)u8"###�����б�", processText.c_str()))
		{
			for (auto item : ErectusProcess::GetProcesses())
			{
				processText = item ? "Fallout76.exe - " + std::to_string(item) : (const char*)u8"��";
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
		ImGui::Text((const char*)u8"ͼ��˵���ݼ�");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"CTRL+ENTER");
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"ˢ����");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%.1f", ImGui::GetIO().Framerate);
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"����ID");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%lu", ErectusProcess::pid);
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"Ӳ��ʶ����");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%p", ErectusProcess::hWnd);
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"��ַ");
		ImGui::NextColumn();
		ImGui::Text((const char*)u8"%016llX", ErectusProcess::exe);
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Text((const char*)u8"���");
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
	if (ImGui::BeginTabItem((const char*)u8"͸��###͸��ҳ��"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"���͸������"))
		{
			ButtonToggle((const char*)u8"���͸��������", &Settings::playerSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###���͸�Ӿ���", &Settings::playerSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"���ƴ�����", &Settings::playerSettings.drawAlive);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###��������ɫ", Settings::playerSettings.aliveColor);
			Utils::ValidateRgb(Settings::playerSettings.aliveColor);

			ButtonToggle((const char*)u8"���Ƶ������", &Settings::playerSettings.drawDowned);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###���������ɫ", Settings::playerSettings.downedColor);
			Utils::ValidateRgb(Settings::playerSettings.downedColor);

			ButtonToggle((const char*)u8"�����������", &Settings::playerSettings.drawDead);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###���������ɫ", Settings::playerSettings.deadColor);
			Utils::ValidateRgb(Settings::playerSettings.deadColor);

			ButtonToggle((const char*)u8"����δ֪���", &Settings::playerSettings.drawUnknown);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###δ֪�����ɫ", Settings::playerSettings.unknownColor);
			Utils::ValidateRgb(Settings::playerSettings.unknownColor);

			ButtonToggle((const char*)u8"�������������", &Settings::playerSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###��������Ҳ�͸����", &Settings::playerSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ������", &Settings::playerSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ�����Ҳ�͸����", &Settings::playerSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�������������", &Settings::playerSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�������������", &Settings::playerSettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾ�������", &Settings::playerSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾ��Ҿ���", &Settings::playerSettings.showDistance);

			ButtonToggle((const char*)u8"��ʾ���Ѫ��", &Settings::playerSettings.showHealth);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾ�������Ѫ��", &Settings::playerSettings.showDeadHealth);

			ButtonToggle((const char*)u8"�����Ϣ��Ӱ", &Settings::playerSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�����Ϣ����", &Settings::playerSettings.textCentered);
		}

		if (ImGui::CollapsingHeader((const char*)u8"NPC͸������"))
		{
			ButtonToggle((const char*)u8"NPC͸��������", &Settings::npcSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###NPC͸�Ӿ���", &Settings::npcSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"���ƴ��NPC", &Settings::npcSettings.drawAlive);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###���NPC��ɫ", Settings::npcSettings.aliveColor);
			Utils::ValidateRgb(Settings::npcSettings.aliveColor);

			ButtonToggle((const char*)u8"���Ƶ���NPC", &Settings::npcSettings.drawDowned);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###����NPC��ɫ", Settings::npcSettings.downedColor);
			Utils::ValidateRgb(Settings::npcSettings.downedColor);

			ButtonToggle((const char*)u8"��������NPC", &Settings::npcSettings.drawDead);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###����NPC��ɫ", Settings::npcSettings.deadColor);
			Utils::ValidateRgb(Settings::npcSettings.deadColor);

			ButtonToggle((const char*)u8"����δ֪NPC", &Settings::npcSettings.drawUnknown);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###δ֪NPC��ɫ", Settings::npcSettings.unknownColor);
			Utils::ValidateRgb(Settings::npcSettings.unknownColor);

			ButtonToggle((const char*)u8"����������NPC", &Settings::npcSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###������NPC��͸����", &Settings::npcSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ���NPC", &Settings::npcSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ���NPC��͸����", &Settings::npcSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"����������NPC", &Settings::npcSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����������NPC", &Settings::npcSettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾNPC����", &Settings::npcSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾNPC����", &Settings::npcSettings.showDistance);

			ButtonToggle((const char*)u8"��ʾNPCѪ��", &Settings::npcSettings.showHealth);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾ����NPCѪ��", &Settings::npcSettings.showDeadHealth);

			ButtonToggle((const char*)u8"NPC��Ϣ��Ӱ", &Settings::npcSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"NPC��Ϣ����", &Settings::npcSettings.textCentered);

			ButtonToggle((const char*)u8"���ǻ���һ�Ǵ���NPC", &Settings::customLegendarySettings.overrideLivingOneStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###���һ�Ǵ�����ɫ", Settings::customLegendarySettings.livingOneStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.livingOneStarColor);

			ButtonToggle((const char*)u8"���ǻ�������һ�Ǵ���NPC", &Settings::customLegendarySettings.overrideDeadOneStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###����һ�Ǵ�����ɫ", Settings::customLegendarySettings.deadOneStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.deadOneStarColor);

			ButtonToggle((const char*)u8"���ǻ��ƶ��Ǵ���NPC", &Settings::customLegendarySettings.overrideLivingTwoStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###�����Ǵ�����ɫ", Settings::customLegendarySettings.livingTwoStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.livingTwoStarColor);

			ButtonToggle((const char*)u8"���ǻ����������Ǵ���NPC", &Settings::customLegendarySettings.overrideDeadTwoStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###�������Ǵ�����ɫ", Settings::customLegendarySettings.deadTwoStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.deadTwoStarColor);

			ButtonToggle((const char*)u8"���ǻ������Ǵ���NPC", &Settings::customLegendarySettings.overrideLivingThreeStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###������Ǵ�����ɫ", Settings::customLegendarySettings.livingThreeStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.livingThreeStarColor);

			ButtonToggle((const char*)u8"���ǻ����������Ǵ���NPC", &Settings::customLegendarySettings.overrideDeadThreeStar);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###�������Ǵ�����ɫ", Settings::customLegendarySettings.deadThreeStarColor);
			Utils::ValidateRgb(Settings::customLegendarySettings.deadThreeStarColor);

			LargeButtonToggle((const char*)u8"���ػ����NPC", &Settings::customExtraNpcSettings.hideSettlerFaction);
			LargeButtonToggle((const char*)u8"���ػ�ɽ��NPC", &Settings::customExtraNpcSettings.hideCraterRaiderFaction);
			LargeButtonToggle((const char*)u8"������Ӳ��NPC", &Settings::customExtraNpcSettings.hideDieHardFaction);
			LargeButtonToggle((const char*)u8"�������ܷ���NPC", &Settings::customExtraNpcSettings.hideSecretServiceFaction);

			LargeButtonToggle((const char*)u8"NPC������������", &Settings::customExtraNpcSettings.useNpcBlacklist);
			if (ImGui::CollapsingHeader((const char*)u8"NPC������"))
			{
				for (auto i = 0; i < 64; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"NPC������: {:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::customExtraNpcSettings.npcBlacklistEnabled[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###NPC������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::customExtraNpcSettings.npcBlacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"����͸������"))
		{
			ButtonToggle((const char*)u8"����͸��������", &Settings::containerSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###�������þ���", &Settings::containerSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###������ɫ", Settings::containerSettings.color);
			Utils::ValidateRgb(Settings::containerSettings.color);

			ButtonToggle((const char*)u8"��������������", &Settings::containerSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###������������͸����", &Settings::containerSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ�������", &Settings::containerSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ���������͸����", &Settings::containerSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"��������������", &Settings::containerSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��������������", &Settings::containerSettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾ��������", &Settings::containerSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾ��������", &Settings::containerSettings.showDistance);

			ButtonToggle((const char*)u8"������Ϣ��Ӱ", &Settings::containerSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"������Ϣ����", &Settings::containerSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"��������������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"����������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::containerSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###����������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::containerSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"����͸������"))
		{
			ButtonToggle((const char*)u8"����͸��������", &Settings::junkSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###����͸�Ӿ���", &Settings::junkSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###������ɫ", Settings::junkSettings.color);
			Utils::ValidateRgb(Settings::junkSettings.color);

			ButtonToggle((const char*)u8"��������������", &Settings::junkSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###������������͸����", &Settings::junkSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			Utils::ValidateFloat(Settings::junkSettings.enabledAlpha, 0.0f, 1.0f);

			ButtonToggle((const char*)u8"�����ѽ�������", &Settings::junkSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ���������͸����", &Settings::junkSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"��������������", &Settings::junkSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��������������", &Settings::junkSettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾ��������", &Settings::junkSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾ��������", &Settings::junkSettings.showDistance);

			ButtonToggle((const char*)u8"������Ϣ��Ӱ", &Settings::junkSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"������Ϣ����", &Settings::junkSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"��������������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"����������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::junkSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###����������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::junkSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"ͼֽ͸������"))
		{
			ButtonToggle((const char*)u8"ͼֽ͸��������", &Settings::planSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###ͼֽ͸�Ӿ���", &Settings::planSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###ͼֽ��ɫ", Settings::planSettings.color);
			Utils::ValidateRgb(Settings::planSettings.color);

			ButtonToggle((const char*)u8"����������ͼֽ", &Settings::planSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###������ͼֽ��͸����", &Settings::planSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ���ͼֽ", &Settings::planSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ���ͼֽ��͸����", &Settings::planSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"������֪ͼֽ", &Settings::recipes.knownRecipesEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����δ֪ͼֽ", &Settings::recipes.unknownRecipesEnabled);

			ButtonToggle((const char*)u8"����������ͼֽ", &Settings::planSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����������ͼֽ", &Settings::planSettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾͼֽ����", &Settings::planSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾͼֽ����", &Settings::planSettings.showDistance);

			ButtonToggle((const char*)u8"ͼֽ��Ϣ��Ӱ", &Settings::planSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"ͼֽ��Ϣ����", &Settings::planSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"ͼֽ����������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"ͼֽ������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::planSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###ͼֽ������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::planSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"��־͸������"))
		{
			ButtonToggle((const char*)u8"��־͸��������", &Settings::magazineSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###��־͸�Ӿ���", &Settings::magazineSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###��־��ɫ", Settings::magazineSettings.color);
			Utils::ValidateRgb(Settings::magazineSettings.color);

			ButtonToggle((const char*)u8"������������־", &Settings::magazineSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###��������־��͸����", &Settings::magazineSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ�����־", &Settings::magazineSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ�����־��͸����", &Settings::magazineSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"������������־", &Settings::magazineSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"������������־", &Settings::magazineSettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾ��־����", &Settings::magazineSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾ��־����", &Settings::magazineSettings.showDistance);

			ButtonToggle((const char*)u8"��־��Ϣ��Ӱ", &Settings::magazineSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��־��Ϣ����", &Settings::magazineSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"��־����������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"��־������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::magazineSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###��־������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::magazineSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"����͸������"))
		{
			ButtonToggle((const char*)u8"����͸��������", &Settings::bobbleheadSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###����͸�Ӿ���", &Settings::bobbleheadSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###������ɫ", Settings::bobbleheadSettings.color);
			Utils::ValidateRgb(Settings::bobbleheadSettings.color);

			ButtonToggle((const char*)u8"��������������", &Settings::bobbleheadSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###���������޲�͸����", &Settings::bobbleheadSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ�������", &Settings::bobbleheadSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ������޲�͸����", &Settings::bobbleheadSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"��������������", &Settings::bobbleheadSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��������������", &Settings::bobbleheadSettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾ��������", &Settings::bobbleheadSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾ���޾���", &Settings::bobbleheadSettings.showDistance);

			ButtonToggle((const char*)u8"������Ϣ��Ӱ", &Settings::bobbleheadSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"������Ϣ����", &Settings::bobbleheadSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"���ް���������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"���ް�����λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::bobbleheadSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###���ް�����{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::bobbleheadSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"��Ʒ͸������"))
		{
			ButtonToggle((const char*)u8"��Ʒ͸��������", &Settings::itemSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###��Ʒ͸�Ӿ���", &Settings::itemSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###��Ʒ��ɫ", Settings::itemSettings.color);
			Utils::ValidateRgb(Settings::itemSettings.color);

			ButtonToggle((const char*)u8"������������Ʒ", &Settings::itemSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###��������Ʒ��͸����", &Settings::itemSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ�����Ʒ", &Settings::itemSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ�����Ʒ��͸����", &Settings::itemSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"������������Ʒ", &Settings::itemSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"������������Ʒ", &Settings::itemSettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾ��Ʒ����", &Settings::itemSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾ��Ʒ����", &Settings::itemSettings.showDistance);

			ButtonToggle((const char*)u8"��Ʒ��Ϣ��Ӱ", &Settings::itemSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��Ʒ��Ϣ����", &Settings::itemSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"��Ʒ����������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"��Ʒ������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::itemSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###��Ʒ������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::itemSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"ֲ��Ⱥϵ͸������"))
		{
			ButtonToggle((const char*)u8"ֲ��Ⱥϵ͸��������", &Settings::floraSettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###ֲ��Ⱥϵ͸�Ӿ���", &Settings::floraSettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###ֲ��Ⱥϵ��ɫ", Settings::floraSettings.color);
			Utils::ValidateRgb(Settings::floraSettings.color);

			ButtonToggle((const char*)u8"����������ֲ��Ⱥϵ", &Settings::floraSettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###������ֲ��Ⱥϵ��͸����", &Settings::floraSettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ���ֲ��Ⱥϵ", &Settings::floraSettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ���ֲ��Ⱥϵ��͸����", &Settings::floraSettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"����������ֲ��Ⱥϵ", &Settings::floraSettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����������ֲ��Ⱥϵ", &Settings::floraSettings.drawUnnamed);

			LargeButtonToggle((const char*)u8"�����ɺ�ɫԭ���ܼ�", &Settings::customFluxSettings.crimsonFluxEnabled);
			LargeButtonToggle((const char*)u8"��������ɫԭ���ܼ�", &Settings::customFluxSettings.cobaltFluxEnabled);
			LargeButtonToggle((const char*)u8"���ƻƱ�ԭ���ܼ�", &Settings::customFluxSettings.yellowcakeFluxEnabled);
			LargeButtonToggle((const char*)u8"����ӫ��ԭ���ܼ�", &Settings::customFluxSettings.fluorescentFluxEnabled);
			LargeButtonToggle((const char*)u8"������ɫԭ���ܼ�", &Settings::customFluxSettings.violetFluxEnabled);

			ButtonToggle((const char*)u8"��ʾֲ��Ⱥϵ����", &Settings::floraSettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʾֲ��Ⱥϵ����", &Settings::floraSettings.showDistance);

			ButtonToggle((const char*)u8"ֲ��Ⱥϵ��Ϣ��Ӱ", &Settings::floraSettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"ֲ��Ⱥϵ��Ϣ����", &Settings::floraSettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"ֲ��Ⱥϵ����������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"ֲ��Ⱥϵ������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::floraSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###ֲ��Ⱥϵ������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::floraSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"ʵ��͸������"))
		{
			ButtonToggle((const char*)u8"ʵ��͸��������", &Settings::entitySettings.enabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###ʵ��͸�Ӿ���", &Settings::entitySettings.enabledDistance, 0, 3000, (const char*)u8"����: %d");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::ColorEdit3((const char*)u8"###ʵ����ɫ", Settings::entitySettings.color);
			Utils::ValidateRgb(Settings::entitySettings.color);

			ButtonToggle((const char*)u8"����������ʵ��", &Settings::entitySettings.drawEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###������ʵ�岻͸����", &Settings::entitySettings.enabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"�����ѽ���ʵ��", &Settings::entitySettings.drawDisabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�ѽ���ʵ�岻͸����", &Settings::entitySettings.disabledAlpha, 0.0f, 1.0f, (const char*)u8"��͸����: %.2f");

			ButtonToggle((const char*)u8"����������ʵ��", &Settings::entitySettings.drawNamed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����������ʵ��", &Settings::entitySettings.drawUnnamed);

			ButtonToggle((const char*)u8"��ʾʵ������", &Settings::entitySettings.showName);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��ʵʵ�����", &Settings::entitySettings.showDistance);

			ButtonToggle((const char*)u8"ʵ����Ϣ��Ӱ", &Settings::entitySettings.textShadowed);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"ʵ����Ϣ����", &Settings::entitySettings.textCentered);

			if (ImGui::CollapsingHeader((const char*)u8"ʵ�����������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"ʵ�������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::floraSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###ʵ�������{:d}", i);
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
	if (ImGui::BeginTabItem((const char*)u8"��Ϣ����###��Ϣ����ҳ��"))
	{
		LargeButtonToggle((const char*)u8"�����Զ������ѹ�״̬", &Settings::infobox.drawScrapLooterStatus);
		LargeButtonToggle((const char*)u8"�����Զ���Ʒ�ѹ�״̬", &Settings::infobox.drawItemLooterStatus);
		LargeButtonToggle((const char*)u8"����NPC�ѹ�״̬", &Settings::infobox.drawNpcLooterStatus);
		LargeButtonToggle((const char*)u8"���������ѹ�״̬", &Settings::infobox.drawContainerLooterStatus);
		LargeButtonToggle((const char*)u8"����ֲ��Ⱥϵ�ջ�״̬", &Settings::infobox.drawHarvesterStatus);
		LargeButtonToggle((const char*)u8"����λ����Ϣ", &Settings::infobox.drawPositionSpoofingStatus);
		LargeButtonToggle((const char*)u8"���ƺ˵�����", &Settings::infobox.drawNukeCodes);

		ImGui::EndTabItem();
	}
}
void Gui::OverlayMenuTabLoot()
{
	if (ImGui::BeginTabItem((const char*)u8"�ѹ�###�ѹ�ҳ��"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"�����ѹ�"))
		{
			if (ErectusMemory::CheckScrapList())
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
				if (ImGui::Button((const char*)u8"�����ѹ� (��ݼ�: CTRL+E)###�ѹ�ѡ������������", ImVec2(224.0f, 0.0f)))
					ErectusMemory::LootScrap();
				ImGui::PopStyleColor(3);
			}
			else
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
				ImGui::Button((const char*)u8"�����ѹ� (��ݼ�: CTRL+E)###�ѹ�ѡ�в����ѽ���", ImVec2(224.0f, 0.0f));
				ImGui::PopStyleColor(3);
				ImGui::PopItemFlag();
			}

			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�����ѹο�ݼ�������", &Settings::scrapLooter.keybindEnabled);

			ButtonToggle((const char*)u8"ʹ������͸������", &Settings::scrapLooter.scrapOverrideEnabled);

			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�Զ��ѹ�������###�����Զ��ѹ�������", &Settings::scrapLooter.autoLootingEnabled);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"�ٶ� (��С): {0:d} ({1:d} ����)", Settings::scrapLooter.autoLootingSpeedMin, Settings::scrapLooter.autoLootingSpeedMin * 16);
				if (ImGui::SliderInt((const char*)u8"###�����ѹ���С�ٶ�", &Settings::scrapLooter.autoLootingSpeedMin, 10, 60, sliderText.c_str()))
				{
					if (Settings::scrapLooter.autoLootingSpeedMax < Settings::scrapLooter.autoLootingSpeedMin)
						Settings::scrapLooter.autoLootingSpeedMax = Settings::scrapLooter.autoLootingSpeedMin;
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"�ٶ� (���): {0:d} ({1:d} ����)", Settings::scrapLooter.autoLootingSpeedMax, Settings::scrapLooter.autoLootingSpeedMax * 16);
				if (ImGui::SliderInt((const char*)u8"###�����ѹ�����ٶ�", &Settings::scrapLooter.autoLootingSpeedMax, 10, 60, sliderText.c_str()))
				{
					if (Settings::scrapLooter.autoLootingSpeedMax < Settings::scrapLooter.autoLootingSpeedMin)
						Settings::scrapLooter.autoLootingSpeedMin = Settings::scrapLooter.autoLootingSpeedMax;
				}
			}

			ImGui::SetNextItemWidth(451.0f);
			ImGui::SliderInt((const char*)u8"###�����ѹξ���", &Settings::scrapLooter.maxDistance, 1, 3000, (const char*)u8"�����ѹξ���: %d");

			for (auto i = 0; i < 40; i++)
			{
				ButtonToggle(Settings::scrapLooter.nameList[i], &Settings::scrapLooter.enabledList[i]);

				ImGui::SameLine(235.0f);
				ImGui::SetNextItemWidth(224.0f);

				auto inputLabel = fmt::format((const char*)u8"###����ֻ��{:d}", i);
				auto inputText = fmt::format((const char*)u8"{:08X}", Settings::scrapLooter.formIdList[i]);
				ImGui::InputText(inputLabel.c_str(), &inputText, ImGuiInputTextFlags_ReadOnly);
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"��Ʒ�ѹ�"))
		{
			if (ErectusMemory::CheckItemLooterSettings())
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
				if (ImGui::Button((const char*)u8"��Ʒ�ѹ� (��ݼ�: CTRL+R)###�ѹ�ѡ����Ʒ������", ImVec2(224.0f, 0.0f)))
					ErectusMemory::LootItems();
				ImGui::PopStyleColor(3);
			}
			else
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));
				ImGui::Button((const char*)u8"��Ʒ�ѹ� (��ݼ�: CTRL+R)###�ѹ�ѡ����Ʒ�ѽ���", ImVec2(224.0f, 0.0f));
				ImGui::PopStyleColor(3);
				ImGui::PopItemFlag();
			}

			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��Ʒ�ѹο�ݼ�������", &Settings::itemLooter.keybindEnabled);

			LargeButtonToggle((const char*)u8"�Զ��ѹ�������###��Ʒ�Զ��ѹ�������", &Settings::itemLooter.autoLootingEnabled);
			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"�ٶ� (��С): {0:d} ({1:d} ����)", Settings::itemLooter.autoLootingSpeedMin, Settings::itemLooter.autoLootingSpeedMin * 16);
				if (ImGui::SliderInt((const char*)u8"###��Ʒ�ѹ���С�ٶ�", &Settings::itemLooter.autoLootingSpeedMin, 10, 60, sliderText.c_str()))
				{
					if (Settings::itemLooter.autoLootingSpeedMax < Settings::itemLooter.autoLootingSpeedMin)
						Settings::itemLooter.autoLootingSpeedMax = Settings::itemLooter.autoLootingSpeedMin;
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"�ٶ� (���): {0:d} ({1:d} ����)", Settings::itemLooter.autoLootingSpeedMax, Settings::itemLooter.autoLootingSpeedMax * 16);
				if (ImGui::SliderInt((const char*)u8"###��Ʒ�ѹ�����ٶ�", &Settings::itemLooter.autoLootingSpeedMax, 10, 60, sliderText.c_str()))
				{
					if (Settings::itemLooter.autoLootingSpeedMax < Settings::itemLooter.autoLootingSpeedMin)
						Settings::itemLooter.autoLootingSpeedMin = Settings::itemLooter.autoLootingSpeedMax;
				}
			}

			ButtonToggle((const char*)u8"����������###�����ѹ�������", &Settings::itemLooter.lootWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###�����ѹξ���", &Settings::itemLooter.lootWeaponsDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"װ��������###װ���ѹ�������", &Settings::itemLooter.lootArmorEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###װ���ѹξ���", &Settings::itemLooter.lootArmorDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"��ҩ������###��ҩ�ѹ�������", &Settings::itemLooter.lootAmmoEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###��ҩ�ѹξ���", &Settings::itemLooter.lootAmmoDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"��װ��������###��װ���ѹ�������", &Settings::itemLooter.lootModsEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###��װ���ѹξ���", &Settings::itemLooter.lootModsDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"��־������###��־�ѹ�������", &Settings::itemLooter.lootMagazinesEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###��־�ѹξ���", &Settings::itemLooter.lootMagazinesDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"����������###�����ѹ�������", &Settings::itemLooter.lootBobbleheadsEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###�����ѹξ���", &Settings::itemLooter.lootBobbleheadsDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"����Ʒ������###����Ʒ�ѹ�������", &Settings::itemLooter.lootAidEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###����Ʒ�ѹξ���", &Settings::itemLooter.lootAidDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"��֪ͼֽ������###��֪ͼֽ�ѹ�������", &Settings::itemLooter.lootKnownPlansEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###��֪ͼֽ�ѹξ���", &Settings::itemLooter.lootKnownPlansDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"δ֪ͼֽ������###δ֪ͼֽ�ѹ�������", &Settings::itemLooter.lootUnknownPlansEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###δ֪ͼֽ�ѹξ���", &Settings::itemLooter.lootUnknownPlansDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"����������###�����ѹ�������", &Settings::itemLooter.lootMiscEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###�����ѹξ���", &Settings::itemLooter.lootMiscDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"����������###�����ѹ�������", &Settings::itemLooter.lootUnlistedEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###�����ѹξ���", &Settings::itemLooter.lootUnlistedDistance, 0, 3000, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"��ƷFormId�б�������###��Ʒ�ѹ��б�������", &Settings::itemLooter.lootListEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###��Ʒ�ѹ��б����", &Settings::itemLooter.lootListDistance, 0, 3000, (const char*)u8"����: %d");

			LargeButtonToggle((const char*)u8"��Ʒ�ѹκ�����������###����/������Ʒ�ѹκ�����", &Settings::itemLooter.blacklistToggle);

			if (ImGui::CollapsingHeader((const char*)u8"��Ʒ�ѹ�FormId�б�"))
			{
				for (auto i = 0; i < 100; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"��Ʒ�ѹ�λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::itemLooter.enabledList[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###��Ʒ�ѹ��б�{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::itemLooter.formIdList[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}

			if (ImGui::CollapsingHeader((const char*)u8"��Ʒ�ѹκ�����"))
			{
				for (auto i = 0; i < 64; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"��Ʒ�ѹκ�����: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::itemLooter.blacklistEnabled[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###��Ʒ�ѹκ�����{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::itemLooter.blacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"NPC�ѹ� (76m ��������)"))
		{
			LargeButtonToggle((const char*)u8"�Զ�NPC�ѹ������� (��ݼ�: CTRL+COMMA)###NPC�ѹ�������", &Settings::npcLooter.enabled);

			ButtonToggle((const char*)u8"��������������###NPC�ѹ���������������", &Settings::npcLooter.allWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����װ��������###NPC�ѹ�����װ��������", &Settings::npcLooter.allArmorEnabled);

			ButtonToggle((const char*)u8"һ�Ǵ�������������###NPC�ѹ�һ�Ǵ�������������", &Settings::npcLooter.oneStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"һ�Ǵ���װ��������###NPC�ѹ�һ�Ǵ���װ��������", &Settings::npcLooter.oneStarArmorEnabled);

			ButtonToggle((const char*)u8"���Ǵ�������������###NPC�ѹζ��Ǵ�������������", &Settings::npcLooter.twoStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"���Ǵ���װ��������###NPC�ѹζ��Ǵ���װ��������", &Settings::npcLooter.twoStarArmorEnabled);

			ButtonToggle((const char*)u8"���Ǵ�������������###NPC�ѹ����Ǵ�������������", &Settings::npcLooter.threeStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"���Ǵ���װ��������###NPC�ѹ����Ǵ���װ��������", &Settings::npcLooter.threeStarArmorEnabled);

			ButtonToggle((const char*)u8"��ҩ������###NPC�ѹε�ҩ������", &Settings::npcLooter.ammoEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��װ��������###NPC�ѹθ�װ��������", &Settings::npcLooter.modsEnabled);

			ButtonToggle((const char*)u8"ƿ��������###NPC�ѹ�ƿ��������", &Settings::npcLooter.capsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����������###NPC�ѹ�����������", &Settings::npcLooter.junkEnabled);

			ButtonToggle((const char*)u8"����Ʒ������###NPC�ѹθ���Ʒ������", &Settings::npcLooter.aidEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�ر�ͼ������###NPC�ѹβر�ͼ������", &Settings::npcLooter.treasureMapsEnabled);

			ButtonToggle((const char*)u8"��֪ͼֽ������###NPC��֪ͼֽ�ѹ�������", &Settings::npcLooter.knownPlansEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"δ֪ͼֽ������###NPCδ֪ͼֽ�ѹ�������", &Settings::npcLooter.unknownPlansEnabled);

			ButtonToggle((const char*)u8"����������###NPC�����ѹ�������", &Settings::npcLooter.miscEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����������###NPC�����ѹ�������", &Settings::npcLooter.unlistedEnabled);

			LargeButtonToggle((const char*)u8"NPC�ѹ�FormId�б�������###NPC�ѹ��б�������", &Settings::npcLooter.listEnabled);
			LargeButtonToggle((const char*)u8"NPC�ѹκ�����������###����/����NPC�ѹκ�����", &Settings::npcLooter.blacklistToggle);

			if (ImGui::CollapsingHeader((const char*)u8"NPC�ѹ�FormId�б�"))
			{
				for (auto i = 0; i < 100; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"NPC�ѹ�λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::npcLooter.enabledList[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###NPC�ѹ��б�{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::npcLooter.formIdList[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}

			if (ImGui::CollapsingHeader((const char*)u8"NPC�ѹκ�����"))
			{
				for (auto i = 0; i < 64; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"NPC�ѹκ�����: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::npcLooter.blacklistEnabled[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###NPC�ѹκ�����{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::npcLooter.blacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"�����ѹ� (6m ��������)"))
		{
			LargeButtonToggle((const char*)u8"�Զ������ѹ������� (��ݼ�: CTRL+PERIOD)###�����ѹ�������", &Settings::containerLooter.enabled);

			ButtonToggle((const char*)u8"��������������###�����ѹ���������������", &Settings::containerLooter.allWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����װ��������###�����ѹ�����װ��������", &Settings::containerLooter.allArmorEnabled);

			ButtonToggle((const char*)u8"һ�Ǵ�������������###�����ѹ�һ�Ǵ�������������", &Settings::containerLooter.oneStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"һ�Ǵ��滤��������###�����ѹ�һ�Ǵ��滤��������", &Settings::containerLooter.oneStarArmorEnabled);

			ButtonToggle((const char*)u8"���Ǵ�������������###�����ѹζ��Ǵ�������������", &Settings::containerLooter.twoStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"���Ǵ��滤��������###�����ѹζ��Ǵ��滤��������", &Settings::containerLooter.twoStarArmorEnabled);

			ButtonToggle((const char*)u8"���Ǵ�������������###�����ѹ����Ǵ�������������", &Settings::containerLooter.threeStarWeaponsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"���Ǵ��滤��������###�����ѹ����Ǵ��滤��������", &Settings::containerLooter.threeStarArmorEnabled);

			ButtonToggle((const char*)u8"��ҩ������###�����ѹε�ҩ������", &Settings::containerLooter.ammoEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"��װ��������###�����ѹθ�װ��������", &Settings::containerLooter.modsEnabled);

			ButtonToggle((const char*)u8"ƿ��������###�����ѹ�ƿ��������", &Settings::containerLooter.capsEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����������###�����ѹ�����������", &Settings::containerLooter.junkEnabled);

			ButtonToggle((const char*)u8"����Ʒ������###�����ѹθ���Ʒ������", &Settings::containerLooter.aidEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�ر�ͼ������###�����ѹβر�ͼ������", &Settings::containerLooter.treasureMapsEnabled);

			ButtonToggle((const char*)u8"��֪ͼֽ������###�����ѹ���֪ͼֽ������", &Settings::containerLooter.knownPlansEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"δ֪ͼֽ������###�����ѹ�δ֪ͼֽ������", &Settings::containerLooter.unknownPlansEnabled);

			ButtonToggle((const char*)u8"����������###�����ѹ�����������", &Settings::containerLooter.miscEnabled);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����������###�����ѹ�����������", &Settings::containerLooter.unlistedEnabled);

			LargeButtonToggle((const char*)u8"�����ѹ�FormId�б�������###�����ѹ��б�������", &Settings::containerLooter.listEnabled);

			LargeButtonToggle((const char*)u8"�����ѹκ�����������###����/���������ѹκ�����", &Settings::containerLooter.blacklistToggle);

			if (ImGui::CollapsingHeader((const char*)u8"�����ѹ�FormId�б�"))
			{
				for (auto i = 0; i < 100; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"�����ѹ�λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::containerLooter.enabledList[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###�����ѹ��б�{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::containerLooter.formIdList[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}

			if (ImGui::CollapsingHeader((const char*)u8"�����ѹκ�����"))
			{
				for (auto i = 0; i < 64; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"�����ѹκ�����: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::containerLooter.blacklistEnabled[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###�����ѹκ�����{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::containerLooter.blacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"ֲ��Ⱥϵ�ջ� (6m ��������)"))
		{
			LargeButtonToggle((const char*)u8"�Զ�ֲ��Ⱥϵ�ջ������� (��ݼ�: CTRL+P])###ֲ��Ⱥϵ�ջ�������", &Settings::harvester.enabled);
			LargeButtonToggle((const char*)u8"ֲ��Ⱥϵ�ջ�Խ������ (ʹ��ֲ��Ⱥϵ͸������)", &Settings::harvester.overrideEnabled);

			for (auto i = 0; i < 69; i++)
			{
				ButtonToggle(Settings::harvester.nameList[i], &Settings::harvester.enabledList[i]);

				ImGui::SameLine(235.0f);
				ImGui::SetNextItemWidth(224.0f);

				auto inputLabel = fmt::format((const char*)u8"###ֲ��Ⱥϵֻ��{:d}", i);
				auto inputText = fmt::format((const char*)u8"{:08X}", Settings::harvester.formIdList[i]);
				ImGui::InputText(inputLabel.c_str(), &inputText, ImGuiInputTextFlags_ReadOnly);
			}
		}

		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabCombat()
{
	if (ImGui::BeginTabItem((const char*)u8"ս��###ս��ҳ��"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"�����༭��"))
		{
			ButtonToggle((const char*)u8"�޺�����", &Settings::weapons.noRecoil);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�����ӵ�", &Settings::weapons.infiniteAmmo);

			ButtonToggle((const char*)u8"����ɢ", &Settings::weapons.noSpread);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"˲�任��", &Settings::weapons.instantReload);

			ButtonToggle((const char*)u8"�޻ζ�", &Settings::weapons.noSway);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"ȫ�Զ���ʶ###����ȫ�Զ�", &Settings::weapons.automaticflag);

			ButtonToggle((const char*)u8"��ϻ����###������ϻ����������", &Settings::weapons.capacityEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###������ϻ����", &Settings::weapons.capacity, 0, 999, (const char*)u8"��ϻ����: %d");

			ButtonToggle((const char*)u8"����###��������������", &Settings::weapons.speedEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###��������", &Settings::weapons.speed, 0.0f, 100.0f, (const char*)u8"����: %.2f");

			ButtonToggle((const char*)u8"���###�������������", &Settings::weapons.reachEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###�������", &Settings::weapons.reach, 0.0f, 999.0f, (const char*)u8"���: %.2f");
		}

		if (ImGui::CollapsingHeader((const char*)u8"Ŀ������"))
		{
			ButtonToggle((const char*)u8"������� (��ݼ�: T)", &Settings::targetting.lockPlayers);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"����NPC (��ݼ�: T)", &Settings::targetting.lockNpCs);

			ButtonToggle((const char*)u8"�˺��ض��� (���)", &Settings::targetting.indirectPlayers);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�˺��ض��� (NPC)", &Settings::targetting.indirectNpCs);

			ButtonToggle((const char*)u8"�����˺� (���)", &Settings::targetting.directPlayers);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�����˺� (NPC)", &Settings::targetting.directNpCs);

			SmallButtonToggle((const char*)u8"���###�������", &Settings::targetting.targetLiving);
			ImGui::SameLine(122.0f);
			SmallButtonToggle((const char*)u8"����###��������", &Settings::targetting.targetDowned);
			ImGui::SameLine(235.0f);
			SmallButtonToggle((const char*)u8"����###��������", &Settings::targetting.targetDead);
			ImGui::SameLine(349.0f);
			SmallButtonToggle((const char*)u8"δ֪###����δ֪", &Settings::targetting.targetUnknown);

			ButtonToggle((const char*)u8"���Գ�Զ����###���Գ�Զ����", &Settings::targetting.ignoreRenderDistance);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderFloat((const char*)u8"###Ŀ�������Ƕ�", &Settings::targetting.lockingFov, 5.0f, 40.0f, (const char*)u8"�����Ƕ�: %.2f");

			ButtonToggle((const char*)u8"���Ի���NPC###���Ի���NPC", &Settings::targetting.ignoreEssentialNpCs);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::ColorEdit3((const char*)u8"###Ŀ��������ɫ", Settings::targetting.lockingColor);
			Utils::ValidateRgb(Settings::playerSettings.unknownColor);

			ButtonToggle((const char*)u8"�Զ��л�Ŀ��###Ŀ�������л�", &Settings::targetting.retargeting);

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"��ȴ: {0:d} ({1:d} ����)", Settings::targetting.cooldown, Settings::targetting.cooldown * 16);
				ImGui::SliderInt((const char*)u8"###Ŀ��������ȴ", &Settings::targetting.cooldown, 0, 120, sliderText.c_str());
			}

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"�����˺� (��С): {0:d} ({1:d} ����)", Settings::targetting.sendDamageMin, Settings::targetting.sendDamageMin * 16);
				if (ImGui::SliderInt((const char*)u8"###��С�����˺�", &Settings::targetting.sendDamageMin, 1, 60, sliderText.c_str()))
				{
					if (Settings::targetting.sendDamageMax < Settings::targetting.sendDamageMin)
						Settings::targetting.sendDamageMax = Settings::targetting.sendDamageMin;
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"�����˺� (���): {0:d} ({1:d} ����)", Settings::targetting.sendDamageMax, Settings::targetting.sendDamageMax * 16);
				if (ImGui::SliderInt((const char*)u8"###������˺�", &Settings::targetting.sendDamageMax, 1, 60, sliderText.c_str()))
				{
					if (Settings::targetting.sendDamageMax < Settings::targetting.sendDamageMin)
						Settings::targetting.sendDamageMin = Settings::targetting.sendDamageMax;
				}
			}

			{
				std::string favoritedWeaponsPreview = (const char*)u8"[?] δѡ������";
				if (Settings::targetting.favoriteIndex < 12)
				{
					favoritedWeaponsPreview = ErectusMemory::GetFavoritedWeaponText(BYTE(Settings::targetting.favoriteIndex));
					if (favoritedWeaponsPreview.empty())
					{
						favoritedWeaponsPreview = fmt::format((const char*)u8"[{}] ��Ч�������", ErectusMemory::FavoriteIndex2Slot(BYTE(Settings::targetting.favoriteIndex)));
					}
				}

				ImGui::SetNextItemWidth(451.0f);
				if (ImGui::BeginCombo((const char*)u8"###������������", favoritedWeaponsPreview.c_str()))
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

		if (ImGui::CollapsingHeader((const char*)u8"��ս����"))
		{
			LargeButtonToggle((const char*)u8"��ս������ (��ݼ�: U)", &Settings::melee.enabled);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"��ս�ٶ� (��С): {0:d} ({1:d} ����)", Settings::melee.speedMin, Settings::melee.speedMin * 16);
				if (ImGui::SliderInt((const char*)u8"###��С��ս�ٶ�", &Settings::melee.speedMin, 1, 60, sliderText.c_str()))
				{
					if (Settings::melee.speedMax < Settings::melee.speedMin)
						Settings::melee.speedMax = Settings::melee.speedMin;
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				auto sliderText = fmt::format((const char*)u8"��ս�ٶ� (���): {0:d} ({1:d} ����)", Settings::melee.speedMax, Settings::melee.speedMax * 16);
				if (ImGui::SliderInt((const char*)u8"###����ս�ٶ�", &Settings::melee.speedMax, 1, 60, sliderText.c_str()))
				{
					if (Settings::melee.speedMax < Settings::melee.speedMin)
						Settings::melee.speedMin = Settings::melee.speedMax;
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"һ����ɱ"))
		{
			LargeButtonToggle((const char*)u8"һ����ɱ��� (��ݼ�: CTRL+B)", &Settings::opk.playersEnabled);
			LargeButtonToggle((const char*)u8"һ����ɱNPC (��ݼ�: CTRL+N)", &Settings::opk.npcsEnabled);
		}

		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabPlayer()
{
	if (ImGui::BeginTabItem((const char*)u8"���###���ҳ��"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"�����������"))
		{
			ButtonToggle((const char*)u8"λ��α�� (��ݼ� CTRL+L)##�������λ��α��������", &Settings::customLocalPlayerSettings.positionSpoofingEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###�������λ��α��߶�", &Settings::customLocalPlayerSettings.positionSpoofingHeight, -524287, 524287, (const char*)u8"α��߶�: %d");

			ButtonToggle((const char*)u8"��ǽ (��ݼ� CTRL+Y)###��ǽ������", &Settings::customLocalPlayerSettings.noclipEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderFloat((const char*)u8"###��ǽ�ٶ�", &Settings::customLocalPlayerSettings.noclipSpeed, 0.0f, 2.0f, (const char*)u8"�ٶ�: %.5f");

			ButtonToggle((const char*)u8"�ͻ�������", &Settings::customLocalPlayerSettings.clientState);
			ImGui::SameLine(235.0f);
			ButtonToggle((const char*)u8"�Զ��ͻ�������", &Settings::customLocalPlayerSettings.automaticClientState);

			LargeButtonToggle((const char*)u8"�����ж�����###�����������AP����������", &Settings::customLocalPlayerSettings.freezeApEnabled);

			ButtonToggle((const char*)u8"�ж�����###��������ж�����������", &Settings::customLocalPlayerSettings.actionPointsEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###��������ж�����", &Settings::customLocalPlayerSettings.actionPoints, 0, 99999, (const char*)u8"�ж�����: %d");

			ButtonToggle((const char*)u8"����###�����������������", &Settings::customLocalPlayerSettings.strengthEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###�����������", &Settings::customLocalPlayerSettings.strength, 0, 99999, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"��֪###������Ҹ�֪������", &Settings::customLocalPlayerSettings.perceptionEnabled);
			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###������Ҹ�֪", &Settings::customLocalPlayerSettings.perception, 0, 99999, (const char*)u8"��֪: %d");

			ButtonToggle((const char*)u8"����###�����������������", &Settings::customLocalPlayerSettings.enduranceEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###�����������", &Settings::customLocalPlayerSettings.endurance, 0, 99999, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"����###�����������������", &Settings::customLocalPlayerSettings.charismaEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###�����������", &Settings::customLocalPlayerSettings.charisma, 0, 99999, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"����###�����������������", &Settings::customLocalPlayerSettings.intelligenceEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###�����������", &Settings::customLocalPlayerSettings.intelligence, 0, 99999, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"����###�����������������", &Settings::customLocalPlayerSettings.agilityEnabled);

			ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);

			ImGui::SliderInt((const char*)u8"###�����������", &Settings::customLocalPlayerSettings.agility, 0, 99999, (const char*)u8"����: %d");

			ButtonToggle((const char*)u8"����###�����������������", &Settings::customLocalPlayerSettings.luckEnabled);					ImGui::SameLine(235.0f);
			ImGui::SetNextItemWidth(224.0f);
			ImGui::SliderInt((const char*)u8"###�����������", &Settings::customLocalPlayerSettings.luck, 0, 99999, (const char*)u8"����: %d");
		}

		if (ImGui::CollapsingHeader((const char*)u8"��ɫ����"))
		{
			LargeButtonToggle((const char*)u8"��ɫ��۱༭��������###��ɫ�༭��������", &Settings::characterEditor.enabled);
			ImGui::SetNextItemWidth(451.0f);
			ImGui::SliderFloat((const char*)u8"###��ɫ����", &Settings::characterEditor.thin, 0.0f, 1.0f, (const char*)u8"��ɫ��� (����): %f");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::SliderFloat((const char*)u8"###��ɫ����", &Settings::characterEditor.muscular, 0.0f, 1.0f, (const char*)u8"��ɫ��� (����): %f");

			ImGui::SetNextItemWidth(451.0f);
			ImGui::SliderFloat((const char*)u8"###��ɫ��С", &Settings::characterEditor.large, 0.0f, 1.0f, (const char*)u8"��ɫ��� (��С): %f");
		}
		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabUtilities()
{
	if (ImGui::BeginTabItem((const char*)u8"����###����ҳ��"))
	{
		if (ImGui::CollapsingHeader((const char*)u8"����"))
		{
			ButtonToggle((const char*)u8"���Ʊ����������", &Settings::utilities.debugPlayer);

			ImGui::SameLine(235.0f);

			ButtonToggle((const char*)u8"͸��DEBUGģʽ", &Settings::utilities.debugEsp);

			{
				if (Settings::utilities.ptrFormId)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

					if (ImGui::Button((const char*)u8"��ȡָ��###��ȡָ��������", ImVec2(224.0f, 0.0f)))
						getPtrResult = ErectusMemory::GetPtr(Settings::utilities.ptrFormId);

					ImGui::PopStyleColor(3);
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::Button((const char*)u8"��ȡָ��###��ȡָ���ѽ���", ImVec2(224.0f, 0.0f));
					ImGui::PopItemFlag();

					ImGui::PopStyleColor(3);
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(80.0f);
				if (ImGui::InputScalar((const char*)u8"###ָ��FormId����", ImGuiDataType_U32, &Settings::utilities.ptrFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal))
					getPtrResult = 0;
			}

			ImGui::SameLine(318.0f);

			{
				ImGui::SetNextItemWidth(141.0f);
				auto inputText = fmt::format("{:16X}", getPtrResult);
				ImGui::InputText((const char*)u8"###ָ������", &inputText, ImGuiInputTextFlags_ReadOnly);
			}

			{
				if (Settings::utilities.addressFormId)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

					if (ImGui::Button((const char*)u8"��ȡ��ַ###��ȡ��ַ������", ImVec2(224.0f, 0.0f)))
						getAddressResult = ErectusMemory::GetAddress(Settings::utilities.addressFormId);

					ImGui::PopStyleColor(3);
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::Button((const char*)u8"��ȡ��ַ###��ȡ��ַ�ѽ���", ImVec2(224.0f, 0.0f));
					ImGui::PopItemFlag();

					ImGui::PopStyleColor(3);
				}
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(80.0f);

				if (ImGui::InputScalar((const char*)u8"###��ַFormId����", ImGuiDataType_U32, &Settings::utilities.addressFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal))
					getAddressResult = 0;
			}

			ImGui::SameLine(318.0f);

			{
				ImGui::SetNextItemWidth(141.0f);

				auto inputText = fmt::format("{:16X}", getAddressResult);
				ImGui::InputText((const char*)u8"###��ַָ������", &inputText, ImGuiInputTextFlags_ReadOnly);
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"���ñ༭��"))
		{
			ButtonToggle((const char*)u8"ԴFormId###�л�����ԴFormId", &swapperSourceToggle);

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				ImGui::InputScalar((const char*)u8"###����ԴFormId����", ImGuiDataType_U32, &Settings::swapper.sourceFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
			}

			ButtonToggle((const char*)u8"Ŀ��FormId###�л�����Ŀ��FormId", &swapperDestinationToggle);

			ImGui::SameLine(235.0f);

			{
				ImGui::SetNextItemWidth(224.0f);
				ImGui::InputScalar((const char*)u8"###����Ŀ��FormId����", ImGuiDataType_U32, &Settings::swapper.destinationFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
			}

			if (swapperSourceToggle && Settings::swapper.sourceFormId && swapperDestinationToggle && Settings::swapper.destinationFormId)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

				if (ImGui::Button((const char*)u8"�༭���� (��дĿ��)###�༭����������", ImVec2(451.0f, 0.0f)))
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
				ImGui::Button((const char*)u8"�༭���� (��дĿ��)###�༭�����ѽ���", ImVec2(451.0f, 0.0f));
				ImGui::PopItemFlag();

				ImGui::PopStyleColor(3);
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"��Ʒ����"))
		{
			SmallButtonToggle((const char*)u8"Դ###�л�Դ��ƷFormId", &transferSourceToggle);

			ImGui::SameLine(122.0f);

			{
				ImGui::SetNextItemWidth(110.0f);
				ImGui::InputScalar((const char*)u8"###Դ��ƷFormId����", ImGuiDataType_U32, &Settings::customTransferSettings.sourceFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
			}

			ImGui::SameLine(235.0f);

			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

				if (ImGui::Button((const char*)u8"��ȡ���###�������Դ��Ʒ", ImVec2(110.0f, 0.0f)))
					Settings::customTransferSettings.sourceFormId = ErectusMemory::GetLocalPlayerFormId();

				ImGui::SameLine(349.0f);

				if (ImGui::Button((const char*)u8"��ȡ������###Դ��Ʒ������", ImVec2(110.0f, 0.0f)))
					Settings::customTransferSettings.sourceFormId = ErectusMemory::GetStashFormId();

				ImGui::PopStyleColor(3);
			}

			SmallButtonToggle((const char*)u8"Ŀ��###�л�Ŀ����ƷFormId", &transferDestinationToggle);

			ImGui::SameLine(122.0f);

			{
				ImGui::SetNextItemWidth(110.0f);
				ImGui::InputScalar((const char*)u8"###Ŀ����ƷFormId����", ImGuiDataType_U32, &Settings::customTransferSettings.destinationFormId,
					nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
			}

			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));
				ImGui::SameLine(235.0f);
				if (ImGui::Button((const char*)u8"��ȡ���###�������Ŀ����Ʒ", ImVec2(110.0f, 0.0f)))
					Settings::customTransferSettings.destinationFormId = ErectusMemory::GetLocalPlayerFormId();

				ImGui::SameLine(349.0f);
				if (ImGui::Button((const char*)u8"��ȡ������###Ŀ����Ʒ������", ImVec2(110.0f, 0.0f)))
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

				if (ImGui::Button((const char*)u8"������Ʒ###������Ʒ������", ImVec2(451.0f, 0.0f)))
					ErectusMemory::TransferItems(Settings::customTransferSettings.sourceFormId, Settings::customTransferSettings.destinationFormId);

				ImGui::PopStyleColor(3);
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.5f));

				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::Button((const char*)u8"������Ʒ###������Ʒ�ѽ���", ImVec2(451.0f, 0.0f));
				ImGui::PopItemFlag();

				ImGui::PopStyleColor(3);
			}

			LargeButtonToggle((const char*)u8"ʹ����Ʒ���������", &Settings::customTransferSettings.useWhitelist);
			LargeButtonToggle((const char*)u8"ʹ����Ʒ���������", &Settings::customTransferSettings.useBlacklist);

			if (ImGui::CollapsingHeader((const char*)u8"��Ʒ�������������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"���������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::customTransferSettings.whitelisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###��Ʒ���������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::customTransferSettings.whitelist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}

			if (ImGui::CollapsingHeader((const char*)u8"��Ʒ�������������"))
			{
				for (auto i = 0; i < 32; i++)
				{
					auto toggleLabel = fmt::format((const char*)u8"���������λ��: {0:d}", i);
					ButtonToggle(toggleLabel.c_str(), &Settings::customTransferSettings.blacklisted[i]);

					ImGui::SameLine(235.0f);
					ImGui::SetNextItemWidth(224.0f);

					auto inputLabel = fmt::format((const char*)u8"###��Ʒ���������{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::customTransferSettings.blacklist[i],
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}
			}
		}

		if (ImGui::CollapsingHeader((const char*)u8"�˵�����"))
		{
			ButtonToggle((const char*)u8"�Զ��˵�����", &Settings::customNukeCodeSettings.automaticNukeCodes);

			ImGui::SameLine(235.0f);

			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

				if (ImGui::Button((const char*)u8"��ȡ�˵�����", ImVec2(224.0f, 0.0f)))
					ErectusMemory::UpdateNukeCodes();

				ImGui::PopStyleColor(3);
			}

			auto text = format((const char*)u8"{} - A��", fmt::join(ErectusMemory::alphaCode, " "));
			ImGui::Text(text.c_str());

			text = format((const char*)u8"{} - B��", fmt::join(ErectusMemory::bravoCode, " "));
			ImGui::Text(text.c_str());

			text = format((const char*)u8"{} - C��", fmt::join(ErectusMemory::charlieCode, " "));
			ImGui::Text(text.c_str());
		}

		ImGui::EndTabItem();
	}
}

void Gui::OverlayMenuTabTeleporter()
{
	if (ImGui::BeginTabItem((const char*)u8"����###����ҳ��"))
	{
		for (auto i = 0; i < 16; i++)
		{
			auto teleportHeaderText = fmt::format((const char*)u8"����λ��: {0:d}", i);
			if (ImGui::CollapsingHeader(teleportHeaderText.c_str()))
			{
				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###����Ŀ��X{:d}", i);
					ImGui::InputFloat(inputLabel.c_str(), &Settings::teleporter.entries[i].destination[0]);
				}

				ImGui::SameLine(122.0f);

				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###����Ŀ��Y{:d}", i);
					ImGui::InputFloat(inputLabel.c_str(), &Settings::teleporter.entries[i].destination[1]);
				}

				ImGui::SameLine(235.0f);

				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###����Ŀ��Z{:d}", i);
					ImGui::InputFloat(inputLabel.c_str(), &Settings::teleporter.entries[i].destination[2]);
				}

				ImGui::SameLine(349.0f);

				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###����Ŀ��W{:d}", i);
					ImGui::InputFloat(inputLabel.c_str(), &Settings::teleporter.entries[i].destination[3]);
				}

				{
					ImGui::SetNextItemWidth(110.0f);
					auto inputLabel = fmt::format((const char*)u8"###��������FormId{:d}", i);
					ImGui::InputScalar(inputLabel.c_str(), ImGuiDataType_U32, &Settings::teleporter.entries[i].cellFormId,
						nullptr, nullptr, "%08lX", ImGuiInputTextFlags_CharsHexadecimal);
				}

				ImGui::SameLine(122.0f);

				{
					auto buttonLabel = fmt::format((const char*)u8"�趨λ��###����Ŀ��{:d}", i);
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
					auto buttonLabel = fmt::format((const char*)u8"����###���ô浵{:d}", i);
					SmallButtonToggle(buttonLabel.c_str(), &Settings::teleporter.entries[i].disableSaving);
				}

				ImGui::SameLine(349.0f);

				if (Settings::teleporter.entries[i].cellFormId)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.3f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.4f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.5f));

					auto buttonLabel = fmt::format((const char*)u8"����###��������������{:d}", i);
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

					auto buttonLabel = fmt::format((const char*)u8"����###���������ѽ���{:d}", i);
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
	if (ImGui::BeginTabItem((const char*)u8"λ��Ϣд��###λ��Ϣд��ҳ��"))
	{
		LargeButtonToggle((const char*)u8"��Ϣ����������", &ErectusMemory::allowMessages);

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

	if (ImGui::Begin((const char*)u8"Erectus - ͼ��˵�", nullptr,
		ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysVerticalScrollbar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::MenuItem((const char*)u8"�˳�"))
				App::CloseWnd();

			if (ImGui::MenuItem((const char*)u8"���˵�"))
				ErectusProcess::SetProcessMenu();

			if (ImGui::MenuItem((const char*)u8"ͼ��"))
			{
				if (!App::SetOverlayPosition(false, true))
					ErectusProcess::SetProcessMenu();
			}

			ImGui::EndMenuBar();
		}

		if (ImGui::BeginTabBar((const char*)u8"###ͼ��˵�ѡ�", ImGuiTabBarFlags_None))
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
