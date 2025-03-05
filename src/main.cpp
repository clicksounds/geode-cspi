#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include <Geode/modify/GJAccountManager.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class $modify(MenuLayer) {
	struct Fields {
		EventListener<web::WebTask> m_userVerifyListener;
		EventListener<web::WebTask> m_autoUpdateListener;
		EventListener<web::WebTask> m_latestPastebinListener;
		bool m_gameInitialized = false;
		std::string m_cdnLink = "https://ligma.balls/";
	};

	bool init () {
		if (!MenuLayer::init() || m_fields->m_gameInitialized) {
			return false;
		}

		/*m_fields->m_userVerifyListener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                auto validText = res->string();
				int accountid = GJAccountManager::get()->m_accountID;
				std::istringstream stream(validText.unwrap());
        		std::string number;
        		bool isValid = false;
				while (std::getline(stream, number, ',')) {
                	uint64_t num = geode::utils::numFromString<uint64_t>(number).unwrapOr(0);
                	if (accountid == 0) {
						this->invalidVerify();
					} else if (num == accountid) {
                	    log::debug("You are valid.");
						isValid = true;
                	} else {
						log::debug("You are invalid.");
					}
				}
				if (!isValid) {
					this->invalidVerify();
					log::debug("The isValid boolean is false.");
				}
            } else if (e->isCancelled()) {
                this->invalidVerify();
				log::debug("Failed to get file");
            }
        });*/
		
		// auto update system

		m_fields->m_latestPastebinListener.bind([this] (web::WebTask::Event* e) {
			if (web::WebResponse* res = e->getValue()) {
				m_fields->m_cdnLink = res->string().unwrapOr("Error: Unwrapping res string failed on pastebin listener binding.");
				log::debug("res string is {}", res->string().unwrapOr("Error: Unwrapping res string failed on debug log"));
				log::debug("m_cdnlink is {}", m_fields->m_cdnLink);
			}
		});

		m_fields->m_autoUpdateListener.bind([this] (web::WebTask::Event* e) {
			if (web::WebResponse* res = e->getValue()) {
				if (res->string().unwrapOr("failed") == "failed" || m_fields->m_gameInitialized) return;
				std::filesystem::path tempDownloadPath = dirs::getTempDir() / "beat.pack-installer-temp.geode";
				res->into(tempDownloadPath);
				if (res->into(tempDownloadPath)) {
					std::filesystem::copy(tempDownloadPath, dirs::getModsDir() / "beat.pack-installer.geode", std::filesystem::copy_options::overwrite_existing);
					std::filesystem::remove(tempDownloadPath);
				}
				m_fields->m_gameInitialized = true;
			}
		});

		auto req = web::WebRequest();
		//m_fields->m_userVerifyListener.setFilter(req.get("https://pastebin.com/raw/CjABWr6F"));
		m_fields->m_latestPastebinListener.setFilter(req.get("https://pastebin.com/raw/PTY7nQ5V"));
		m_fields->m_autoUpdateListener.setFilter(req.get(m_fields->m_cdnLink));
		return true;
	}
	void invalidVerify() {
		geode::createQuickPopup( 
			"CS Pack Installer",
			"Unable to validate user. Please try again later.",
			"Disable CSPI", "Close GD",
			[this](auto, bool btn1) {
				if (btn1) {
					game::exit();
				} else {
					Mod::get()->disable();
					game::restart();
				}
			}
		);
	}
};

class $modify(IndexModGarageLayer, GJGarageLayer) {
	
	struct Fields {
		EventListener<web::WebTask> m_packCountListener;
		int m_packCount = 10;
	};
	
	bool init() {
		if (!GJGarageLayer::init()) {
			return false;
		}

		// get pack count by web

		m_fields->m_packCountListener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
				m_fields->m_packCount = std::stoi(res->string().unwrap());
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
		if (!Mod::get()->getSavedValue<bool>("read-warnings")) {
			geode::createQuickPopup("CS Pack Installer", "Thank you for boosting the Click Sounds server!\n<cr>Read these instructions carefully, as they will not be repeated.</c>", "Next", nullptr, [this](auto, bool btn1) {
				if (!btn1) {
					geode::createQuickPopup("CS Pack Installer", "To make a pack, use the pack generator on the Click Sounds website.", "Next", nullptr, [this](auto, bool btn1) {
						if (!btn1) {
							geode::createQuickPopup("CS Pack Installer", "To redownload the index after clearing it, disable CSPI temporarily and restart Geometry Dash.", "Next", nullptr, [this](auto, bool btn1) {
								if (!btn1) {
									geode::createQuickPopup("CS Pack Installer", fmt::format("Packs installed here can have a maximum of \n<cg>{} Click Sounds and {} Release Sounds.</c>", m_fields->m_packCount, m_fields->m_packCount), "Next", nullptr, [](auto, bool btn1) {
										if (!btn1) {
											geode::createQuickPopup("CS Pack Installer", "Once again, thank you for boosting the server. Have fun!", "Close", nullptr, [](auto, bool btn1){
												if (!btn1) { Mod::get()->setSavedValue<bool>("read-warnings", true); }
											});
										}
									});
								}
							});
						}
					});
				}
			});
		}		
		
		if (Mod::get()->getSavedValue<bool>("read-warnings"))
			geode::createQuickPopup(
				"CS Pack Installer",
				fmt::format("What would you like to do?\n\nPacks with up to {} clicks and releases per type can be installed.", m_fields->m_packCount),
				"Clear Index", "Add Pack",
				[this, sender](auto, bool btn1) {
					if (btn1) {
						addPackPopup(sender);
					} else {
						clearIndexPopup();
					}
				}
			);
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

	void clearIndexPopup() {
		std::filesystem::path dir = dirs::getGeodeDir() / "config" / "beat.click-sound" / "Clicks" / "clicks-main";

		for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        	std::filesystem::remove_all(entry.path());
    	}

    	std::filesystem::create_directory(dir / "Meme");
    	std::filesystem::create_directory(dir / "Useful");

		geode::createQuickPopup(
			"CS Pack Installer",
			"Successfully cleared index! Restart to apply changes.",
			"Close", "Restart",
			[](auto, bool btn1) {
				if (btn1) {
					game::restart();
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
							"Pack installed successfully! Restart to apply changes.",
							"Restart", "Close",
							[newZipPath](auto, bool btn1) {
								std::filesystem::remove(newZipPath);
								if (!btn1) {
									game::restart();
								}
							}
						);
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