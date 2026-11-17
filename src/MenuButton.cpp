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
        if (!Mod::get()->getSettingValue<bool>("show-menu-button")) {
            return true;
        }
        if (this->getChildByID("gddl-top-menu"_spr)) {
            return true;
        }

        auto sprite = ButtonSprite::create("GDDL TOP", "bigFont.fnt", "GJ_button_01.png", 0.6f);
        auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(GDDLMenuLayer::onTopLevels));
        button->setID("gddl-top-button"_spr);

        auto menu = CCMenu::create();
        menu->addChild(button);
        menu->setID("gddl-top-menu"_spr);
        menu->setPosition(this->findButtonPosition());
        this->addChild(menu, 6);

        return true;
    }

    /// One row above the achievements icon, at the far left of the bottom row,
    /// which is usually empty space. Falls back to the middle of the left edge.
    cocos2d::CCPoint findButtonPosition() {
        if (auto bottomMenu = this->getChildByID("bottom-menu")) {
            if (auto achievements = bottomMenu->getChildByID("achievements-button")) {
                auto world = achievements->convertToWorldSpaceAR({0.f, 0.f});
                auto position = this->convertToNodeSpaceAR(world);
                position.y += 46.f;
                return position;
            }
        }
        auto winSize = cocos2d::CCDirector::get()->getWinSize();
        return {64.f, winSize.height / 2.f};
    }

    void onTopLevels(CCObject*) {
        GDDLTopPopup::open();
    }
};
