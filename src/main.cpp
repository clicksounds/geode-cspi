#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp> // cspi button
#include "utils/redownloadIndex.h"

using namespace geode::prelude;

class $modify(IndexModGarageLayer, GJGarageLayer) {
	
	struct Fields {
		bool m_filePickerOpen = false;
	};

	bool init() {
		if (!GJGarageLayer::init()) {
			return false;
		}

		// sets the persistent directory to the geode folder if the user hasnt ever chosen a path before
		if (Mod::get()->getSavedValue<std::filesystem::path>("persistent-dir").empty() || !std::filesystem::exists(Mod::get()->getSavedValue<std::filesystem::path>("persistent-dir"))) {
			Mod::get()->setSavedValue<std::filesystem::path>("persistent-dir", dirs::getGeodeDir());
		}
		
		auto spr = CCSprite::create("piicon.png"_spr);

        auto piButton = CCMenuItemSpriteExtra::create(
			spr,
			this,
			menu_selector(IndexModGarageLayer::onPiButton)
		);

		auto sideBar = this->getChildByID("shards-menu");
		sideBar->addChild(piButton);
		piButton->setID("pi-button"_spr);
		spr->setScale(0.85);
		sideBar->updateLayout();
		return true;
	}

	void onPiButton(CCObject* sender) {
		Mod::get()->getSavedValue<bool>("read-warnings") ? startPopup(sender) : introPopup();
	}

	void startPopup(CCObject* sender) {
		geode::createQuickPopup(
			"CS Pack Installer",
			"What would you like to do?\n\nPacks with over 25 clicks and/or 25 releases will not be accepted on the Click Sounds Index.",
			"Manage Index", "Add Pack",
			[this, sender](auto, bool btn1) {
				if (btn1) {
					fileSelection(sender);
				} else {
					if (!Loader::get()->getInstalledMod("beat.click-sound")->getSavedValue<bool>("CSINDEXDOWNLOADING")) {
						manageIndexPopup();
					} else {
						FLAlertLayer::create("CS Pack Installer", "The index can not be managed while it is downloading. Please wait.", "Close")->show();
					}
				}
			}
		);
	}

	void introPopup(int i = 0) {
		static const std::vector<std::pair<std::string, std::string>> msgContent = {
			{"<cr>Read these instructions carefully, as they will not be repeated.</c>", "Next"},
			{"To make a pack, use the pack generator on the Click Sounds website.", "Next"},
			{"Only <cg>.packgen.zip</c> click pack files can be installed. <cr>Packs from ZCB Live are not compatible.</c>", "Next"},
			{"To convert a <cr>ZCB Live click pack</c> to <cg>.packgen.zip</c>, use the pack generator on the Click Sounds website.", "Next"},
			{"The Click Sounds Index will not redownload on startup while CSPI is enabled. It can be manually updated from the CSPI menu while enabled.", "Next"},
			{"When freemode is not active, you will need to boost the Click Sounds discord server to use CSPI.", "Close"}
		};
	
		if (i >= msgContent.size()) {
			Mod::get()->setSavedValue<bool>("read-warnings", true);
			return;
		}
	
		geode::createQuickPopup("CS Pack Installer", msgContent[i].first.c_str(), msgContent[i].second.c_str(), nullptr, [this, i](auto, bool btn1) {
			if (!btn1) {
				introPopup(i + 1);
			}
		});
	}

	void manageIndexPopup() {
		geode::createQuickPopup(
			"CS Pack Installer",
			"What do you want to do with the index?",
			"Clear", "Redownload",
			[this](auto, bool btn1) {
				if (btn1) {
					// redownload index
					geode::createQuickPopup(
						"CS Pack Installer",
						"Redownloading the index <cr>will remove all packs you've installed</c>. \nAre you sure you want to do this?",
						"Cancel", "Redownload",
						[this](auto, bool btn1) {
							if (btn1) {
								redownloadIndex();
							}
						}
					);
				} else {
					// clear index
					std::filesystem::path dir = Loader::get()->getInstalledMod("beat.click-sound")->getConfigDir() / "Clicks" / "clicks-main";

					for (const auto& entry : std::filesystem::directory_iterator(dir)) {
						std::filesystem::remove_all(entry.path());
					}

					std::filesystem::create_directory(dir / "Meme");
					std::filesystem::create_directory(dir / "Useful");

					FLAlertLayer::create("CS Pack Installer", "Successfully cleared index!", "Close")->show();
					Loader::get()->getInstalledMod("beat.click-sound")->setSavedValue("CSINDEXRELOAD", true);
				}
			}
		);
	}

	void fileSelection(CCObject* sender) {
		std::filesystem::path dir = Loader::get()->getInstalledMod("beat.click-sound")->getConfigDir() / "Clicks" / "clicks-main";
		file::FilePickOptions::Filter textFilter;
		file::FilePickOptions fileOptions;
		textFilter.description = "Click Pack";
		textFilter.files = { "*.zip" };
		fileOptions.filters.push_back(textFilter);
		auto getPersistentDir = Mod::get()->getSavedValue<std::filesystem::path>("persistent-dir");

		if (m_fields->m_filePickerOpen) return;
		m_fields->m_filePickerOpen = true;

		file::pick(file::PickMode::OpenFile, { getPersistentDir, { textFilter } }).listen(
			[this, sender, dir](Result<std::filesystem::path>* res) {
				std::filesystem::path path;
				if (!res || !res->isOk()) return false;
				if (res->isOk()) {
					path = res->unwrap();
					Mod::get()->setSavedValue<std::filesystem::path>("persistent-dir", path);

					std::filesystem::path tempDir = dirs::getTempDir() / path.stem();
					std::filesystem::create_directories(tempDir);

					std::filesystem::path tempZipPath = tempDir / path.filename();
					std::filesystem::copy(path, tempZipPath, std::filesystem::copy_options::overwrite_existing);

					auto unzip = file::Unzip::create(tempZipPath);
					auto unzipResult = unzip.unwrap().extractAllTo(tempDir);
					if (!unzipResult) return false;

					std::filesystem::path packJsonPath = tempDir / "pack.json";
					std::string type = "Useful";

					if (std::filesystem::exists(packJsonPath)) {
						std::ifstream jsonFile(packJsonPath);
						if (jsonFile.is_open()) {
							std::string content((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
							auto jsonData = matjson::parse(content).unwrapOr(-2);
							if (jsonData.contains("type")) {
								auto val = jsonData.get("type").unwrap();
								if (val == "Meme") type = "Meme";
							}
						}
					}

					std::filesystem::path newDir = dir / type / path.stem();
					std::filesystem::create_directories(newDir);

					for (auto& p : std::filesystem::recursive_directory_iterator(tempDir)) {
						if (p.is_directory()) continue;
						auto rel = std::filesystem::relative(p.path(), tempDir);
						std::filesystem::create_directories(newDir / rel.parent_path());
						std::filesystem::copy_file(p.path(), newDir / rel, std::filesystem::copy_options::overwrite_existing);
					}

					geode::createQuickPopup(
						"CS Pack Installer",
						"Pack installed successfully!",
						"Close", nullptr,
						[tempZipPath](auto, bool) {
							std::filesystem::remove(tempZipPath);
						}
					);

					Loader::get()->getInstalledMod("beat.click-sound")->setSavedValue("CSINDEXRELOAD", true);
					return false;
				}
				return true;
			});
		return;
	}
};
