#include "GDDL.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/utils/web.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <string>

using namespace geode::prelude;

namespace {
    constexpr cocos2d::ccColor3B BADGE_COLOR{255, 214, 92};
    constexpr cocos2d::ccColor3B BADGE_MUTED{190, 190, 190};
    constexpr float BADGE_SCALE = 0.75f;
    constexpr float PAD_X = 8.f;
    constexpr float PAD_Y = 5.f;
    constexpr float PART_GAP = 4.f;
}

/// Adds a clickable "GDDL #<place>" badge to the info screen of demon levels.
class $modify(GDDLLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        async::TaskHolder<web::WebResponse> levelListener;
        CCLabelBMFont* main = nullptr;
        CCLabelBMFont* extra = nullptr;
        NineSlice* panel = nullptr;
        cocos2d::CCNode* root = nullptr;
        cocos2d::CCMenu* menu = nullptr;
    };

    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }
        if (!Mod::get()->getSettingValue<bool>("show-level-badge")) {
            log::info("GDDL: level info opened, badge disabled in settings");
            return true;
        }
        // In Geometry Dash every demon (including platformer demons) has 10 stars.
        if (!level || level->m_stars != 10) {
            log::info("GDDL: level info opened, not a demon (stars = {}), no badge",
                level ? level->m_stars : -1);
            return true;
        }

        log::info("GDDL: level info opened for demon {}, adding badge", level->m_levelID);
        this->createBadge();
        this->loadRank();
        return true;
    }

    /// A small dark plate with the gold "GDDL #890" and the rating in the
    /// colour of its tier, sitting right below the star counter.
    void createBadge() {
        auto root = cocos2d::CCNode::create();

        auto panel = NineSlice::createWithSpriteFrameName(
            "GJ_square01.png", NineSlice::Insets{8.f, 8.f, 8.f, 8.f}
        );
        panel->setColor({0, 0, 0});
        panel->setOpacity(150);
        root->addChild(panel);

        auto main = CCLabelBMFont::create("GDDL ...", "chatFont.fnt");
        main->setScale(BADGE_SCALE);
        main->setAnchorPoint({0.f, 0.5f});
        root->addChild(main);

        auto extra = CCLabelBMFont::create("", "chatFont.fnt");
        extra->setScale(BADGE_SCALE);
        extra->setAnchorPoint({0.f, 0.5f});
        root->addChild(extra);

        auto item = CCMenuItemSpriteExtra::create(root, this, menu_selector(GDDLLevelInfoLayer::onBadgeClick));
        item->setID("gddl-rank-badge"_spr);

        auto menu = CCMenu::create();
        menu->addChild(item);
        menu->setID("gddl-rank-menu"_spr);
        this->addChild(menu, 20);

        m_fields->main = main;
        m_fields->extra = extra;
        m_fields->panel = panel;
        m_fields->root = root;
        m_fields->menu = menu;

        // Sit right below the star counter (and below the "Tier" label of the
        // official GDDL Integration mod, if it is installed).
        cocos2d::CCPoint position{60.f, 60.f};
        if (m_starsLabel) {
            float shift = 1.f;
            if (m_level->m_coins > 0) {
                shift += 0.8f;
            }
            if (m_level->m_dailyID > 100000) {
                shift += 0.8f;
            }
            position.x = m_starsLabel->getPositionX();
            position.y = m_starsLabel->getPositionY() - (shift + 1.f) * 15.5f;
        }
        menu->setPosition(position);

        this->setBadgeParts("GDDL ...", "", BADGE_MUTED, BADGE_MUTED);
    }

    /// Sets the two halves of the badge text ("GDDL #890" and "(23.97)") and
    /// resizes everything to fit.
    void setBadgeParts(
        std::string const& mainText,
        std::string const& extraText,
        cocos2d::ccColor3B const& mainColor,
        cocos2d::ccColor3B const& extraColor
    ) {
        auto main = m_fields->main;
        auto extra = m_fields->extra;
        auto panel = m_fields->panel;
        auto root = m_fields->root;
        if (!main || !extra || !panel || !root) {
            return;
        }

        main->setString(mainText.c_str());
        main->setColor(mainColor);
        extra->setString(extraText.c_str());
        extra->setColor(extraColor);

        auto mainSize = main->getScaledContentSize();
        auto extraSize = extra->getScaledContentSize();
        float gap = extraSize.width > 0.f ? PART_GAP : 0.f;
        float textHeight = std::max(mainSize.height, extraSize.height);

        cocos2d::CCSize size{
            mainSize.width + gap + extraSize.width + PAD_X * 2.f,
            textHeight + PAD_Y * 2.f
        };

        root->setContentSize(size);
        panel->setContentSize(size);
        panel->setPosition({size.width / 2.f, size.height / 2.f});
        main->setPosition({PAD_X, size.height / 2.f});
        extra->setPosition({PAD_X + mainSize.width + gap, size.height / 2.f});

        // The item anchors its content at the bottom-left corner and stretches
        // the clickable area to the content size.
        if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(root->getParent())) {
            item->setContentSize(size);
        }

        // Keep the badge inside the screen even when the text is long.
        if (auto menu = m_fields->menu) {
            auto winSize = cocos2d::CCDirector::get()->getWinSize();
            auto pos = menu->getPosition();
            pos.x = std::clamp(pos.x, size.width / 2.f + 4.f, winSize.width - size.width / 2.f - 4.f);
            pos.y = std::clamp(pos.y, size.height / 2.f + 4.f, winSize.height - size.height / 2.f - 4.f);
            menu->setPosition(pos);
        }
    }

    void loadRank() {
        int levelID = m_level->m_levelID;
        if (auto cached = gddl::Client::get().getRank(levelID)) {
            this->applyRank(*cached);
            return;
        }

        auto request = gddl::makeRequest();
        m_fields->levelListener.spawn(
            request.get(gddl::apiLevelUrl(levelID), Mod::get()),
            [this, levelID](web::WebResponse response) {
                this->onLevelResponse(std::move(response), levelID);
            }
        );
    }

    void onLevelResponse(web::WebResponse response, int levelID) {
        auto jsonResult = response.json();
        if (!response.ok() || !jsonResult.isOk()) {
            this->setBadgeParts("GDDL ?", "", BADGE_MUTED, BADGE_MUTED);
            return;
        }

        // The level document has both the rating and the place on the ladder,
        // so one request is enough.
        auto info = gddl::parseLevelInfo(jsonResult.unwrap());
        gddl::Client::get().storeRank(levelID, info);
        this->applyRank(info);
    }

    void applyRank(gddl::RankInfo const& info) {
        if (!info.found) {
            this->setBadgeParts("GDDL not rated", "", BADGE_MUTED, BADGE_MUTED);
            return;
        }

        auto ratingText = fmt::format("({:.2f})", info.rating);
        auto ratingTint = gddl::ratingColor(info.rating);
        if (info.rank <= 0) {
            this->setBadgeParts("GDDL", ratingText, BADGE_COLOR, ratingTint);
            return;
        }
        this->setBadgeParts(fmt::format("GDDL #{}", info.rank), ratingText, BADGE_COLOR, ratingTint);
    }

    void onBadgeClick(CCObject*) {
        std::string url = gddl::siteLevelUrl(m_level->m_levelID);
        geode::utils::web::openLinkInBrowser(url);
    }
};
