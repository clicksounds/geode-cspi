#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp> // cspi button
#include <Geode/modify/GJAccountManager.hpp> // account id
#include <Geode/utils/web.hpp> // user validation
#include <Geode/loader/Index.hpp> // outdated mod updater
#include "utils/redownloadIndex.h"

using namespace geode::prelude;

class $modify(IndexModGarageLayer, GJGarageLayer) {
	
	struct Fields {
		EventListener<web::WebTask> m_packCountListener;
		EventListener<web::WebTask> m_userVerifyListener;
		int m_packCount = 10;
		bool m_isValid = false;
		std::string m_packCountMessage = "Failed to retreive pack count message";
	};

	bool init() {
		if (!GJGarageLayer::init()) {
			return false;
		}

		// get pack count by web
		m_fields->m_packCountListener.bind([this] (web::WebTask::Event* e) {
			if (web::WebResponse* res = e->getValue()) {
				auto validText = res->string();
				std::istringstream stream(validText.unwrap());
				std::string line;
				
				// get pack count (first line of pastebin)
				std::getline(stream, line);
				m_fields->m_packCount = std::stoi(line);
				
				// get pack count message (second line of pastebin)
				std::getline(stream, line);
				m_fields->m_packCountMessage = line;
			} else if (e->isCancelled()) {
				log::debug("Failed to get count file, defaulting to 10.");
			}
		});


		auto req = web::WebRequest();
		m_fields->m_packCountListener.setFilter(req.get("https://pastebin.com/raw/PAM4sHpv"));

		
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
		m_fields->m_userVerifyListener.bind([this, sender] (web::WebTask::Event* e) {
			if (web::WebResponse* res = e->getValue()) {
				auto validText = res->string();
				int accountid = GJAccountManager::get()->m_accountID;
				std::istringstream stream(validText.unwrap());
				std::string line;
				
				std::getline(stream, line);
				std::istringstream numbersStream(line);
				std::string number;
				
				// check if accountid is in the list of boosters (first line of pastebin)
				while (std::getline(numbersStream, number, ',')) {
					uint64_t num = geode::utils::numFromString<uint64_t>(number).unwrapOr(0);
					if (num == accountid) {
						log::debug("Account ID checked was valid.");
						m_fields->m_isValid = true;
						break;
					} else {
						log::debug("Account ID checked was invalid.");
					}
				}
				
				// check if freemode is enabled (second line of pastebin)
				std::getline(stream, line);
        		if (line == "true") {
            		m_fields->m_isValid = true;
            		log::debug("Freemode is enabled.");
					startPopup(sender);
        		} else {
            		log::debug("Freemode is disabled.");
        		}

				// check if mod version is outdated (third line of pastebin)
				std::getline(stream, line);
				std::string currentVersion = Mod::get()->getVersion();
				if (line != currentVersion) {
					log::debug("Mod version is outdated. Expected: {} but found: {}", line, currentVersion);
					outdatedPopup(line);
					return;
				}
		
				if (!m_fields->m_isValid) {
					this->invalidVerify("You are not a server booster and freemode is inactive. Please boost the Click Sounds discord server for permanent access.");
					log::debug("The isValid boolean is false.");
					return;
				}

			} else if (e->isCancelled()) {
				this->invalidVerify("Failed to check freemode and boost status. Are you connected to the internet?");
				log::debug("Failed to get file");
				return;
			}
		});
		auto req = web::WebRequest();
		m_fields->m_userVerifyListener.setFilter(req.get("https://pastebin.com/raw/CjABWr6F"));
		
		if (!Mod::get()->getSavedValue<bool>("read-warnings")) {
			introPopup();
		}
	}

	void startPopup(CCObject* sender) {
		if (Mod::get()->getSavedValue<bool>("read-warnings") && m_fields->m_isValid) {
			geode::createQuickPopup(
				"CS Pack Installer",
				fmt::format("What would you like to do?\n\n{}", m_fields->m_packCountMessage),
				"Manage Index", "Add Pack",
				[this, sender](auto, bool btn1) {
					if (btn1) {
						addPackPopup(sender);
					} else {
						manageIndexPopup();
					}
				}
			);
		}
	}

