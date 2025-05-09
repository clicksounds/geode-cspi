#include "redownloadIndex.h"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

void redownloadIndex() {
    std::filesystem::path configDir = Loader::get()->getInstalledMod("beat.click-sound")->getConfigDir();

    Loader::get()->queueInMainThread([=] {
        Notification::create("CS: Downloading index...", CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png"))->show();
        Loader::get()->getInstalledMod("beat.click-sound")->setSavedValue("CSINDEXDOWNLOADING", true);
    });

    web::WebRequest().get("https://github.com/clicksounds/clicks/archive/refs/heads/main.zip").listen([=](auto res) {
        if (res->string().unwrapOr("failed") == "failed") {
            Loader::get()->queueInMainThread([=] {
                Notification::create("CS: Download failed.", CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png"))->show();
            });
            Loader::get()->getInstalledMod("beat.click-sound")->setSavedValue("CSINDEXDOWNLOADING", false);
            return;
        }

        if (res->into(configDir / "Clicks.zip")) {
            std::thread([=] {
                auto unzip = file::Unzip::create(configDir / "Clicks.zip");
                if (!unzip) {
                    Loader::get()->getInstalledMod("beat.click-sound")->setSavedValue("CSINDEXDOWNLOADING", false);
                    return;
                }

                std::filesystem::remove_all(configDir / "Clicks");
                (void) unzip.unwrap().extractAllTo(configDir / "Clicks");

                Loader::get()->queueInMainThread([=] {
                    std::filesystem::path clicksDir = configDir / "Clicks" / "clicks-main";
                    for (const auto& entry : std::filesystem::directory_iterator(clicksDir)) {
                        if (entry.path().filename() != "Meme" && entry.path().filename() != "Useful") {
                            std::filesystem::remove_all(entry.path());
                        }
                    }
                    if (std::filesystem::exists(configDir / "Clicks.zip")) {
                        std::filesystem::remove(configDir / "Clicks.zip");
                    }
                    
                    Notification::create("CS: Download successful!", CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png"))->show();

                    FLAlertLayer::create("CS Pack Installer", "Successfully redownloaded index!", "Close")->show();
                    Loader::get()->getInstalledMod("beat.click-sound")->setSavedValue("CSINDEXRELOAD", true);
                    Loader::get()->getInstalledMod("beat.click-sound")->setSavedValue("CSINDEXDOWNLOADING", false);
                });
            }).detach();
        } else {
            Loader::get()->queueInMainThread([=] {
                Notification::create("CS: Download failed.", CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png"))->show();
                Loader::get()->getInstalledMod("beat.click-sound")->setSavedValue("CSINDEXDOWNLOADING", false);
            });
        }
    }, [](auto prog) {},
    [=]() {
        Loader::get()->queueInMainThread([=] {
            Notification::create("CS: Download failed.", CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png"))->show();
        });
    });
}