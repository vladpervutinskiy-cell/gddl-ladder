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

        auto sprite = ButtonSprite::create("GDDL TOP", "bigFont.fnt", "GJ_button_01.png", 0.55f);
        auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(GDDLMenuLayer::onTopLevels));
        button->setID("gddl-top-button"_spr);

        // Geode puts empty menus into the main menu for mods to fill, and their
        // layouts handle the positioning. Using one of them avoids guessing
        // coordinates (a layer ignores its anchor point, so node-space maths
        // done by hand ends up off screen).
        if (auto sideMenu = this->getChildByID("side-menu")) {
            sideMenu->addChild(button);
            sideMenu->updateLayout();
            log::info("GDDL: menu button placed in side-menu");
            return true;
        }
        if (auto bottomMenu = this->getChildByID("bottom-menu")) {
            bottomMenu->addChild(button);
            bottomMenu->updateLayout();
            log::info("GDDL: menu button placed in bottom-menu");
            return true;
        }

        // Last resort: our own menu in the upper left corner.
        auto menu = CCMenu::create();
        menu->addChild(button);
        menu->setID("gddl-top-menu"_spr);
        menu->setPosition(60.f, 260.f);
        this->addChild(menu, 100);
        log::warn("GDDL: no side-menu or bottom-menu found, placed the button manually");
        return true;
    }

    void onTopLevels(CCObject*) {
        GDDLTopPopup::open();
    }
};
