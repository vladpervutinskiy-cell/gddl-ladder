#include "GDDL.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include <fmt/format.h>

#include <algorithm>

using namespace geode::prelude;

namespace {
    constexpr cocos2d::ccColor3B BADGE_COLOR{255, 214, 92};
    constexpr cocos2d::ccColor3B BADGE_MUTED{190, 190, 190};
}

/// Adds a clickable "GDDL #<place>" badge to the info screen of demon levels.
class $modify(GDDLLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        async::TaskHolder<web::WebResponse> levelListener;
        async::TaskHolder<web::WebResponse> countListener;
        cocos2d::CCLabelBMFont* text = nullptr;
        cocos2d::CCLabelBMFont* shadow = nullptr;
        cocos2d::CCNode* root = nullptr;
        cocos2d::CCMenu* menu = nullptr;
    };

    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }
        if (!Mod::get()->getSettingValue<bool>("show-level-badge")) {
            return true;
        }
        // In Geometry Dash every demon (including platformer demons) has 10 stars.
        if (!level || level->m_stars != 10) {
            return true;
        }

        this->createBadge();
        this->loadRank();
        return true;
    }

    void createBadge() {
        auto text = CCLabelBMFont::create("GDDL ...", "chatFont.fnt");
        text->setScale(0.7f);
        text->setColor(BADGE_COLOR);

        auto shadow = CCLabelBMFont::create("GDDL ...", "chatFont.fnt");
        shadow->setScale(0.7f);
        shadow->setColor({0, 0, 0});
        shadow->setOpacity(170);

        auto root = CCNode::create();
        root->setContentSize({44.f, 14.f});
        root->addChild(shadow);
        root->addChild(text);

        auto item = CCMenuItemSpriteExtra::create(root, this, menu_selector(GDDLLevelInfoLayer::onBadgeClick));
        item->setID("gddl-rank-badge"_spr);

        auto menu = CCMenu::create();
        menu->addChild(item);
        menu->setID("gddl-rank-menu"_spr);
        this->addChild(menu, 20);

        m_fields->text = text;
        m_fields->shadow = shadow;
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

        this->setBadgeText("GDDL ...", BADGE_MUTED);
    }

    void setBadgeText(std::string const& value, cocos2d::ccColor3B const& color) {
        auto text = m_fields->text;
        auto shadow = m_fields->shadow;
        auto root = m_fields->root;
        if (!text || !shadow || !root) {
            return;
        }

        text->setString(value.c_str());
        text->setColor(color);
        shadow->setString(value.c_str());

        // The item anchors its content at the bottom-left corner and stretches
        // the clickable area to the content size, so the labels are centered
        // inside 0..size.
        auto labelSize = text->getScaledContentSize();
        cocos2d::CCSize size{labelSize.width + 12.f, labelSize.height + 6.f};
        root->setContentSize(size);
        text->setPosition({size.width / 2.f, size.height / 2.f});
        shadow->setPosition({size.width / 2.f + 1.5f, size.height / 2.f - 1.5f});

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
            this->setBadgeText("GDDL ?", BADGE_MUTED);
            return;
        }

        auto json = jsonResult.unwrap();
        auto rating = json["Rating"].asDouble().unwrapOr(0.0);
        if (!json.contains("Rating") || rating <= 0.0) {
            // The API returns an empty object for levels that are not rated.
            gddl::RankInfo info;
            gddl::Client::get().storeRank(levelID, info);
            this->setBadgeText("GDDL not rated", BADGE_MUTED);
            return;
        }

        if (rating >= 39.0) {
            // 39 is the highest tier, so it is always shared first place.
            gddl::RankInfo info;
            info.found = true;
            info.rating = rating;
            info.rank = 1;
            gddl::Client::get().storeRank(levelID, info);
            this->applyRank(info);
            return;
        }

        // Number of levels rated strictly higher than this one is equal to the
        // number of levels whose rating is >= the next representable double.
        auto request = gddl::makeRequest();
        request.param("excludeUnrated", "true");
        request.param("minRating", fmt::format("{}", gddl::nextRating(rating)));
        request.param("limit", "1");
        request.param("page", "0");

        m_fields->countListener.spawn(
            request.get(fmt::format("{}/levels", gddl::API_BASE), Mod::get()),
            [this, levelID, rating](web::WebResponse countResponse) {
                this->onCountResponse(std::move(countResponse), levelID, rating);
            }
        );
    }

    void onCountResponse(web::WebResponse response, int levelID, double rating) {
        int rank = 0;
        auto jsonResult = response.json();
        if (response.ok() && jsonResult.isOk()) {
            auto json = jsonResult.unwrap();
            if (json.contains("total")) {
                rank = static_cast<int>(json["total"].asInt().unwrapOr(0)) + 1;
            }
        }

        if (rank <= 0) {
            this->setBadgeText("GDDL ?", BADGE_MUTED);
            return;
        }

        gddl::RankInfo info;
        info.found = true;
        info.rating = rating;
        info.rank = rank;
        gddl::Client::get().storeRank(levelID, info);
        this->applyRank(info);
    }

    void applyRank(gddl::RankInfo const& info) {
        if (!info.found) {
            this->setBadgeText("GDDL not rated", BADGE_MUTED);
            return;
        }
        this->setBadgeText(fmt::format("GDDL #{} ({:.2f})", info.rank, info.rating), BADGE_COLOR);
    }

    void onBadgeClick(CCObject*) {
        std::string url = gddl::siteLevelUrl(m_level->m_levelID);
        geode::utils::web::openLinkInBrowser(url);
    }
};