	void outdatedPopup(std::string latestVer) {
		geode::createQuickPopup(
			"CS Pack Installer",
			fmt::format("CS Pack Installer is outdated. The current version is {} but the latest version is {}. \nPlease update to the latest version to continue using CS Pack Installer.", Mod::get()->getVersion(), latestVer),
			"Close", "Update",
			[](auto, bool btn1) {
				if (btn1) {
					geode::openInfoPopup(Loader::get()->getInstalledMod("beat.pack-installer"));
				}
			}
		);
	}

	void invalidVerify(std::string errorMessage = "Unknown error.") {
		geode::createQuickPopup(
			"CS Pack Installer",
			fmt::format("Unable to validate user. Please try again later. Error: \n\"{}\"", errorMessage),
			"Close", nullptr,
			[this](auto, bool btn1) {}
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
			std::filesystem::path dir = dirs::getGeodeDir() / "config" / "beat.click-sound" / "Clicks" / "clicks-main";
				if (btn1) {
					dir /= "Useful";
					fileSelection(sender, dir, "useful");
					log::debug("useful button selected, dir is {}", dir);
				} else {
					dir /= "Meme";
					fileSelection(sender, dir, "meme");
					log::debug("meme button selected, dir is {}", dir);
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
					std::filesystem::path dir = dirs::getGeodeDir() / "config" / "beat.click-sound" / "Clicks" / "clicks-main";

					for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        				std::filesystem::remove_all(entry.path());
    				}

    				std::filesystem::create_directory(dir / "Meme");
    				std::filesystem::create_directory(dir / "Useful");

					FLAlertLayer::create("CS Pack Installer", "Successfully cleared index!", "Close")->show();
					std::ofstream((dirs::getTempDir() / "CSINDEXRELOAD").string()).close();
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
					log::debug("fileSelection(): The path stored in path variable is\n {}", path);

                	std::filesystem::path newDir = dir / path.stem();
                	std::filesystem::create_directory(newDir);
                
                	std::filesystem::path newZipPath = newDir / path.filename();
                	std::filesystem::copy(path, newZipPath, std::filesystem::copy_options::overwrite_existing);

					auto unzip = file::Unzip::create(newZipPath);
					auto unzipResult = unzip.unwrap().extractAllTo(newDir);

					std::filesystem::path clicksDir = newDir / "Clicks";
                	std::filesystem::path releasesDir = newDir / "Releases";

                	int numClicksFiles = 0;
                	int numReleasesFiles = 0;

                	if (std::filesystem::exists(clicksDir) && std::filesystem::is_directory(clicksDir)) {
                	    for (const auto& entry : std::filesystem::directory_iterator(clicksDir)) {
                	        if (std::filesystem::is_regular_file(entry)) {
                	            numClicksFiles++;
                	        }
                	    }
                	}

                	
                	if (std::filesystem::exists(releasesDir) && std::filesystem::is_directory(releasesDir)) {
                	    for (const auto& entry : std::filesystem::directory_iterator(releasesDir)) {
                	        if (std::filesystem::is_regular_file(entry)) {
                	            numReleasesFiles++;
                	        }
                	    }
                	}

					if (!unzipResult) return false;
                	
                	if (numClicksFiles > m_fields->m_packCount || numReleasesFiles > m_fields->m_packCount) {
                	    Loader::get()->queueInMainThread([=] {
							std::filesystem::remove_all(newDir);
						});
						geode::createQuickPopup(
							"CS Pack Installer",
							fmt::format("Pack failed to install: Too many files (max {} clicks and {} releases)", m_fields->m_packCount, m_fields->m_packCount),
							"Close", nullptr,
							[newZipPath](auto, bool btn1) {
								std::filesystem::remove(newZipPath);
							}
						);
                	} else {
						geode::createQuickPopup(
							"CS Pack Installer",
							"Pack installed successfully!",
							"Close", nullptr,
							[newZipPath](auto, bool btn1) {
								std::filesystem::remove(newZipPath);
							}
						);
						std::ofstream((dirs::getTempDir() / "CSINDEXRELOAD").string()).close();
					}
					return false;
            	} else {
					log::debug("Res not okay, res is ({})", res->unwrapErr());
				}
				return true;
        	});
		return;
	}
};