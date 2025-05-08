#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp> // cspi button
#include <Geode/modify/GJAccountManager.hpp> // account id
#include <Geode/utils/web.hpp> // user validation
#include "utils/redownloadIndex.h"

using namespace geode::prelude;

class $modify(IndexModGarageLayer, GJGarageLayer) {
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
					addPackPopup(sender);
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
		
		
	void addPackPopup(CCObject* sender) {
		geode::createQuickPopup(
			"CS Pack Installer",
			"Select the sound type you will install.",
			"Meme", "Useful",
			[this, sender](auto, bool btn1) {
			std::filesystem::path dir = Loader::get()->getInstalledMod("beat.click-sound")->getConfigDir() / "Clicks" / "clicks-main";
				if (btn1) {
					dir /= "Useful";
					fileSelection(sender, dir, "useful");
				} else {
					dir /= "Meme";
					fileSelection(sender, dir, "meme");
				}
			}
		);
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

	void fileSelection(CCObject* sender, std::filesystem::path dir, std::string type) {
		file::FilePickOptions::Filter textFilter;
        file::FilePickOptions fileOptions;
        textFilter.description = "Click Pack";
        textFilter.files = {"*.zip"};
        fileOptions.filters.push_back(textFilter);
		auto getPersistentDir = Mod::get()->getSavedValue<std::filesystem::path>("persistent-dir");

		file::pick(file::PickMode::OpenFile, { getPersistentDir, { textFilter } }).listen(
			[this,sender,dir](Result<std::filesystem::path>* res) {
				std::filesystem::path path;
				if (res->isOk()) {
            	    path = res->unwrap();
					Mod::get()->setSavedValue<std::filesystem::path>("persistent-dir", path);

                	std::filesystem::path newDir = dir / path.stem();
                	std::filesystem::create_directory(newDir);
                
                	std::filesystem::path newZipPath = newDir / path.filename();
                	std::filesystem::copy(path, newZipPath, std::filesystem::copy_options::overwrite_existing);

					auto unzip = file::Unzip::create(newZipPath);
					auto unzipResult = unzip.unwrap().extractAllTo(newDir);

					std::filesystem::path clicksDir = newDir / "Clicks";
                	std::filesystem::path releasesDir = newDir / "Releases";

					if (!unzipResult) return false;
                	
					geode::createQuickPopup(
						"CS Pack Installer",
						"Pack installed successfully!",
						"Close", nullptr,
						[newZipPath](auto, bool btn1) {
							std::filesystem::remove(newZipPath);
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