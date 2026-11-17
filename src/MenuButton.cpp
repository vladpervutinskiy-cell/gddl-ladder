#include "TopLevelsPopup.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

/// Adds a "GDDL TOP" button to the main menu.
class $modify(GDDLMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        bool enabled = Mod::get()->getSettingValue<bool>("show-menu-button");
        log::info("GDDL: MenuLayer::init, show-menu-button = {}", enabled);
        if (!enabled) {
            return true;
        }
        if (this->getChildByID("gddl-top-button"_spr)) {
            return true;
        }

        auto sprite = ButtonSprite::create("GDDL TOP", "bigFont.fnt", "GJ_button_01.png", 0.5f);
        // The button shares a row with the game's own icons, so it is scaled
        // down to a fixed width instead of relying on the font size.
        constexpr float MAX_WIDTH = 70.f;
        float width = sprite->getContentSize().width * sprite->getScale();
        if (width > MAX_WIDTH) {
            sprite->setScale(sprite->getScale() * MAX_WIDTH / width);
        }

        auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(GDDLMenuLayer::onTopLevels));
        button->setID("gddl-top-button"_spr);

        // Geode prepares empty menus in the main menu for mods, and their
        // layouts handle the positioning. Using one of them means no manual
        // coordinates are needed (a layer ignores its anchor point, so hand
        // written node-space maths can end up off screen).
        if (auto bottomMenu = this->getChildByID("bottom-menu")) {
            bottomMenu->addChild(button);
            bottomMenu->updateLayout();
            log::info("GDDL: menu button placed in bottom-menu");
            return true;
        }
        if (auto sideMenu = this->getChildByID("side-menu")) {
            sideMenu->addChild(button);
            sideMenu->updateLayout();
            log::info("GDDL: menu button placed in side-menu");
            return true;
        }

        // Last resort: our own menu in the upper left corner.
        auto menu = CCMenu::create();
        menu->addChild(button);
        menu->setID("gddl-top-menu"_spr);
        menu->setPosition(60.f, 260.f);
        this->addChild(menu, 100);
        log::warn("GDDL: no bottom-menu or side-menu found, placed the button manually");
        return true;
    }

    void onTopLevels(CCObject*) {
        GDDLTopPopup::open();
    }
};
