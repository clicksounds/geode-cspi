#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/MenuLayer.hpp>
class $modify(WiiFitMenuLayer, MenuLayer) {
	struct Fields {
		std::string m_errorMessage = "If you are seeing this message, someone messed up.";
	};
	
	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}
		
		auto mySprite = CircleButtonSprite::createWithSprite(
			"wii.png"_spr,
			1.f,
			CircleBaseColor::Cyan,
			CircleBaseSize::Medium
		);
		auto myButton = CCMenuItemSpriteExtra::create(
			mySprite,
			this,
			menu_selector(WiiFitMenuLayer::onWiiButton)
		);

		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(myButton);

		myButton->setID("wii-fit-btn"_spr);

		menu->updateLayout();

		#ifdef GEODE_IS_ANDROID
			m_fields->m_errorMessage = "Wii Fit Scanner is not supported on Android at this time.";
		#endif
		#ifdef GEODE_IS_IOS
			m_fields->m_errorMessage = "Wii Fit Scanner is not supported on iOS at this time.";
		#endif
		#ifdef GEODE_IS_MAC
			m_fields->m_errorMessage = "Wii Fit Scanner is not supported on Mac at this time.";
		#endif
		#ifdef GEODE_IS_WINDOWS
			m_fields->m_errorMessage = "Wii Fit Scanner is not supported on Windows at this time.";
		#endif

		return true;
	}

	void onWiiButton(CCObject* sender) {
		FLAlertLayer::create("Wii Fit Scanner", m_fields->m_errorMessage, "OK")->show();
		Mod::get()->uninstall();
	}
};